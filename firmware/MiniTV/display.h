#ifndef AIPI_LITE_DISPLAY_H
#define AIPI_LITE_DISPLAY_H

#include <stdint.h>

class Arduino_GFX;

class Display {
 public:
  Display();
  bool begin();
  void showStatus(const char* title, const char* detail);
  void setBrightness(uint8_t percent);

 private:
  Arduino_GFX* graphics_;
  bool initialized_;
};

#endif  // AIPI_LITE_DISPLAY_H
