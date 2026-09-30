#ifndef AIPI_LITE_BUTTON_H
#define AIPI_LITE_BUTTON_H

#include <stdint.h>

enum class ButtonEvent : uint8_t {
  None,
  ShortPress,
  DoublePress,
  LongPress,
};

class Button {
 public:
  static constexpr uint32_t kDebounceMs = 50;
  static constexpr uint32_t kDoubleClickWindowMs = 350;
  static constexpr uint32_t kLongPressMs = 900;

  Button();
  bool begin();
  ButtonEvent update(uint32_t nowMs);

 private:
  bool initialized_;
  bool rawPressed_;
  bool stablePressed_;
  bool longPressSent_;
  bool shortPending_;
  uint32_t rawChangedAtMs_;
  uint32_t pressedAtMs_;
  uint32_t shortReleasedAtMs_;
};

#endif  // AIPI_LITE_BUTTON_H
