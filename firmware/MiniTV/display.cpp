#include "display.h"

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "aipi_lite_profile.h"

namespace {

constexpr uint8_t kBacklightChannel = 0;
constexpr uint32_t kBacklightFrequencyHz = 5000;
constexpr uint8_t kBacklightResolutionBits = 8;

}  // namespace

Display::Display() : graphics_(nullptr), initialized_(false) {}

bool Display::begin() {
  pinMode(AipiLiteProfile::kBacklightPin, OUTPUT);
  digitalWrite(AipiLiteProfile::kBacklightPin, LOW);
  if (ledcSetup(kBacklightChannel, kBacklightFrequencyHz,
                kBacklightResolutionBits) == 0) {
    return false;
  }
  ledcAttachPin(AipiLiteProfile::kBacklightPin, kBacklightChannel);

  Arduino_DataBus* bus = new Arduino_ESP32SPI(
      AipiLiteProfile::kDisplayDcPin, AipiLiteProfile::kDisplayCsPin,
      AipiLiteProfile::kDisplaySclkPin, AipiLiteProfile::kDisplayMosiPin,
      GFX_NOT_DEFINED, AipiLiteProfile::kDisplaySpiBusId, false);
  if (bus == nullptr) {
    return false;
  }

  graphics_ = new Arduino_ST7735(
      bus, AipiLiteProfile::kDisplayResetPin,
      AipiLiteProfile::kDisplayRotation, false,
      AipiLiteProfile::kDisplayWidth, AipiLiteProfile::kDisplayHeight,
      0, 0, 0, 0, !AipiLiteProfile::kDisplayRgbOrder);
  if (graphics_ == nullptr ||
      !graphics_->begin(AipiLiteProfile::kDisplaySpiHz)) {
    graphics_ = nullptr;
    return false;
  }

  graphics_->setRotation(AipiLiteProfile::kDisplayRotation);
  graphics_->fillScreen(RGB565_BLACK);
  initialized_ = true;
  setBrightness(100);
  showStatus("MiniTV", "Ready");
  return true;
}

void Display::showStatus(const char* title, const char* detail) {
  if (!initialized_ || graphics_ == nullptr) {
    return;
  }

  graphics_->fillScreen(RGB565_BLACK);
  graphics_->setTextWrap(true);
  graphics_->setTextSize(1);
  graphics_->setTextColor(RGB565_WHITE);
  graphics_->setCursor(2, 4);
  graphics_->print("Status: ");
  graphics_->println(title != nullptr ? title : "Unknown");
  graphics_->drawFastHLine(2, 19, AipiLiteProfile::kDisplayWidth - 4,
                           RGB565_WHITE);
  graphics_->setCursor(2, 25);
  graphics_->print("Detail: ");
  graphics_->println(detail != nullptr ? detail : "None");
}

void Display::setBrightness(uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }
  const uint8_t duty = static_cast<uint8_t>((percent * 255U) / 100U);
  ledcWrite(kBacklightChannel, duty);
}
