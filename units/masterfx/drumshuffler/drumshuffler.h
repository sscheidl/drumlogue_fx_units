#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>

#include "unit.h"

class DrumShuffler {
 public:
  DrumShuffler() {
    for (uint8_t i = 0; i < kNumParams; ++i) {
      params_[i].store(kParamInit[i], std::memory_order_relaxed);
    }
  }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (desc->samplerate != kSampleRate)
      return k_unit_err_samplerate;

    if ((desc->input_channels != 2 && desc->input_channels != 4) ||
        desc->output_channels != 2)
      return k_unit_err_geometry;

    input_channels_ = desc->input_channels;
    setTempo(120U << 16);
    Reset();
    return k_unit_err_none;
  }

  inline void Teardown() {}

  inline void Reset() {
    for (size_t i = 0; i < kDelayBufferSize; ++i) {
      delay_l_[i] = 0.0f;
      delay_r_[i] = 0.0f;
    }

    for (uint8_t ch = 0; ch < 2; ++ch) {
      color_lp_[ch] = 0.0f;
      delay_smooth_[ch] = 1.0f;
    }

    write_index_ = 0U;
    lfo_phase_ = 0.0f;
    swing_counter_ = 0.0f;
    swing_state_ = -1.0f;
    swing_polarity_ = -1;
  }

  inline void Resume() {}
  inline void Suspend() {}

  fast_inline void Process(const float * in, float * out, size_t frames) {
    const float shuffle = percentToUnit(getClampedParam(kParamShuffle));
    const float swing = percentToUnit(getClampedParam(kParamSwing));
    const float width = percentToUnit(getClampedParam(kParamWidth));
    const float mix = percentToUnit(getClampedParam(kParamMix));
    const int32_t delay_mode = getClampedParam(kParamDelayMode);
    const float mod = percentToUnit(getClampedParam(kParamMod));
    const int32_t rate_mode = getClampedParam(kParamRateMode);
    const float color = percentToUnit(getClampedParam(kParamColor));

    const float note_delay = getDelayModeSamples(delay_mode);
    const float base_delay =
        computeBaseDelaySamples(delay_mode, note_delay, shuffle);
    const float swing_depth =
        computeSwingDepthSamples(delay_mode, note_delay, base_delay, shuffle,
                                 swing);
    const float stereo_depth =
        computeStereoDepthSamples(delay_mode, note_delay, base_delay, shuffle,
                                  width);
    const float mod_depth =
        computeModDepthSamples(delay_mode, note_delay, base_delay, shuffle,
                               mod);
    const float lfo_increment = kTwoPi / getRatePeriodSamples(rate_mode);
    const float swing_period = getSwingPeriodSamples(delay_mode, note_delay);

    const float * __restrict in_p = in;
    float * __restrict out_p = out;
    for (size_t i = 0; i < frames; ++i, in_p += input_channels_, out_p += 2) {
      const float dry_l = in_p[0];
      const float dry_r = in_p[1];

      delay_l_[write_index_] = dry_l;
      delay_r_[write_index_] = dry_r;

      advanceTempoState(lfo_increment, swing_period);

      const float lfo_main = sinf(lfo_phase_);
      const float lfo_quad = sinf(lfo_phase_ + kHalfPi);
      const float swing_shape = swing_state_;
      const float center_delay = base_delay + swing_depth * swing_shape;
      const float stereo_shape = 0.65f + 0.35f * fabsf(swing_shape);
      const float stereo_offset = stereo_depth * stereo_shape;
      const float mod_l = mod_depth * (0.82f * lfo_main + 0.18f * lfo_quad);
      const float mod_r = mod_depth * (0.82f * lfo_main - 0.18f * lfo_quad);

      const float target_l =
          clampFloat(center_delay - stereo_offset + mod_l, 1.0f,
                     kMaxDelaySamples);
      const float target_r =
          clampFloat(center_delay + stereo_offset + mod_r, 1.0f,
                     kMaxDelaySamples);

      delay_smooth_[0] += kDelaySmoothing * (target_l - delay_smooth_[0]);
      delay_smooth_[1] += kDelaySmoothing * (target_r - delay_smooth_[1]);

      float wet_l = readDelay(delay_l_, delay_smooth_[0]);
      float wet_r = readDelay(delay_r_, delay_smooth_[1]);

      wet_l = applyColor(wet_l, 0, color);
      wet_r = applyColor(wet_r, 1, color);

      out_p[0] = clampAudio(dry_l + (wet_l - dry_l) * mix);
      out_p[1] = clampAudio(dry_r + (wet_r - dry_r) * mix);

      write_index_ = (write_index_ + 1U) & kDelayMask;
    }
  }

  inline void setParameter(uint8_t index, int32_t value) {
    if (index >= kNumParams)
      return;

    params_[index].store(clampInt(value, kParamMin[index], kParamMax[index]),
                         std::memory_order_relaxed);
  }

  inline int32_t getParameterValue(uint8_t index) const {
    if (index >= kNumParams)
      return 0;
    return params_[index].load(std::memory_order_relaxed);
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    switch (index) {
      case kParamDelayMode:
        return kDelayNames[clampInt(value, 0, 3)];
      case kParamRateMode:
        return kRateNames[clampInt(value, 0, 3)];
      default:
        return nullptr;
    }
  }

  inline const uint8_t * getParameterBmpValue(uint8_t index,
                                              int32_t value) const {
    (void)index;
    (void)value;
    return nullptr;
  }

  inline void setTempo(uint32_t tempo) {
    const float integer = static_cast<float>(tempo >> 16);
    const float frac = static_cast<float>(tempo & 0xFFFFU) / 65536.0f;
    const float bpm = clampFloat(integer + frac, 20.0f, 300.0f);
    bpm_ = bpm;
    samples_per_beat_ =
        static_cast<float>(kSampleRate) * 60.0f / clampFloat(bpm_, 20.0f, 300.0f);
  }

  inline void LoadPreset(uint8_t idx) { (void)idx; }
  inline uint8_t getPresetIndex() const { return 0; }

  static inline const char * getPresetName(uint8_t idx) {
    (void)idx;
    return nullptr;
  }

 private:
  enum ParamIndex : uint8_t {
    kParamShuffle = 0,
    kParamSwing,
    kParamWidth,
    kParamMix,
    kParamDelayMode,
    kParamMod,
    kParamRateMode,
    kParamColor,
    kNumParams
  };

  static constexpr uint32_t kSampleRate = 48000U;
  static constexpr size_t kDelayBufferSize = 65536U;
  static constexpr size_t kDelayMask = kDelayBufferSize - 1U;
  static constexpr float kMaxDelaySamples =
      static_cast<float>(kDelayBufferSize - 4U);
  static constexpr float kFreeDelayMaxSamples =
      static_cast<float>(kSampleRate) * 0.03f;
  static constexpr float kPi = 3.14159265358979323846f;
  static constexpr float kTwoPi = 6.28318530717958647692f;
  static constexpr float kHalfPi = 1.57079632679489661923f;
  static constexpr float kDelaySmoothing = 0.0045f;
  static constexpr float kSwingSmoothing = 0.0035f;

  static const int32_t kParamMin[kNumParams];
  static const int32_t kParamMax[kNumParams];
  static const int32_t kParamInit[kNumParams];
  static const char * const kDelayNames[4];
  static const char * const kRateNames[4];

  inline int32_t getClampedParam(uint8_t index) const {
    return clampInt(params_[index].load(std::memory_order_relaxed),
                    kParamMin[index], kParamMax[index]);
  }

  static inline int32_t clampInt(int32_t value, int32_t lo, int32_t hi) {
    return value < lo ? lo : (value > hi ? hi : value);
  }

  static inline float clampFloat(float value, float lo, float hi) {
    return value < lo ? lo : (value > hi ? hi : value);
  }

  static inline float clampAudio(float value) {
    return clampFloat(value, -1.2f, 1.2f);
  }

  static inline float percentToUnit(int32_t value) {
    return clampFloat(static_cast<float>(value) * 0.01f, 0.0f, 1.0f);
  }

  static inline float wrapPhase(float phase) {
    while (phase >= kTwoPi)
      phase -= kTwoPi;
    while (phase < 0.0f)
      phase += kTwoPi;
    return phase;
  }

  inline float getDelayModeSamples(int32_t mode) const {
    switch (mode) {
      case 0:
        return samples_per_beat_ * 0.0625f;
      case 1:
        return samples_per_beat_ * 0.125f;
      case 2:
        return samples_per_beat_ * 0.25f;
      case 3:
      default:
        return 0.0f;
    }
  }

  inline float getRatePeriodSamples(int32_t mode) const {
    switch (mode) {
      case 0:
        return samples_per_beat_ * 4.0f;
      case 1:
        return samples_per_beat_ * 2.0f;
      case 2:
        return samples_per_beat_;
      case 3:
      default:
        return samples_per_beat_ * 0.5f;
    }
  }

  inline float getSwingPeriodSamples(int32_t delay_mode,
                                     float note_delay) const {
    if (delay_mode == 3 || note_delay <= 1.0f)
      return samples_per_beat_ * 0.5f;

    return clampFloat(note_delay * 2.0f, samples_per_beat_ * 0.125f,
                      samples_per_beat_);
  }

  static inline float computeBaseDelaySamples(int32_t delay_mode,
                                              float note_delay,
                                              float shuffle) {
    if (delay_mode == 3)
      return 1.0f + kFreeDelayMaxSamples * shuffle;

    return note_delay * (0.10f + 0.90f * shuffle);
  }

  static inline float computeSwingDepthSamples(int32_t delay_mode,
                                               float note_delay,
                                               float base_delay,
                                               float shuffle, float swing) {
    if (swing <= 0.0001f)
      return 0.0f;

    if (delay_mode == 3) {
      return (8.0f + base_delay * 0.55f) * swing;
    }

    const float intensity = 0.35f + 0.65f * shuffle;
    return note_delay * (0.04f + 0.28f * intensity) * swing;
  }

  static inline float computeStereoDepthSamples(int32_t delay_mode,
                                                float note_delay,
                                                float base_delay,
                                                float shuffle,
                                                float width) {
    if (width <= 0.0001f)
      return 0.0f;

    if (delay_mode == 3)
      return base_delay * 0.70f * width;

    const float intensity = 0.25f + 0.75f * shuffle;
    return note_delay * (0.06f + 0.18f * intensity) * width;
  }

  static inline float computeModDepthSamples(int32_t delay_mode,
                                             float note_delay,
                                             float base_delay, float shuffle,
                                             float mod) {
    if (mod <= 0.0001f)
      return 0.0f;

    if (delay_mode == 3)
      return (4.0f + base_delay * 0.35f) * mod;

    const float intensity = 0.25f + 0.75f * shuffle;
    return note_delay * (0.015f + 0.12f * intensity) * mod;
  }

  inline void advanceTempoState(float lfo_increment, float swing_period) {
    lfo_phase_ = wrapPhase(lfo_phase_ + lfo_increment);

    swing_counter_ += 1.0f;
    if (swing_counter_ >= swing_period) {
      swing_counter_ -= swing_period;
      swing_polarity_ = -swing_polarity_;
    }

    const float target = static_cast<float>(swing_polarity_);
    swing_state_ += kSwingSmoothing * (target - swing_state_);
  }

  inline float readDelay(const float * buffer, float delay_samples) const {
    const float delay = clampFloat(delay_samples, 1.0f, kMaxDelaySamples);
    float read_pos = static_cast<float>(write_index_) - delay;
    while (read_pos < 0.0f)
      read_pos += static_cast<float>(kDelayBufferSize);

    const size_t index_a =
        static_cast<size_t>(static_cast<uint32_t>(read_pos)) & kDelayMask;
    const size_t index_b = (index_a - 1U) & kDelayMask;
    const float frac = read_pos - floorf(read_pos);

    return buffer[index_a] * (1.0f - frac) + buffer[index_b] * frac;
  }

  inline float applyColor(float x, uint8_t ch, float color) {
    const float alpha = 0.04f + 0.34f * color;
    color_lp_[ch] += alpha * (x - color_lp_[ch]);
    const float low = color_lp_[ch];
    const float high = x - low;

    if (color < 0.5f) {
      const float dark = (0.5f - color) * 2.0f;
      return low + high * (0.55f - 0.45f * dark);
    }

    const float bright = (color - 0.5f) * 2.0f;
    return x + high * (0.42f * bright);
  }

  std::atomic<int32_t> params_[kNumParams];
  float delay_l_[kDelayBufferSize] = {};
  float delay_r_[kDelayBufferSize] = {};
  float color_lp_[2] = {0.0f, 0.0f};
  float delay_smooth_[2] = {1.0f, 1.0f};
  size_t write_index_ = 0U;
  float bpm_ = 120.0f;
  float samples_per_beat_ = 24000.0f;
  float lfo_phase_ = 0.0f;
  float swing_counter_ = 0.0f;
  float swing_state_ = -1.0f;
  int32_t swing_polarity_ = -1;
  uint8_t input_channels_ = 4;
};

const int32_t DrumShuffler::kParamMin[8] = {0, 0, 0, 0, 0, 0, 0, 0};
const int32_t DrumShuffler::kParamMax[8] = {100, 100, 100, 100, 3, 100, 3, 100};
const int32_t DrumShuffler::kParamInit[8] = {30, 20, 60, 40, 2, 30, 2, 50};
const char * const DrumShuffler::kDelayNames[4] = {"1/64", "1/32", "1/16",
                                                   "FREE"};
const char * const DrumShuffler::kRateNames[4] = {"1/1", "1/2", "1/4", "1/8"};
