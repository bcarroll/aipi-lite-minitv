#include "media_source.h"

#include <string.h>

namespace {

bool validMediaPath(const char* path) {
  if (path == nullptr || strncmp(path, "/Videos/", 8) != 0 ||
      strchr(path, '\\') != nullptr || strchr(path, '%') != nullptr ||
      strchr(path, '?') != nullptr || strchr(path, '#') != nullptr) {
    return false;
  }

  const size_t length = strlen(path);
  if (length < 7 || length > 160) {
    return false;
  }
  bool supportedExtension = false;
  const char* extensions[] = {".mjpeg", ".mp3"};
  for (const char* extension : extensions) {
    const size_t extensionLength = strlen(extension);
    if (length < extensionLength) {
      continue;
    }
    bool matches = true;
    for (size_t i = 0; i < extensionLength; ++i) {
      char actual = path[length - extensionLength + i];
      if (actual >= 'A' && actual <= 'Z') {
        actual = static_cast<char>(actual - 'A' + 'a');
      }
      if (actual != extension[i]) {
        matches = false;
        break;
      }
    }
    supportedExtension = supportedExtension || matches;
  }
  if (!supportedExtension) {
    return false;
  }

  const char* segment = path;
  for (const char* cursor = path;; ++cursor) {
    if (*cursor == '/' || *cursor == '\0') {
      if (cursor - segment == 2 && segment[0] == '.' && segment[1] == '.') {
        return false;
      }
      if (*cursor == '\0') {
        break;
      }
      segment = cursor + 1;
    }
  }
  return true;
}

}  // namespace

MediaSource::MediaSource(fs::FS& filesystem) : filesystem_(filesystem) {}

MediaSource::~MediaSource() { close(); }

bool MediaSource::open(const char* path) {
  close();
  if (!validMediaPath(path)) {
    return false;
  }
  file_ = filesystem_.open(path, FILE_READ);
  if (!file_ || file_.isDirectory()) {
    close();
    return false;
  }
  return true;
}

int MediaSource::read(uint8_t* output, size_t capacity) {
  if (!file_ || output == nullptr || capacity == 0) {
    return -1;
  }
  if (capacity > kMaximumReadBytes) {
    capacity = kMaximumReadBytes;
  }
  const size_t bytesRead = file_.read(output, capacity);
  return static_cast<int>(bytesRead);
}

int MediaSource::available() {
  return file_ ? file_.available() : 0;
}

void MediaSource::close() {
  if (file_) {
    file_.close();
  }
}
