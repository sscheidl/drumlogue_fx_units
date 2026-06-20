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
    out_dc_x_[0] = 0.0f;
    out_dc_x_[1] = 0.0f;
    out_dc_y_[0] = 0.0f;
    out_dc_y_[1] = 0.0f;
    lofi_counter_ = 0;
    write_index_ = 0U;
    scheduler_counter_ = 0.0f;
    density_accum_ = 0.0f;
    sm_density_ = percentToUnit(getClampedParam(kParamDensity));
    sm_grain_len_ = percentToUnit(getClampedParam(kParamGrainLength));
    sm_pitch_ = static_cast<float>(getClampedParam(kParamPitch) - 24);
    last_rhythm_ = getClampedParam(kParamRhythm);
    rng_ = 0x12345678U;
  }

  inline void Resume() {}
  inline void Suspend() {}

  fast_inline void Process(const float * in, float * out, size_t frames) {
    const float density_target = percentToUnit(getClampedParam(kParamDensity));
    const float grain_len_target = percentToUnit(getClampedParam(kParamGrainLength));
    const int32_t rhythm = getClampedParam(kParamRhythm);
    const float mix = percentToUnit(getClampedParam(kParamMix));
    const float pitch_target = static_cast<float>(getClampedParam(kParamPitch) - 24);
    const float spread = percentToUnit(getClampedParam(kParamSpread));
    const float pan = percentToUnit(getClampedParam(kParamPan));
    const int32_t color = getClampedParam(kParamColor);
    const float chaos = percentToUnit(getClampedParam(kParamChaos));
    const float block_smooth =
        clampFloat(static_cast<float>(frames) * (1.0f / (0.035f * kSampleRate)),
                   0.006f, 0.22f);

    if (rhythm != last_rhythm_) {
      scheduler_counter_ = 0.0f;
      density_accum_ = 0.0f;
      clearGrains();
      last_rhythm_ = rhythm;
    }

    sm_density_ += (density_target - sm_density_) * block_smooth;
    sm_grain_len_ += (grain_len_target - sm_grain_len_) * block_smooth;
    sm_pitch_ += (pitch_target - sm_pitch_) * block_smooth;

    const float density = clampFloat(sm_density_, 0.0f, 1.0f);
    const float grain_len_pct = clampFloat(sm_grain_len_, 0.0f, 1.0f);
    const float pitch_semitones = clampFloat(sm_pitch_, -24.0f, 12.0f);

    const float step_samples = getRhythmStepSamples(rhythm);
    const float density_curve = density * density * (3.0f - 2.0f * density);
    const float grains_per_beat = 1.25f + density_curve * 24.0f;
    const uint8_t voice_budget = getVoiceBudget(grain_len_pct, density, rhythm);
    const uint8_t spawn_limit = getSpawnLimit(grain_len_pct, density, rhythm);
    const float wet_drive = 1.25f + 1.65f * density + 0.85f * chaos;
    const float wet_level = mix * (1.12f + 1.58f * density);
    const float dry_level = 1.0f - 0.92f * mix;

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
        scheduleStep(grains_per_beat, density, grain_len_pct, rhythm,
                     pitch_semitones, spread, pan, chaos, step_samples,
                     voice_budget, spawn_limit);
      }

      float wet_l = 0.0f;
      float wet_r = 0.0f;
      renderGrains(wet_l, wet_r, voice_budget);
      renderGhostRepeats(wet_l, wet_r, step_samples, density, spread, pan,
                         chaos, rhythm);
      applyColorStereo(wet_l, wet_r, color, chaos);

      wet_l = softClip(wet_l * wet_drive);
      wet_r = softClip(wet_r * wet_drive);

      float out_l = dry_l * dry_level + wet_l * wet_level;
      float out_r = dry_r * dry_level + wet_r * wet_level;
      out_l = processDcBlock(out_l, 0);
      out_r = processDcBlock(out_r, 1);

      out_p[0] = outputLimit(safeAudio(out_l, dry_l));
      out_p[1] = outputLimit(safeAudio(out_r, dry_r));

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
        return kRhythmNames[clampInt(value, 0, kNumRhythms - 1)];
      case kParamPitch:
        return kPitchNames[clampInt(value, 0, 36)];
      case kParamColor:
        return kColorNames[clampInt(value, 0, kNumColors - 1)];
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
  static constexpr uint8_t kMaxGrains = 16U;
  static constexpr uint8_t kMinVoiceBudget = 8U;
  static constexpr int32_t kNumRhythms = 6;
  static constexpr int32_t kNumColors = 3;
  static constexpr float kMaxLookback = static_cast<float>(kBufferSize - 512U);

  static const int32_t kParamMin[kNumParams];
  static const int32_t kParamMax[kNumParams];
  static const int32_t kParamInit[kNumParams];
  static const char * const kRhythmNames[kNumRhythms];
  static const char * const kPitchNames[37];
  static const char * const kColorNames[kNumColors];

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

  static inline float softClip(float x) {
    x = clampFloat(x, -4.0f, 4.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
  }

  static inline float outputLimit(float x) {
    return 1.14f * softClip(x * 0.87f);
  }

  static inline float safeAudio(float value, float fallback) {
    return value > -1000000.0f && value < 1000000.0f
               ? clampFloat(value, -4.0f, 4.0f)
               : fallback;
  }

  static inline uint8_t getVoiceBudget(float grain_len_pct, float density,
                                       int32_t rhythm) {
    const float load = clampFloat(grain_len_pct * density, 0.0f, 1.0f);
    int32_t budget = static_cast<int32_t>(20.0f - load * 12.0f);
    if (rhythm == 2 || rhythm == 4)
      budget -= 2;
    if (rhythm == 5)
      budget -= 3;
    return static_cast<uint8_t>(clampInt(budget, kMinVoiceBudget, kMaxGrains));
  }

  static inline uint8_t getSpawnLimit(float grain_len_pct, float density,
                                      int32_t rhythm) {
    const float load = clampFloat(grain_len_pct * density, 0.0f, 1.0f);
    int32_t limit = rhythm == 5 ? 5 : 4;
    limit -= static_cast<int32_t>(load * 2.8f);
    if (rhythm == 2 || rhythm == 4)
      --limit;
    return static_cast<uint8_t>(clampInt(limit, 1, 5));
  }

  static inline float grainEnvelope(float phase) {
    const float p = clampFloat(phase, 0.0f, 1.0f);
    return 4.0f * p * (1.0f - p);
  }

  inline float nextRand01() {
    rng_ = 1664525U * rng_ + 1013904223U;
    return static_cast<float>((rng_ >> 8) & 0x00FFFFFFU) / 16777215.0f;
  }

  inline float nextRandSigned() { return nextRand01() * 2.0f - 1.0f; }

  inline float wrapPosition(float position) const {
    if (!(position > -1000000.0f && position < 1000000.0f))
      return 0.0f;
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
        return samples_per_beat_ * 0.125f;
      case 3:
        return samples_per_beat_ / 3.0f;
      case 4:
        return samples_per_beat_ / 6.0f;
      case 5:
      default:
        return samples_per_beat_ * 0.25f;
    }
  }

  inline float getStepsPerBeat(float step_samples) const {
    return clampFloat(samples_per_beat_ / clampFloat(step_samples, 1.0f,
                                                     samples_per_beat_ * 2.0f),
                      1.0f, 12.0f);
  }

  static inline float mapGrainLengthSamples(float grain_len_pct) {
    const float shaped = powf(clampFloat(grain_len_pct, 0.0f, 1.0f), 1.18f);
    return 160.0f + shaped * 8200.0f;
  }

  inline void scheduleStep(float grains_per_beat, float density,
                           float grain_len_pct, int32_t rhythm,
                           float pitch_semitones, float spread, float pan,
                           float chaos, float step_samples,
                           uint8_t voice_budget, uint8_t spawn_limit) {
    const float steps_per_beat = getStepsPerBeat(step_samples);
    float density_variation = 1.0f + nextRandSigned() * chaos * 0.22f;

    if (rhythm == 5)
      density_variation *= 1.45f + 1.10f * chaos;

    density_accum_ += clampFloat((grains_per_beat / steps_per_beat) *
                                     density_variation,
                                 0.0f, 7.0f);

    if (rhythm == 5)
      density_accum_ += 0.65f + density * 2.35f;

    if (density > 0.72f && density_accum_ < 1.0f)
      density_accum_ = 1.0f;

    uint8_t spawned = 0;
    while (density_accum_ >= 1.0f && spawned < spawn_limit) {
      (void)spawnGrain(density, grain_len_pct, rhythm, pitch_semitones, spread,
                       pan, chaos, step_samples, grains_per_beat, voice_budget);
      density_accum_ -= 1.0f;
      ++spawned;
    }

    density_accum_ = clampFloat(density_accum_, 0.0f,
                                static_cast<float>(spawn_limit) * 0.75f);
  }

  inline bool spawnGrain(float density, float grain_len_pct, int32_t rhythm,
                         float pitch_semitones, float spread, float pan,
                         float chaos, float step_samples,
                         float grains_per_beat, uint8_t voice_budget) {
    uint8_t slot = 0;
    if (!findVoiceSlot(voice_budget, slot))
      return false;
    Grain & voice = grains_[slot];

    const float grain_length =
        clampFloat(mapGrainLengthSamples(grain_len_pct), 96.0f, kMaxLookback * 0.5f);
    const float rhythm_pull = rhythm == 2 || rhythm == 4 ? 0.70f : 1.0f;
    const float lookback_base =
        clampFloat(step_samples * (0.70f + 0.82f * spread) * rhythm_pull +
                       grain_length * 0.62f,
                   grain_length + 32.0f, kMaxLookback);
    const float timing_depth = step_samples * spread * (0.10f + 0.60f * chaos);
    float lookback = lookback_base + nextRandSigned() * timing_depth;

    if (rhythm == 5)
      lookback += nextRandSigned() * step_samples * (0.25f + 0.75f * chaos);

    lookback = clampFloat(lookback, grain_length + 32.0f, kMaxLookback);

    const float random_pitch =
        nextRandSigned() * spread * (1.5f + 10.5f * chaos) +
        nextRandSigned() * chaos * 2.5f;
    const float increment =
        semitonesToRatio(clampFloat(pitch_semitones + random_pitch, -30.0f,
                                    24.0f));

    const float stereo_bias = (pan - 0.5f) * 0.75f;
    const float random_stereo =
        nextRandSigned() * spread * (0.35f + 0.65f * chaos);
    const float stereo = clampFloat(stereo_bias + random_stereo, -0.98f, 0.98f);
    const float gain_l = sqrtf(0.5f * (1.0f - stereo));
    const float gain_r = sqrtf(0.5f * (1.0f + stereo));
    const float level =
        (0.48f + 0.36f * density + 0.18f * chaos) /
        sqrtf(1.0f + clampFloat(grains_per_beat, 0.0f, 28.0f) * 0.055f);

    voice.active = true;
    voice.read_pos = wrapPosition(static_cast<float>(write_index_) - lookback);
    voice.increment = increment;
    voice.age = 0.0f;
    voice.duration = grain_length;
    voice.gain = level;
    voice.gain_l = gain_l;
    voice.gain_r = gain_r;
    return true;
  }

  inline bool findVoiceSlot(uint8_t voice_budget, uint8_t &slot) const {
    const uint8_t limit = voice_budget > kMaxGrains ? kMaxGrains : voice_budget;

    for (uint8_t i = 0; i < limit; ++i) {
      if (!grains_[i].active) {
        slot = i;
        return true;
      }
    }

    return false;
  }

  inline float readBuffer(const int16_t * buffer, float position) const {
    const float wrapped = wrapPosition(position);
    const size_t index_int = static_cast<size_t>(wrapped);
    const size_t index_a = index_int & kBufferMask;
    const size_t index_b = (index_a + 1U) & kBufferMask;
    const float frac = wrapped - static_cast<float>(index_int);

    return lerp(i16ToFloat(buffer[index_a]), i16ToFloat(buffer[index_b]), frac);
  }

  inline void renderGrains(float &wet_l, float &wet_r, uint8_t voice_budget) {
    int32_t active_count = 0;
    const uint8_t limit = voice_budget > kMaxGrains ? kMaxGrains : voice_budget;

    for (uint8_t i = limit; i < kMaxGrains; ++i) {
      grains_[i].active = false;
    }

    for (uint8_t i = 0; i < limit; ++i) {
      Grain & voice = grains_[i];
      if (!voice.active)
        continue;

      const float phase = voice.age / voice.duration;
      if (phase >= 1.0f) {
        voice.active = false;
        continue;
      }

      const float env = grainEnvelope(phase);
      const float src_l = readBuffer(buffer_l_, voice.read_pos);
      const float src_r = readBuffer(buffer_r_, voice.read_pos);
      const float mono = 0.5f * (src_l + src_r);
      const float stereo_l = src_l * 0.70f + mono * 0.30f;
      const float stereo_r = src_r * 0.70f + mono * 0.30f;

      wet_l += stereo_l * env * voice.gain * voice.gain_l;
      wet_r += stereo_r * env * voice.gain * voice.gain_r;

      voice.read_pos = wrapPosition(voice.read_pos + voice.increment);
      voice.age += 1.0f;
      ++active_count;
    }

    if (active_count > 3) {
      const float norm =
          1.0f / (1.0f + static_cast<float>(active_count - 3) * 0.065f);
      wet_l *= norm;
      wet_r *= norm;
    }
  }

  inline void renderGhostRepeats(float &wet_l, float &wet_r, float step_samples,
                                 float density, float spread, float pan,
                                 float chaos, int32_t rhythm) {
    const float stereo_bias = (pan - 0.5f) * 0.55f;
    const float jitter = step_samples * spread * (0.05f + 0.23f * chaos);
    const float base = clampFloat(step_samples, 96.0f, kMaxLookback * 0.50f);
    const float burst_tighten = rhythm == 5 ? 0.52f : 1.0f;
    const float tap1 = clampFloat(base * burst_tighten, 96.0f, kMaxLookback);
    const float tap2 = clampFloat(base * (rhythm == 5 ? 0.76f : 1.72f), 128.0f,
                                  kMaxLookback);

    const float g1 = 0.18f + 0.40f * density;
    const float g2 = 0.07f + 0.24f * density * (0.45f + 0.55f * chaos);

    const float l1 = readBuffer(buffer_l_, static_cast<float>(write_index_) -
                                             tap1 - jitter * (1.0f + stereo_bias));
    const float r1 = readBuffer(buffer_r_, static_cast<float>(write_index_) -
                                             tap1 + jitter * (1.0f - stereo_bias));
    const float l2 = readBuffer(buffer_r_, static_cast<float>(write_index_) -
                                             tap2 + jitter * 0.57f);
    const float r2 = readBuffer(buffer_l_, static_cast<float>(write_index_) -
                                             tap2 - jitter * 0.57f);

    wet_l += l1 * g1 + l2 * g2;
    wet_r += r1 * g1 + r2 * g2;
  }

  inline void applyColorStereo(float &l, float &r, int32_t mode, float chaos) {
    switch (mode) {
      case 1: {
        const float lp_alpha = 0.12f + 0.06f * (1.0f - chaos);
        color_lp_[0] += lp_alpha * (l - color_lp_[0]);
        color_lp_[1] += lp_alpha * (r - color_lp_[1]);
        if (lofi_counter_ <= 0) {
          lofi_hold_[0] = quantize(softClip(color_lp_[0] * 1.35f), 6);
          lofi_hold_[1] = quantize(softClip(color_lp_[1] * 1.35f), 6);
          lofi_counter_ = 3 + static_cast<int32_t>(chaos * 5.0f);
        }
        --lofi_counter_;
        l = lofi_hold_[0];
        r = lofi_hold_[1];
        break;
      }
      case 2:
        color_lp_[0] += 0.055f * (l - color_lp_[0]);
        color_lp_[1] += 0.055f * (r - color_lp_[1]);
        l = color_lp_[0] * 0.90f;
        r = color_lp_[1] * 0.90f;
        break;
      case 0:
      default:
        color_lp_[0] = l;
        color_lp_[1] = r;
        break;
    }
  }

  static inline float quantize(float x, int32_t bits) {
    const int32_t clamped_bits = clampInt(bits, 3, 16);
    const float levels = static_cast<float>(1U << clamped_bits);
    const float scaled = x * levels;
    return (scaled >= 0.0f ? floorf(scaled + 0.5f) : ceilf(scaled - 0.5f)) /
           levels;
  }

  inline float processDcBlock(float x, uint8_t ch) {
    const float y = x - out_dc_x_[ch] + 0.995f * out_dc_y_[ch];
    out_dc_x_[ch] = x;
    out_dc_y_[ch] = y;
    return y;
  }

  inline void clearGrains() {
    for (uint8_t i = 0; i < kMaxGrains; ++i) {
      grains_[i].active = false;
      grains_[i].age = 0.0f;
      grains_[i].duration = 1.0f;
    }
  }

  std::atomic<int32_t> params_[kNumParams];
  int16_t buffer_l_[kBufferSize] = {};
  int16_t buffer_r_[kBufferSize] = {};
  Grain grains_[kMaxGrains];
  float color_lp_[2] = {0.0f, 0.0f};
  float lofi_hold_[2] = {0.0f, 0.0f};
  float out_dc_x_[2] = {0.0f, 0.0f};
  float out_dc_y_[2] = {0.0f, 0.0f};
  int32_t lofi_counter_ = 0;
  size_t write_index_ = 0U;
  float bpm_ = 120.0f;
  float samples_per_beat_ = 24000.0f;
  float scheduler_counter_ = 0.0f;
  float density_accum_ = 0.0f;
  float sm_density_ = 0.60f;
  float sm_grain_len_ = 0.45f;
  float sm_pitch_ = 0.0f;
  int32_t last_rhythm_ = 1;
  uint32_t rng_ = 0x12345678U;
};

const int32_t GrainDrift::kParamMin[9] = {0, 10, 0, 0, 0, 0, 0, 0, 0};
const int32_t GrainDrift::kParamMax[9] = {100, 100, 5, 100, 36, 100, 100, 2,
                                          100};
const int32_t GrainDrift::kParamInit[9] = {60, 45, 1, 55, 24, 55, 70, 0, 35};
const char * const GrainDrift::kRhythmNames[6] = {"1/8", "1/16", "1/32",
                                                  "1/8T", "1/16T", "BURST"};
const char * const GrainDrift::kPitchNames[37] = {
    "-24", "-23", "-22", "-21", "-20", "-19", "-18", "-17", "-16",
    "-15", "-14", "-13", "-12", "-11", "-10", "-9",  "-8",  "-7",
    "-6",  "-5",  "-4",  "-3",  "-2",  "-1",  "0",   "+1",  "+2",
    "+3",  "+4",  "+5",  "+6",  "+7",  "+8",   "+9",  "+10", "+11",
    "+12"};
const char * const GrainDrift::kColorNames[3] = {"CLEAN", "LOFI", "DARK"};
