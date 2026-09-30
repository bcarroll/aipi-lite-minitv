#include "button.h"

#include <Arduino.h>

#include "aipi_lite_profile.h"

Button::Button()
    : initialized_(false),
      rawPressed_(false),
      stablePressed_(false),
      longPressSent_(false),
      shortPending_(false),
      rawChangedAtMs_(0),
      pressedAtMs_(0),
      shortReleasedAtMs_(0) {}

bool Button::begin() {
  pinMode(AipiLiteProfile::kFunctionButtonPin, INPUT_PULLUP);
  rawPressed_ = digitalRead(AipiLiteProfile::kFunctionButtonPin) == LOW;
  stablePressed_ = rawPressed_;
  initialized_ = true;
  rawChangedAtMs_ = millis();
  pressedAtMs_ = rawPressed_ ? rawChangedAtMs_ : 0;
  return true;
}

ButtonEvent Button::update(uint32_t nowMs) {
  if (!initialized_) {
    return ButtonEvent::None;
  }

  const bool currentRawPressed =
      digitalRead(AipiLiteProfile::kFunctionButtonPin) == LOW;
  if (currentRawPressed != rawPressed_) {
    rawPressed_ = currentRawPressed;
    rawChangedAtMs_ = nowMs;
  }

  if (rawPressed_ != stablePressed_ &&
      static_cast<uint32_t>(nowMs - rawChangedAtMs_) >= kDebounceMs) {
    stablePressed_ = rawPressed_;
    if (stablePressed_) {
      pressedAtMs_ = nowMs;
      longPressSent_ = false;
    } else if (!longPressSent_) {
      const uint32_t heldMs = static_cast<uint32_t>(nowMs - pressedAtMs_);
      if (heldMs >= kLongPressMs) {
        longPressSent_ = true;
        shortPending_ = false;
        return ButtonEvent::LongPress;
      }
      if (shortPending_ &&
          static_cast<uint32_t>(nowMs - shortReleasedAtMs_) <=
              kDoubleClickWindowMs) {
        shortPending_ = false;
        return ButtonEvent::DoublePress;
      }
      shortPending_ = true;
      shortReleasedAtMs_ = nowMs;
    }
  }

  if (stablePressed_ && !longPressSent_ &&
      static_cast<uint32_t>(nowMs - pressedAtMs_) >= kLongPressMs) {
    longPressSent_ = true;
    shortPending_ = false;
    return ButtonEvent::LongPress;
  }

  if (!stablePressed_ && shortPending_ &&
      static_cast<uint32_t>(nowMs - shortReleasedAtMs_) >
          kDoubleClickWindowMs) {
    shortPending_ = false;
    return ButtonEvent::ShortPress;
  }

  return ButtonEvent::None;
}
