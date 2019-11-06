#include "math.hpp"
#include "dsp/digital.hpp"

using namespace rack;

struct LowFrequencyOscillator {
  float phase = 0.0f;
  float lastPhase = 0.0f;
  float freq = 1.0f;
  float freqCorrection = 0.0f;

  int PPQN = 1;
  float freqTarget = 1.0f;

  dsp::SchmittTrigger clockTrigger;

  struct NormalizationResult {
    float value;
    bool normalized;
  };

  LowFrequencyOscillator() {}

  void setPitch(float pitch) {
    freq = pitch;
  }

  void reset(float value) {
    this->phase = std::fmod(value, 1.0f);
    this->freqCorrection = 0.0f;
  }

  bool step(float dt) {
    float deltaPhase = (freq + this->freqCorrection) * dt;
    float summ = phase + deltaPhase;
    this->lastPhase = this->phase;
    this->phase = rack::math::eucMod(summ, 1.0f);
    bool flipped = freq >= 0.0f ? summ >= 1.0f : summ < 0.0f;
    return flipped;
  }

  void adjustPhase(float pulse) {
    if (!this->clockTrigger.process(pulse)) { return; }
    if (phase == 0.0f) {
      this->freqCorrection = 0.0f;
      return;
    }
    float segmentLength = 1.0f / this->PPQN;
    float absoluteSegmentPhase = std::fmod(this->phase, segmentLength);
    float scaledPhase = absoluteSegmentPhase * this->PPQN;
    float tempoBendingLimit = 0.99f * std::abs(this->freq);
    if (this->freq >= 0.0f) {
      if (scaledPhase < 0.5f) {
        // We are moving too fast
        float deviation = scaledPhase;
        this->freqCorrection = -std::min(this->freq * deviation, tempoBendingLimit);
      } else {
        // We are lagging behind
        float deviation = 1.0f - scaledPhase;
        this->freqCorrection = std::min(this->freq * deviation, tempoBendingLimit);
      }
    } else {
      if (scaledPhase >= 0.5f) {
        // We are moving too fast
        float deviation = 1.0f - scaledPhase;
        this->freqCorrection = std::min(-this->freq * deviation, tempoBendingLimit);
      } else {
        // We are lagging behind
        float deviation = scaledPhase;
        this->freqCorrection = -std::min(-this->freq * deviation, tempoBendingLimit);
      }
    }
  }
};

struct ClockTracker {
  int triggersPassed;
  float period;
  float freq;
  bool freqDetected;

  dsp::SchmittTrigger clockTrigger;

  void init() {
    triggersPassed = 0;
    period = 0.0f;
    freq = 0.0f;
    freqDetected = false;
  }

  void process(float dt, float pulse) {
    period += dt;
    if (clockTrigger.process(pulse)) {
      if (triggersPassed < 3) {
        triggersPassed += 1;
      }
      if (triggersPassed > 2) {
        freqDetected = true;
        freq = 1.0f / period;
      }
      period = 0.0f;
    }
  }
};
