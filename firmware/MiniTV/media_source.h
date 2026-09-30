#ifndef AIPI_LITE_MEDIA_SOURCE_H
#define AIPI_LITE_MEDIA_SOURCE_H

#include <Arduino.h>
#include <FS.h>

class MediaSource {
 public:
  static constexpr size_t kMaximumReadBytes = 4096;

  explicit MediaSource(fs::FS& filesystem);
  ~MediaSource();

  bool open(const char* path);
  int read(uint8_t* output, size_t capacity);
  int available();
  void close();

 private:
  fs::FS& filesystem_;
  File file_;
};

#endif  // AIPI_LITE_MEDIA_SOURCE_H
