#ifndef AIPI_LITE_MEDIA_CATALOG_H
#define AIPI_LITE_MEDIA_CATALOG_H

#include <Arduino.h>
#include <FS.h>

constexpr size_t kMediaCatalogMaximumPairs = 64;

struct MediaPair {
  String title;
  String channelId;
  String videoPath;
  String audioPath;
  uint32_t durationMs;
};

enum class CatalogError : uint8_t {
  None,
  VideosDirectoryMissing,
  Empty,
  CapacityExceeded,
};

struct CatalogResult {
  MediaPair pairs[kMediaCatalogMaximumPairs];
  size_t count;
  CatalogError error;
};

class MediaCatalog {
 public:
  static bool scan(fs::FS& filesystem, CatalogResult& result);
};

#endif  // AIPI_LITE_MEDIA_CATALOG_H
