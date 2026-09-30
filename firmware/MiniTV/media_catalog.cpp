#include "media_catalog.h"

namespace {

String baseName(const String& path) {
  const int slash = path.lastIndexOf('/');
  return slash < 0 ? path : path.substring(slash + 1);
}

bool validChannelFolder(const String& name) {
  if (name == "random") {
    return true;
  }
  if (name.length() == 0 || name.length() > 8) {
    return false;
  }
  for (size_t i = 0; i < name.length(); ++i) {
    if (name[i] < '0' || name[i] > '9') {
      return false;
    }
  }
  return true;
}

bool isMjpegName(const String& name) {
  return name.endsWith(".mjpeg") || name.endsWith(".MJPEG");
}

void resetResult(CatalogResult& result) {
  result.count = 0;
  result.error = CatalogError::None;
}

}  // namespace

bool MediaCatalog::scan(fs::FS& filesystem, CatalogResult& result) {
  resetResult(result);
  File root = filesystem.open("/Videos", FILE_READ);
  if (!root || !root.isDirectory()) {
    result.error = CatalogError::VideosDirectoryMissing;
    return false;
  }

  while (true) {
    File folder = root.openNextFile();
    if (!folder) {
      break;
    }
    const bool isDirectory = folder.isDirectory();
    const String folderName = baseName(folder.name());
    folder.close();
    if (!isDirectory || !validChannelFolder(folderName)) {
      continue;
    }

    const String folderPath = String("/Videos/") + folderName;
    File contents = filesystem.open(folderPath, FILE_READ);
    if (!contents || !contents.isDirectory()) {
      continue;
    }

    while (true) {
      File media = contents.openNextFile();
      if (!media) {
        break;
      }
      const bool isNestedDirectory = media.isDirectory();
      const String videoName = baseName(media.name());
      media.close();
      if (isNestedDirectory || !isMjpegName(videoName) ||
          videoName.length() <= 6) {
        continue;
      }
      const String title = videoName.substring(0, videoName.length() - 6);
      if (title.length() == 0 || title.length() > 48) {
        continue;
      }
      const String videoPath = folderPath + "/" + videoName;
      const String audioPath = folderPath + "/" + title + ".mp3";
      if (!filesystem.exists(audioPath)) {
        continue;
      }
      if (result.count >= kMediaCatalogMaximumPairs) {
        root.close();
        contents.close();
        result.count = 0;
        result.error = CatalogError::CapacityExceeded;
        return false;
      }
      MediaPair& pair = result.pairs[result.count++];
      pair.title = title;
      pair.channelId = folderName;
      pair.videoPath = videoPath;
      pair.audioPath = audioPath;
      pair.durationMs = 0;
    }
    contents.close();
  }
  root.close();
  if (result.count == 0) {
    result.error = CatalogError::Empty;
  }
  return true;
}
