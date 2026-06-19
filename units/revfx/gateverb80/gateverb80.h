#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>

#include "unit.h"

class GateVerb80 {
 public:
  GateVerb80() {
    for (uint8_t i = 0; i < kNumParams; ++i) {
      params_[i].store(kParamInit[i], std::memory_order_relaxed);
    }
  }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (desc->samplerate != kSampleRate)
      return k_unit_err_samplerate;

    if (desc->input_channels != 2 || desc->output_channels != 2)
      return k_unit_err_geometry;

    Reset();
    return k_unit_err_none;
  }

  inline void Teardown() {}

  inline void Reset() {
    predelay_index_ = 0;
    input_env_ = 0.0f;
    comp_env_ = 0.0f;
    wet_gain_smooth_ = 1.0f;
    gate_env_ = 0.0f;
    gate_hold_counter_ = 0;

    for (uint8_t ch = 0; ch < 2; ++ch) {
      hp_lp_[ch] = 0.0f;
      tone_lp_[ch] = 0.0f;
    }

    for (size_t i = 0; i < kPredelaySize; ++i) {
      predelay_l_[i] = 0.0f;
      predelay_r_[i] = 0.0f;
    }

    clearCombBuffers();
    clearAllpassBuffers();
  }

  inline void Resume() {}
  inline void Suspend() {}

  fast_inline void Process(const float * in, float * out, size_t frames) {
    const float size = percentToUnit(getClampedParam(kParamSize));
    const float gate = percentToUnit(getClampedParam(kParamGate));
    const float tone = percentToUnit(getClampedParam(kParamTone));
    const float comp = percentToUnit(getClampedParam(kParamComp));
    const float predelay_ms = static_cast<float>(getClampedParam(kParamPredelay));
    const float hp = percentToUnit(getClampedParam(kParamHp));
    const float width = percentToUnit(getClampedParam(kParamWidth));
    const float mix = percentToUnit(getClampedParam(kParamMix));

    const int32_t predelay_samples = clampInt(static_cast<int32_t>(predelay_ms * 48.0f), 0,
                                              static_cast<int32_t>(kPredelaySize - 2));
    const float reverb_feedback = 0.60f + size * 0.30f;
    const float reverb_damp = 0.18f + (1.0f - size) * 0.22f;
    const float comp_threshold_db = -10.0f - 30.0f * comp;
    const float comp_threshold_lin = dbToLin(comp_threshold_db);
    const float comp_ratio = 2.0f + 6.0f * comp;
    const float comp_attack = expf(-1.0f / (0.0025f * static_cast<float>(kSampleRate)));
    const float comp_release = expf(-1.0f / (0.110f * static_cast<float>(kSampleRate)));
    const int32_t gate_hold_samples =
        static_cast<int32_t>(lerp(0.220f, 0.055f, gate) * static_cast<float>(kSampleRate));
    const float gate_release_coef =
        expf(-1.0f / (lerp(0.180f, 0.045f, gate) * static_cast<float>(kSampleRate)));
    const float gate_shape = 1.0f + 4.0f * gate;
    const float hp_alpha = hzToOnePole(80.0f * powf(5.0f, hp));

    const float * __restrict in_p = in;
    float * __restrict out_p = out;
    for (size_t i = 0; i < frames; ++i, in_p += 2, out_p += 2) {
      const float dry_l = in_p[0];
      const float dry_r = in_p[1];
      const float detector = sqrtf(0.5f * (dry_l * dry_l + dry_r * dry_r) + 1.0e-12f);

      input_env_ = detector > input_env_
                       ? 0.10f * detector + 0.90f * input_env_
                       : 0.005f * detector + 0.995f * input_env_;

      if (input_env_ > 0.035f) {
        gate_env_ = 1.0f;
        gate_hold_counter_ = gate_hold_samples;
      } else if (gate_hold_counter_ > 0) {
        --gate_hold_counter_;
      } else {
        gate_env_ = gate_release_coef * gate_env_;
      }

      float wet_l = 0.0f;
      float wet_r = 0.0f;
      processPredelay(dry_l, dry_r, predelay_samples, wet_l, wet_r);
      processReverbCore(wet_l, wet_r, reverb_feedback, reverb_damp);
      processWetCompressor(wet_l, wet_r, comp_threshold_lin, comp_ratio,
                           comp_attack, comp_release);

      const float gated = powf(clampFloat(gate_env_, 0.0f, 1.0f), gate_shape);
      wet_l *= gated;
      wet_r *= gated;

      wet_l = processHp(wet_l, 0, hp_alpha);
      wet_r = processHp(wet_r, 1, hp_alpha);
      wet_l = processTone(wet_l, 0, tone);
      wet_r = processTone(wet_r, 1, tone);
      applyWidth(wet_l, wet_r, width);

      out_p[0] = clampAudio(dry_l + (wet_l - dry_l) * mix);
      out_p[1] = clampAudio(dry_r + (wet_r - dry_r) * mix);
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
    (void)index;
    (void)value;
    return nullptr;
  }

  inline const uint8_t * getParameterBmpValue(uint8_t index, int32_t value) const {
    (void)index;
    (void)value;
    return nullptr;
  }

  inline void LoadPreset(uint8_t idx) { (void)idx; }
  inline uint8_t getPresetIndex() const { return 0; }

  static inline const char * getPresetName(uint8_t idx) {
    (void)idx;
    return nullptr;
  }

 private:
  enum ParamIndex : uint8_t {
    kParamSize = 0,
    kParamGate,
    kParamTone,
    kParamComp,
    kParamPredelay,
    kParamHp,
    kParamWidth,
    kParamMix,
    kNumParams
  };

  static constexpr uint32_t kSampleRate = 48000U;
  static constexpr size_t kPredelaySize = 2048U;
  static constexpr int kCombTaps = 4;
  static constexpr int kAllpassTaps = 2;
  static constexpr int kCombLenL0 = 1116;
  static constexpr int kCombLenL1 = 1188;
  static constexpr int kCombLenL2 = 1277;
  static constexpr int kCombLenL3 = 1356;
  static constexpr int kCombLenR0 = 1139;
  static constexpr int kCombLenR1 = 1211;
  static constexpr int kCombLenR2 = 1300;
  static constexpr int kCombLenR3 = 1379;
  static constexpr int kAllpassLenL0 = 556;
  static constexpr int kAllpassLenL1 = 441;
  static constexpr int kAllpassLenR0 = 579;
  static constexpr int kAllpassLenR1 = 464;

  static const int32_t kParamMin[kNumParams];
  static const int32_t kParamMax[kNumParams];
  static const int32_t kParamInit[kNumParams];

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

  static inline float dbToLin(float db) {
    return powf(10.0f, db * 0.05f);
  }

  static inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
  }

  static inline float hzToOnePole(float hz) {
    const float normalized = clampFloat(hz / static_cast<float>(kSampleRate), 0.0005f, 0.45f);
    return 1.0f - expf(-2.0f * 3.14159265358979323846f * normalized);
  }

  inline void processPredelay(float in_l, float in_r, int32_t predelay_samples,
                              float &out_l, float &out_r) {
    predelay_l_[predelay_index_] = in_l;
    predelay_r_[predelay_index_] = in_r;

    int32_t read = static_cast<int32_t>(predelay_index_) - predelay_samples;
    while (read < 0)
      read += static_cast<int32_t>(kPredelaySize);

    out_l = predelay_l_[read];
    out_r = predelay_r_[read];
    predelay_index_ = (predelay_index_ + 1U) % kPredelaySize;
  }

  inline float processComb(float * buffer, int size, int &index, float &filter_store,
                           float input, float feedback, float damp) {
    const float output = buffer[index];
    filter_store = output * (1.0f - damp) + filter_store * damp;
    buffer[index] = input + filter_store * feedback;
    index = index + 1 >= size ? 0 : index + 1;
    return output;
  }

  inline float processAllpass(float * buffer, int size, int &index, float input) {
    const float buf_out = buffer[index];
    const float output = -input + buf_out;
    buffer[index] = input + buf_out * 0.5f;
    index = index + 1 >= size ? 0 : index + 1;
    return output;
  }

  inline void processReverbCore(float &l, float &r, float feedback, float damp) {
    const float mono = 0.5f * (l + r);
    float wet_l = 0.0f;
    float wet_r = 0.0f;

    wet_l += processComb(comb_l0_, kCombLenL0, comb_idx_l_[0], comb_store_l_[0], mono, feedback, damp);
    wet_l += processComb(comb_l1_, kCombLenL1, comb_idx_l_[1], comb_store_l_[1], mono, feedback, damp);
    wet_l += processComb(comb_l2_, kCombLenL2, comb_idx_l_[2], comb_store_l_[2], mono, feedback, damp);
    wet_l += processComb(comb_l3_, kCombLenL3, comb_idx_l_[3], comb_store_l_[3], mono, feedback, damp);

    wet_r += processComb(comb_r0_, kCombLenR0, comb_idx_r_[0], comb_store_r_[0], mono, feedback, damp);
    wet_r += processComb(comb_r1_, kCombLenR1, comb_idx_r_[1], comb_store_r_[1], mono, feedback, damp);
    wet_r += processComb(comb_r2_, kCombLenR2, comb_idx_r_[2], comb_store_r_[2], mono, feedback, damp);
    wet_r += processComb(comb_r3_, kCombLenR3, comb_idx_r_[3], comb_store_r_[3], mono, feedback, damp);

    wet_l = processAllpass(allpass_l0_, kAllpassLenL0, allpass_idx_l_[0], wet_l * 0.25f);
    wet_l = processAllpass(allpass_l1_, kAllpassLenL1, allpass_idx_l_[1], wet_l);
    wet_r = processAllpass(allpass_r0_, kAllpassLenR0, allpass_idx_r_[0], wet_r * 0.25f);
    wet_r = processAllpass(allpass_r1_, kAllpassLenR1, allpass_idx_r_[1], wet_r);

    l = wet_l * 0.9f;
    r = wet_r * 0.9f;
  }

  inline void processWetCompressor(float &l, float &r, float threshold_lin,
                                   float ratio, float attack_coef,
                                   float release_coef) {
    const float detector = sqrtf(0.5f * (l * l + r * r) + 1.0e-12f);
    const float coef = detector > comp_env_ ? attack_coef : release_coef;
    comp_env_ = coef * comp_env_ + (1.0f - coef) * detector;

    const float env_db = 20.0f * log10f(comp_env_ + 1.0e-12f);
    const float threshold_db = 20.0f * log10f(threshold_lin);
    const float over_db = env_db - threshold_db;
    float target_gain = 1.0f;

    if (over_db > 0.0f) {
      target_gain = dbToLin((1.0f / ratio - 1.0f) * over_db);
    }

    wet_gain_smooth_ = 0.985f * wet_gain_smooth_ + 0.015f * target_gain;
    l *= wet_gain_smooth_;
    r *= wet_gain_smooth_;
  }

  inline float processHp(float x, uint8_t ch, float alpha) {
    hp_lp_[ch] += alpha * (x - hp_lp_[ch]);
    return x - hp_lp_[ch];
  }

  inline float processTone(float x, uint8_t ch, float tone) {
    const float alpha = 0.12f + 0.28f * tone;
    tone_lp_[ch] += alpha * (x - tone_lp_[ch]);
    const float high = x - tone_lp_[ch];

    if (tone < 0.5f) {
      const float dark = (0.5f - tone) * 2.0f;
      return x - high * 0.85f * dark;
    }

    const float bright = (tone - 0.5f) * 2.0f;
    return x + high * 0.45f * bright;
  }

  static inline void applyWidth(float &l, float &r, float width) {
    const float mid = 0.5f * (l + r);
    const float side = 0.5f * (l - r) * (0.2f + 1.6f * width);
    l = mid + side;
    r = mid - side;
  }

  inline void clearCombBuffers() {
    for (int i = 0; i < kCombLenL0; ++i) comb_l0_[i] = 0.0f;
    for (int i = 0; i < kCombLenL1; ++i) comb_l1_[i] = 0.0f;
    for (int i = 0; i < kCombLenL2; ++i) comb_l2_[i] = 0.0f;
    for (int i = 0; i < kCombLenL3; ++i) comb_l3_[i] = 0.0f;
    for (int i = 0; i < kCombLenR0; ++i) comb_r0_[i] = 0.0f;
    for (int i = 0; i < kCombLenR1; ++i) comb_r1_[i] = 0.0f;
    for (int i = 0; i < kCombLenR2; ++i) comb_r2_[i] = 0.0f;
    for (int i = 0; i < kCombLenR3; ++i) comb_r3_[i] = 0.0f;

    for (int i = 0; i < kCombTaps; ++i) {
      comb_idx_l_[i] = 0;
      comb_idx_r_[i] = 0;
      comb_store_l_[i] = 0.0f;
      comb_store_r_[i] = 0.0f;
    }
  }

  inline void clearAllpassBuffers() {
    for (int i = 0; i < kAllpassLenL0; ++i) allpass_l0_[i] = 0.0f;
    for (int i = 0; i < kAllpassLenL1; ++i) allpass_l1_[i] = 0.0f;
    for (int i = 0; i < kAllpassLenR0; ++i) allpass_r0_[i] = 0.0f;
    for (int i = 0; i < kAllpassLenR1; ++i) allpass_r1_[i] = 0.0f;

    for (int i = 0; i < kAllpassTaps; ++i) {
      allpass_idx_l_[i] = 0;
      allpass_idx_r_[i] = 0;
    }
  }

  std::atomic<int32_t> params_[kNumParams];
  float predelay_l_[kPredelaySize] = {};
  float predelay_r_[kPredelaySize] = {};
  size_t predelay_index_ = 0U;

  float input_env_ = 0.0f;
  float comp_env_ = 0.0f;
  float wet_gain_smooth_ = 1.0f;
  float gate_env_ = 0.0f;
  int32_t gate_hold_counter_ = 0;
  float hp_lp_[2] = {0.0f, 0.0f};
  float tone_lp_[2] = {0.0f, 0.0f};

  int comb_idx_l_[kCombTaps] = {};
  int comb_idx_r_[kCombTaps] = {};
  float comb_store_l_[kCombTaps] = {};
  float comb_store_r_[kCombTaps] = {};
  int allpass_idx_l_[kAllpassTaps] = {};
  int allpass_idx_r_[kAllpassTaps] = {};

  float comb_l0_[kCombLenL0] = {};
  float comb_l1_[kCombLenL1] = {};
  float comb_l2_[kCombLenL2] = {};
  float comb_l3_[kCombLenL3] = {};
  float comb_r0_[kCombLenR0] = {};
  float comb_r1_[kCombLenR1] = {};
  float comb_r2_[kCombLenR2] = {};
  float comb_r3_[kCombLenR3] = {};
  float allpass_l0_[kAllpassLenL0] = {};
  float allpass_l1_[kAllpassLenL1] = {};
  float allpass_r0_[kAllpassLenR0] = {};
  float allpass_r1_[kAllpassLenR1] = {};
};

const int32_t GateVerb80::kParamMin[8] = {0, 0, 0, 0, 0, 0, 0, 0};
const int32_t GateVerb80::kParamMax[8] = {100, 100, 100, 100, 40, 100, 100, 100};
const int32_t GateVerb80::kParamInit[8] = {60, 70, 50, 60, 15, 30, 75, 40};
