#ifndef AIPI_LITE_AUDIO_CODEC_H
#define AIPI_LITE_AUDIO_CODEC_H

#include <Arduino.h>

class AudioCodec {
 public:
  static constexpr uint32_t kSupportedSampleRate = 16000;
  static constexpr size_t kMaximumPcmSamplesPerWrite = 2048;

  AudioCodec();
  bool begin(uint32_t sampleRate);
  bool writePcm(const int16_t* samples, size_t sampleCount);
  void stopPlayback();
  void setMuted(bool muted);
  void setVolume(uint8_t percent);

 private:
  bool writeRegister(uint8_t reg, uint8_t value);
  bool readRegister(uint8_t reg, uint8_t& value);
  bool writeSequence(const uint8_t* sequence, size_t pairCount);
  void setSpeakerGate(bool enabled);

  uint8_t address_;
  uint8_t volumeRegister_;
  bool initialized_;
  bool muted_;
  bool playbackActive_;
  bool speakerGateReady_;
  bool codecMuteConfirmed_;
};

#endif  // AIPI_LITE_AUDIO_CODEC_H
