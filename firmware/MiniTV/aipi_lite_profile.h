#ifndef AIPI_LITE_PROFILE_H
#define AIPI_LITE_PROFILE_H

#include <stdint.h>

namespace AipiLiteProfile {

constexpr char kProfileName[] = "AIPI_LITE_XY006PL01";

constexpr uint16_t kDisplayWidth = 128;
constexpr uint16_t kDisplayHeight = 128;
constexpr uint8_t kDisplayRotation = 1;
constexpr bool kDisplayRgbOrder = true;
constexpr uint8_t kDisplaySpiBusId = 1;
constexpr uint32_t kDisplaySpiHz = 20000000;

constexpr int8_t kBacklightPin = 3;
constexpr int8_t kCodecI2cSclPin = 4;
constexpr int8_t kCodecI2cSdaPin = 5;
constexpr int8_t kCodecI2sMclkPin = 6;
constexpr int8_t kDisplayDcPin = 7;
constexpr int8_t kSpeakerEnablePin = 9;
constexpr int8_t kCodecI2sDoutPin = 11;
constexpr int8_t kCodecI2sLrclkPin = 12;
constexpr int8_t kCodecI2sDinPin = 13;
constexpr int8_t kCodecI2sBclkPin = 14;
constexpr int8_t kDisplayCsPin = 15;
constexpr int8_t kDisplaySclkPin = 16;
constexpr int8_t kDisplayMosiPin = 17;
constexpr int8_t kDisplayResetPin = 18;
constexpr int8_t kFunctionButtonPin = 42;

// GPIO10 controls board power and is deliberately not declared as an output.

}  // namespace AipiLiteProfile

#endif  // AIPI_LITE_PROFILE_H
