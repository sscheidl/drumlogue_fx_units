#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>

#include "unit.h"

class GrainDrift {
 public:
  GrainDrift() {
    for (uint8_t i = 0; i < kNumParams; ++i) {
      params_[i].store(kParamInit[i], std::memory_order_relaxed);
    }
  }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (desc->samplerate != kSampleRate)
      return k_unit_err_samplerate;

    if (desc->input_channels != 2 || desc->output_channels != 2)
      return k_unit_err_geometry;

    setTempo(120U << 16);
    Reset();
    return k_unit_err_none;
  }

  inline void Teardown() {}

  inline void Reset() {
    for (size_t i = 0; i < kBufferSize; ++i) {
      buffer_l_[i] = 0;
      buffer_r_[i] = 0;
    }

    for (uint8_t i = 0; i < kMaxGrains; ++i) {
      grains_[i].active = false;
      grains_[i].read_pos = 0.0f;
      grains_[i].increment = 1.0f;
      grains_[i].age = 0.0f;
      grains_[i].duration = 1.0f;
      grains_[i].gain = 0.0f;
      grains_[i].gain_l = 0.7071f;
      grains_[i].gain_r = 0.7071f;
    }

    color_lp_[0] = 0.0f;
    color_lp_[1] = 0.0f;
    lofi_hold_[0] = 0.0f;
    lofi_hold_[1] = 0.0f;
    lofi_counter_ = 0;
    write_index_ = 0U;
    scheduler_counter_ = 0.0f;
    density_accum_ = 0.0f;
    rng_ = 0x12345678U;
  }

  inline void Resume() {}
  inline void Suspend() {}

  fast_inline void Process(const float * in, float * out, size_t frames) {
    const float density = percentToUnit(getClampedParam(kParamDensity));
    const float grain_len_pct =
        percentToUnit(getClampedParam(kParamGrainLength));
    const int32_t rhythm = getClampedParam(kParamRhythm);
    const float mix = percentToUnit(getClampedParam(kParamMix));
    const float pitch_semitones =
        static_cast<float>(getClampedParam(kParamPitch) - 12);
    const float spread = percentToUnit(getClampedParam(kParamSpread));
    const float pan = percentToUnit(getClampedParam(kParamPan));
    const int32_t color = getClampedParam(kParamColor);
    const float chaos = percentToUnit(getClampedParam(kParamChaos));

    const float step_samples = getRhythmStepSamples(rhythm);
    const float grains_per_beat = density * 5.0f;

    const float * __restrict in_p = in;
    float * __restrict out_p = out;
    for (size_t i = 0; i < frames; ++i, in_p += 2, out_p += 2) {
      const float dry_l = clampFloat(in_p[0], -1.0f, 1.0f);
      const float dry_r = clampFloat(in_p[1], -1.0f, 1.0f);

      buffer_l_[write_index_] = floatToI16(dry_l);
      buffer_r_[write_index_] = floatToI16(dry_r);

      scheduler_counter_ += 1.0f;
      while (scheduler_counter_ >= step_samples) {
        scheduler_counter_ -= step_samples;
        scheduleStep(grains_per_beat, grain_len_pct, rhythm, pitch_semitones,
                     spread, pan, chaos, step_samples);
      }

      float wet_l = 0.0f;
      float wet_r = 0.0f;
      renderGrains(wet_l, wet_r);
      applyColorStereo(wet_l, wet_r, color);

      out_p[0] = clampAudio(dry_l * (1.0f - mix) + wet_l * mix);
      out_p[1] = clampAudio(dry_r * (1.0f - mix) + wet_r * mix);

      write_index_ = (write_index_ + 1U) & kBufferMask;
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
      case kParamRhythm:
        return kRhythmNames[clampInt(value, 0, 3)];
      case kParamColor:
        return kColorNames[clampInt(value, 0, 2)];
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
    bpm_ = clampFloat(integer + frac, 20.0f, 300.0f);
    samples_per_beat_ = static_cast<float>(kSampleRate) * 60.0f / bpm_;
  }

  inline void LoadPreset(uint8_t idx) { (void)idx; }
  inline uint8_t getPresetIndex() const { return 0; }

  static inline const char * getPresetName(uint8_t idx) {
    (void)idx;
    return nullptr;
  }

 private:
  enum ParamIndex : uint8_t {
    kParamDensity = 0,
    kParamGrainLength,
    kParamRhythm,
    kParamMix,
    kParamPitch,
    kParamSpread,
    kParamPan,
    kParamColor,
    kParamChaos,
    kNumParams
  };

  struct Grain {
    bool active = false;
    float read_pos = 0.0f;
    float increment = 1.0f;
    float age = 0.0f;
    float duration = 1.0f;
    float gain = 0.0f;
    float gain_l = 0.7071f;
    float gain_r = 0.7071f;
  };

  static constexpr uint32_t kSampleRate = 48000U;
  static constexpr size_t kBufferSize = 32768U;
  static constexpr size_t kBufferMask = kBufferSize - 1U;
  static constexpr uint8_t kMaxGrains = 12U;
  static constexpr float kPi = 3.14159265358979323846f;
  static constexpr float kTwoPi = 6.28318530717958647692f;
  static constexpr float kMaxLookback =
      static_cast<float>(kBufferSize - 512U);

  static const int32_t kParamMin[kNumParams];
  static const int32_t kParamMax[kNumParams];
  static const int32_t kParamInit[kNumParams];
  static const char * const kRhythmNames[4];
  static const char * const kColorNames[3];

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

  static inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
  }

  static inline int16_t floatToI16(float x) {
    const float clipped = clampFloat(x, -1.0f, 1.0f);
    return static_cast<int16_t>(clipped * 32767.0f);
  }

  static inline float i16ToFloat(int16_t x) {
    return static_cast<float>(x) * (1.0f / 32768.0f);
  }

  static inline float semitonesToRatio(float semitones) {
    return exp2f(semitones / 12.0f);
  }

  inline float nextRand01() {
    rng_ = 1664525U * rng_ + 1013904223U;
    return static_cast<float>((rng_ >> 8) & 0x00FFFFFFU) / 16777215.0f;
  }

  inline float nextRandSigned() { return nextRand01() * 2.0f - 1.0f; }

  inline float wrapPosition(float position) const {
    while (position < 0.0f)
      position += static_cast<float>(kBufferSize);
    while (position >= static_cast<float>(kBufferSize))
      position -= static_cast<float>(kBufferSize);
    return position;
  }

  inline float getRhythmStepSamples(int32_t rhythm) const {
    switch (rhythm) {
      case 0:
        return samples_per_beat_ * 0.5f;
      case 1:
        return samples_per_beat_ * 0.25f;
      case 2:
        return samples_per_beat_ / 3.0f;
      case 3:
      default:
        return samples_per_beat_ * 0.25f;
    }
  }

  inline float getStepsPerBeat(float step_samples) const {
    return clampFloat(samples_per_beat_ / clampFloat(step_samples, 1.0f,
                                                     samples_per_beat_ * 2.0f),
                      1.0f, 8.0f);
  }

  static inline float mapGrainLengthSamples(float grain_len_pct) {
    const float shaped = powf(clampFloat(grain_len_pct, 0.0f, 1.0f), 1.25f);
    return 240.0f + shaped * 5520.0f;
  }

  inline void scheduleStep(float grains_per_beat, float grain_len_pct,
                           int32_t rhythm, float pitch_semitones, float spread,
                           float pan, float chaos, float step_samples) {
    const float steps_per_beat = getStepsPerBeat(step_samples);
    float density_variation = 1.0f + nextRandSigned() * chaos * 0.3f;
    if (rhythm == 3) {
      density_variation *= 1.15f + 0.25f * chaos;
    }

    density_accum_ +=
        clampFloat((grains_per_beat / steps_per_beat) * density_variation, 0.0f,
                   3.5f);

    while (density_accum_ >= 1.0f) {
      spawnGrain(grain_len_pct, rhythm, pitch_semitones, spread, pan, chaos,
                 step_samples, grains_per_beat);
      density_accum_ -= 1.0f;
    }
  }

  inline void spawnGrain(float grain_len_pct, int32_t rhythm,
                         float pitch_semitones, float spread, float pan,
                         float chaos, float step_samples,
                         float grains_per_beat) {
    const uint8_t slot = findVoiceSlot();
    Grain & voice = grains_[slot];

    const float grain_length = mapGrainLengthSamples(grain_len_pct);
    const float lookback_base =
        clampFloat(step_samples * (0.45f + 0.55f * spread) + grain_length * 0.5f,
                   grain_length + 32.0f, kMaxLookback);
    const float timing_depth =
        grain_length * spread * (0.18f + 0.52f * chaos);
    float lookback = lookback_base + nextRandSigned() * timing_depth;

    if (rhythm == 3) {
      lookback += nextRandSigned() * step_samples * (0.15f + 0.35f * chaos);
    }
    lookback = clampFloat(lookback, grain_length + 32.0f, kMaxLookback);

    const float random_pitch =
        nextRandSigned() * spread * chaos * 4.0f +
        nextRandSigned() * spread * 0.75f;
    const float increment =
        semitonesToRatio(clampFloat(pitch_semitones + random_pitch, -18.0f,
                                    18.0f));

    const float stereo =
        clampFloat(nextRandSigned() * pan * (0.35f + 0.65f * chaos), -1.0f,
                   1.0f);
    const float gain_l = sqrtf(0.5f * (1.0f - stereo));
    const float gain_r = sqrtf(0.5f * (1.0f + stereo));
    const float level =
        0.24f * (0.75f + 0.25f * spread) /
        sqrtf(1.0f + clampFloat(grains_per_beat, 0.0f, 6.0f) * 0.35f);

    voice.active = true;
    voice.read_pos =
        wrapPosition(static_cast<float>(write_index_) - lookback);
    voice.increment = increment;
    voice.age = 0.0f;
    voice.duration = grain_length;
    voice.gain = level;
    voice.gain_l = gain_l;
    voice.gain_r = gain_r;
  }

  inline uint8_t findVoiceSlot() const {
    uint8_t oldest = 0;
    float oldest_phase = -1.0f;

    for (uint8_t i = 0; i < kMaxGrains; ++i) {
      if (!grains_[i].active)
        return i;

      const float phase = grains_[i].age / grains_[i].duration;
      if (phase > oldest_phase) {
        oldest_phase = phase;
        oldest = i;
      }
    }

    return oldest;
  }

  inline float readBuffer(const int16_t * buffer, float position) const {
    const float wrapped = wrapPosition(position);
    const size_t index_a = static_cast<size_t>(wrapped) & kBufferMask;
    const size_t index_b = (index_a + 1U) & kBufferMask;
    const float frac = wrapped - floorf(wrapped);

    return lerp(i16ToFloat(buffer[index_a]), i16ToFloat(buffer[index_b]), frac);
  }

  inline void renderGrains(float &wet_l, float &wet_r) {
    int32_t active_count = 0;

    for (uint8_t i = 0; i < kMaxGrains; ++i) {
      Grain & voice = grains_[i];
      if (!voice.active)
        continue;

      const float phase = voice.age / voice.duration;
      if (phase >= 1.0f) {
        voice.active = false;
        continue;
      }

      const float env = sinf(kPi * phase);
      const float src_l = readBuffer(buffer_l_, voice.read_pos);
      const float src_r = readBuffer(buffer_r_, voice.read_pos);

      wet_l += src_l * env * voice.gain * voice.gain_l;
      wet_r += src_r * env * voice.gain * voice.gain_r;

      voice.read_pos = wrapPosition(voice.read_pos + voice.increment);
      voice.age += 1.0f;
      ++active_count;
    }

    if (active_count > 1) {
      const float norm = 1.0f / sqrtf(static_cast<float>(active_count));
      wet_l *= norm;
      wet_r *= norm;
    }
  }

  inline void applyColorStereo(float &l, float &r, int32_t mode) {
    switch (mode) {
      case 1:
        color_lp_[0] += 0.22f * (l - color_lp_[0]);
        color_lp_[1] += 0.22f * (r - color_lp_[1]);
        if (lofi_counter_ <= 0) {
          lofi_hold_[0] = quantize(color_lp_[0], 9);
          lofi_hold_[1] = quantize(color_lp_[1], 9);
          lofi_counter_ = 2;
        }
        --lofi_counter_;
        l = lofi_hold_[0];
        r = lofi_hold_[1];
        break;
      case 2:
        color_lp_[0] += 0.08f * (l - color_lp_[0]);
        color_lp_[1] += 0.08f * (r - color_lp_[1]);
        l = color_lp_[0];
        r = color_lp_[1];
        break;
      case 0:
      default:
        color_lp_[0] = l;
        color_lp_[1] = r;
        break;
    }
  }

  static inline float quantize(float x, int32_t bits) {
    const int32_t clamped_bits = clampInt(bits, 4, 16);
    const float levels = static_cast<float>(1U << clamped_bits);
    const float scaled = x * levels;
    return (scaled >= 0.0f ? floorf(scaled + 0.5f) : ceilf(scaled - 0.5f)) /
           levels;
  }

  std::atomic<int32_t> params_[kNumParams];
  int16_t buffer_l_[kBufferSize] = {};
  int16_t buffer_r_[kBufferSize] = {};
  Grain grains_[kMaxGrains];
  float color_lp_[2] = {0.0f, 0.0f};
  float lofi_hold_[2] = {0.0f, 0.0f};
  int32_t lofi_counter_ = 0;
  size_t write_index_ = 0U;
  float bpm_ = 120.0f;
  float samples_per_beat_ = 24000.0f;
  float scheduler_counter_ = 0.0f;
  float density_accum_ = 0.0f;
  uint32_t rng_ = 0x12345678U;
};

const int32_t GrainDrift::kParamMin[9] = {0, 10, 0, 0, 0, 0, 0, 0, 0};
const int32_t GrainDrift::kParamMax[9] = {100, 100, 3, 100, 24, 100, 100, 2,
                                          100};
const int32_t GrainDrift::kParamInit[9] = {40, 40, 1, 30, 12, 25, 60, 0, 20};
const char * const GrainDrift::kRhythmNames[4] = {"1/8", "1/16", "1/8T",
                                                  "FREE"};
const char * const GrainDrift::kColorNames[3] = {"CLEAN", "LOFI", "FILTER"};
