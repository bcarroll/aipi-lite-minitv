#include "playlist.h"

#include <ArduinoJson.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

namespace {

bool validText(JsonVariantConst value, size_t maxLength, String& output) {
  if (!value.is<const char*>()) {
    return false;
  }
  const char* text = value.as<const char*>();
  if (text == nullptr) {
    return false;
  }
  size_t length = 0;
  while (length <= maxLength && text[length] != '\0') {
    ++length;
  }
  if (length == 0 || length > maxLength) {
    return false;
  }
  for (size_t i = 0; i < length; ++i) {
    const unsigned char ch = static_cast<unsigned char>(text[i]);
    if (ch < 0x20 || ch == 0x7f) {
      return false;
    }
  }
  output = text;
  return true;
}

bool validId(const String& id) {
  for (size_t i = 0; i < id.length(); ++i) {
    const char ch = id[i];
    if (!(isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_')) {
      return false;
    }
  }
  return true;
}

bool hasExtension(const String& value, const char* extension) {
  const size_t valueLength = value.length();
  const size_t extensionLength = strlen(extension);
  if (valueLength < extensionLength) {
    return false;
  }
  for (size_t i = 0; i < extensionLength; ++i) {
    char left = value[valueLength - extensionLength + i];
    char right = extension[i];
    if (left >= 'A' && left <= 'Z') {
      left = static_cast<char>(left - 'A' + 'a');
    }
    if (right >= 'A' && right <= 'Z') {
      right = static_cast<char>(right - 'A' + 'a');
    }
    if (left != right) {
      return false;
    }
  }
  return true;
}

bool safeLocalPath(const String& path, const char* extension) {
  if (path.length() > Playlist::kMaximumPathLength ||
      !path.startsWith("/Videos/") || path.indexOf('\\') >= 0 ||
      path.indexOf('%') >= 0 || path.indexOf(':') >= 0 ||
      path.indexOf('?') >= 0 || path.indexOf('#') >= 0 ||
      !hasExtension(path, extension)) {
    return false;
  }
  int start = 0;
  while (start < static_cast<int>(path.length())) {
    const int slash = path.indexOf('/', start);
    const int end = slash < 0 ? path.length() : slash;
    if (end - start == 2 && path[start] == '.' && path[start + 1] == '.') {
      return false;
    }
    start = end + 1;
  }
  return true;
}

bool safeHttpUrl(const String& url, const char* extension) {
  if (url.length() > Playlist::kMaximumUrlLength || url.indexOf(' ') >= 0 ||
      url.indexOf('\\') >= 0 || url.indexOf('#') >= 0 ||
      !(url.startsWith("http://") || url.startsWith("https://"))) {
    return false;
  }
  const int schemeEnd = url.indexOf("://");
  const int authorityStart = schemeEnd + 3;
  int authorityEnd = url.indexOf('/', authorityStart);
  const int queryAt = url.indexOf('?', authorityStart);
  if (authorityEnd < 0 || (queryAt >= 0 && queryAt < authorityEnd)) {
    authorityEnd = queryAt;
  }
  if (authorityEnd < 0 || authorityEnd == authorityStart ||
      url.substring(authorityStart, authorityEnd).indexOf('@') >= 0) {
    return false;
  }
  String resource = url.substring(authorityEnd);
  const int query = resource.indexOf('?');
  if (query >= 0) {
    resource.remove(query);
  }
  return hasExtension(resource, extension);
}

bool validLocation(JsonVariantConst value, bool isVideo, const String& kind,
                   String& output) {
  const size_t maxLength = kind == "file" ? Playlist::kMaximumPathLength
                                           : Playlist::kMaximumUrlLength;
  if (!validText(value, maxLength, output)) {
    return false;
  }
  const char* extension = isVideo ? ".mjpeg" : ".mp3";
  if (kind == "file") {
    return safeLocalPath(output, extension);
  }
  return safeHttpUrl(output, extension);
}

void resetResult(PlaylistResult& result) {
  result.count = 0;
  result.error = PlaylistError::None;
}

}  // namespace

bool Playlist::load(fs::FS& filesystem, const char* path,
                    PlaylistResult& result) {
  resetResult(result);
  if (path == nullptr || strcmp(path, "/Videos/playlist.json") != 0) {
    result.error = PlaylistError::InvalidPath;
    return false;
  }

  File file = filesystem.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    result.error = PlaylistError::NotFound;
    return false;
  }
  const size_t fileSize = file.size();
  if (fileSize == 0 || fileSize > kMaximumBytes) {
    file.close();
    result.error = PlaylistError::TooLarge;
    return false;
  }

  char* buffer = static_cast<char*>(malloc(fileSize + 1));
  if (buffer == nullptr) {
    file.close();
    result.error = PlaylistError::ReadFailure;
    return false;
  }
  const size_t bytesRead = file.readBytes(buffer, fileSize);
  file.close();
  if (bytesRead != fileSize) {
    free(buffer);
    result.error = PlaylistError::ReadFailure;
    return false;
  }
  buffer[fileSize] = '\0';
  const bool parsed = parse(buffer, fileSize, result);
  free(buffer);
  return parsed;
}

bool Playlist::parse(const char* json, size_t length, PlaylistResult& result) {
  resetResult(result);
  if (json == nullptr || length == 0 || length > kMaximumBytes) {
    result.error = PlaylistError::TooLarge;
    return false;
  }

  JsonDocument document;
  const DeserializationError parseError = deserializeJson(
      document, json, length, DeserializationOption::NestingLimit(5));
  if (parseError || !document.is<JsonObjectConst>()) {
    result.error = PlaylistError::InvalidJson;
    return false;
  }

  JsonObjectConst root = document.as<JsonObjectConst>();
  if (root.size() != 2 || !root["version"].is<uint8_t>() ||
      root["version"].as<uint8_t>() != 1) {
    result.error = PlaylistError::WrongVersion;
    return false;
  }
  JsonArrayConst channels = root["channels"].as<JsonArrayConst>();
  if (channels.isNull() || channels.size() == 0 ||
      channels.size() > kPlaylistMaximumChannels) {
    result.error = PlaylistError::InvalidChannel;
    return false;
  }

  for (JsonObjectConst channel : channels) {
    if (channel.size() != 5 || result.count >= kPlaylistMaximumChannels) {
      result.count = 0;
      result.error = PlaylistError::InvalidChannel;
      return false;
    }
    PlaylistEntry entry;
    if (!validText(channel["id"], kMaximumIdLength, entry.id) ||
        !validId(entry.id) ||
        !validText(channel["title"], kMaximumTitleLength, entry.title) ||
        !validText(channel["source"], 4, entry.sourceKind) ||
        (entry.sourceKind != "file" && entry.sourceKind != "http") ||
        !validLocation(channel["video"], true, entry.sourceKind,
                       entry.videoLocation) ||
        !validLocation(channel["audio"], false, entry.sourceKind,
                       entry.audioLocation)) {
      result.count = 0;
      result.error = PlaylistError::InvalidChannel;
      return false;
    }
    for (size_t i = 0; i < result.count; ++i) {
      if (result.channels[i].id == entry.id) {
        result.count = 0;
        result.error = PlaylistError::DuplicateId;
        return false;
      }
    }
    result.channels[result.count++] = entry;
  }
  return true;
}
