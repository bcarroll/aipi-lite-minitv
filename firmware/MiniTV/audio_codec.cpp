#include "audio_codec.h"

#include <Wire.h>
#include <driver/i2s.h>

#include "aipi_lite_profile.h"

namespace {

constexpr uint8_t kExpectedAddresses[] = {0x18, 0x19};
constexpr uint32_t kI2cFrequencyHz = 100000;
constexpr i2s_port_t kI2sPort = I2S_NUM_0;
constexpr uint8_t kDacMuteRegister = 0x31;
constexpr uint8_t kDacVolumeRegister = 0x32;
constexpr uint8_t kDacMuteMask = 0x60;
constexpr uint8_t kVolumeQuietRegister = 0x47;  // -60 dB
constexpr uint8_t kVolumeMaximumRegister = 0xBF; // 0 dB

const uint8_t kOutputRegisterSequence[][2] = {
    {0x00, 0x1F}, {0x00, 0x00},
    {0x01, 0x9F}, {0x02, 0x10}, {0x03, 0x10}, {0x04, 0x20},
    {0x05, 0x00}, {0x06, 0x03}, {0x07, 0x00}, {0x08, 0xFF},
    {0x09, 0x0C}, {0x0A, 0x0C},
    {0x32, 0xBF}, {0x12, 0x00}, {0x13, 0x10}, {0x31, 0x60},
    {0x37, 0x08}, {0x0D, 0x01}, {0x0E, 0x02}, {0x00, 0x80},
};

bool installI2s(uint32_t sampleRate) {
  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
  config.sample_rate = sampleRate;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = 0;
  config.dma_buf_count = 8;
  config.dma_buf_len = 256;
  config.use_apll = false;
  config.tx_desc_auto_clear = true;
  config.fixed_mclk = 0;
  if (i2s_driver_install(kI2sPort, &config, 0, nullptr) != ESP_OK) {
    return false;
  }

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = AipiLiteProfile::kCodecI2sBclkPin;
  pins.ws_io_num = AipiLiteProfile::kCodecI2sLrclkPin;
  pins.data_out_num = AipiLiteProfile::kCodecI2sDoutPin;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  if (i2s_set_pin(kI2sPort, &pins) != ESP_OK) {
    i2s_driver_uninstall(kI2sPort);
    return false;
  }
  return true;
}

}  // namespace

AudioCodec::AudioCodec()
    : address_(0),
      volumeRegister_(kVolumeQuietRegister),
      initialized_(false),
      muted_(true),
      playbackActive_(false),
      speakerGateReady_(false),
      codecMuteConfirmed_(true) {}

bool AudioCodec::begin(uint32_t sampleRate) {
  address_ = 0;
  initialized_ = false;
  speakerGateReady_ = false;
  playbackActive_ = false;
  muted_ = true;
  codecMuteConfirmed_ = true;

  pinMode(AipiLiteProfile::kSpeakerEnablePin, OUTPUT);
  setSpeakerGate(false);
  if (sampleRate != kSupportedSampleRate) {
    return false;
  }

  Wire.begin(AipiLiteProfile::kCodecI2cSdaPin,
             AipiLiteProfile::kCodecI2cSclPin, kI2cFrequencyHz);
  for (uint8_t candidate : kExpectedAddresses) {
    Wire.beginTransmission(candidate);
    if (Wire.endTransmission() == 0) {
      address_ = candidate;
      break;
    }
  }
  if (address_ == 0 || !installI2s(sampleRate)) {
    return false;
  }

  if (!writeSequence(&kOutputRegisterSequence[0][0],
                     sizeof(kOutputRegisterSequence) /
                         sizeof(kOutputRegisterSequence[0]))) {
    i2s_driver_uninstall(kI2sPort);
    return false;
  }
  volumeRegister_ = kVolumeMaximumRegister;
  speakerGateReady_ = true;
  initialized_ = true;
  setVolume(50);
  setMuted(true);
  return true;
}

bool AudioCodec::writePcm(const int16_t* samples, size_t sampleCount) {
  if (!initialized_ || samples == nullptr || sampleCount == 0 ||
      sampleCount > kMaximumPcmSamplesPerWrite) {
    return false;
  }
  playbackActive_ = true;
  setSpeakerGate(!muted_ && codecMuteConfirmed_);
  const size_t byteCount = sampleCount * sizeof(int16_t);
  size_t written = 0;
  const esp_err_t result = i2s_write(kI2sPort, samples, byteCount, &written,
                                    pdMS_TO_TICKS(500));
  if (result != ESP_OK || written != byteCount) {
    stopPlayback();
    return false;
  }
  return true;
}

void AudioCodec::stopPlayback() {
  playbackActive_ = false;
  setSpeakerGate(false);
  if (initialized_) {
    i2s_zero_dma_buffer(kI2sPort);
  }
}

void AudioCodec::setMuted(bool muted) {
  muted_ = muted;
  if (!initialized_) {
    setSpeakerGate(false);
    return;
  }
  uint8_t current = 0;
  if (!readRegister(kDacMuteRegister, current)) {
    codecMuteConfirmed_ = false;
    setSpeakerGate(false);
    return;
  }
  current &= static_cast<uint8_t>(~kDacMuteMask);
  if (muted_) {
    current |= kDacMuteMask;
  }
  if (!writeRegister(kDacMuteRegister, current)) {
    codecMuteConfirmed_ = false;
    setSpeakerGate(false);
    return;
  }
  codecMuteConfirmed_ = true;
  setSpeakerGate(!muted_ && playbackActive_ && codecMuteConfirmed_);
}

void AudioCodec::setVolume(uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }
  const uint16_t range = kVolumeMaximumRegister - kVolumeQuietRegister;
  volumeRegister_ = static_cast<uint8_t>(
      kVolumeQuietRegister + (range * percent + 50U) / 100U);
  if (initialized_) {
    writeRegister(kDacVolumeRegister, volumeRegister_);
  }
  if (percent == 0) {
    setMuted(true);
  }
}

bool AudioCodec::writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address_);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool AudioCodec::readRegister(uint8_t reg, uint8_t& value) {
  Wire.beginTransmission(address_);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(address_, 1) != 1) {
    return false;
  }
  value = static_cast<uint8_t>(Wire.read());
  return true;
}

bool AudioCodec::writeSequence(const uint8_t* sequence, size_t pairCount) {
  for (size_t i = 0; i < pairCount; ++i) {
    if (!writeRegister(sequence[i * 2], sequence[i * 2 + 1])) {
      setSpeakerGate(false);
      return false;
    }
    if (sequence[i * 2] == 0x00) {
      delay(5);
    }
  }
  return true;
}

void AudioCodec::setSpeakerGate(bool enabled) {
  const bool outputEnabled = enabled && speakerGateReady_;
  digitalWrite(AipiLiteProfile::kSpeakerEnablePin,
               outputEnabled ? HIGH : LOW);
}
