#ifndef AIPI_LITE_PLAYLIST_H
#define AIPI_LITE_PLAYLIST_H

#include <Arduino.h>
#include <FS.h>

constexpr size_t kPlaylistMaximumChannels = 32;

struct PlaylistEntry {
  String id;
  String title;
  String sourceKind;
  String videoLocation;
  String audioLocation;
};

enum class PlaylistError : uint8_t {
  None,
  InvalidPath,
  NotFound,
  TooLarge,
  ReadFailure,
  InvalidJson,
  WrongVersion,
  InvalidChannel,
  DuplicateId,
  UnsafeLocation,
};

struct PlaylistResult {
  PlaylistEntry channels[kPlaylistMaximumChannels];
  size_t count;
  PlaylistError error;
};

class Playlist {
 public:
  static constexpr size_t kMaximumBytes = 4096;
  static constexpr size_t kMaximumIdLength = 32;
  static constexpr size_t kMaximumTitleLength = 48;
  static constexpr size_t kMaximumPathLength = 160;
  static constexpr size_t kMaximumUrlLength = 512;

  static bool load(fs::FS& filesystem, const char* path,
                   PlaylistResult& result);

 private:
  static bool parse(const char* json, size_t length, PlaylistResult& result);
};

#endif  // AIPI_LITE_PLAYLIST_H
