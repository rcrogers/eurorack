// Copyright 2012 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef YARNS_ENVELOPE_H_
#define YARNS_ENVELOPE_H_

#include "stmlib/stmlib.h"

#include "stmlib/utils/dsp.h"

#include "yarns/resources.h"

namespace yarns {

using namespace stmlib;

enum EnvelopeSegment {
  ENV_SEGMENT_ATTACK,
  ENV_SEGMENT_DECAY,
  ENV_SEGMENT_SUSTAIN,
  ENV_SEGMENT_RELEASE,
  ENV_SEGMENT_DEAD,

  ENV_NUM_SEGMENTS
};

enum EnvelopeCurve {
  ENVELOPE_CURVE_SQUARE,
  ENVELOPE_CURVE_LINEAR,
  ENVELOPE_CURVE_EXPONENTIAL,
  ENVELOPE_CURVE_RING,
  ENVELOPE_CURVE_STEPS,
  ENVELOPE_CURVE_NOISE_BURST,

  ENVELOPE_CURVE_LAST
};

class Envelope {
 public:
  Envelope() { }
  ~Envelope() { }

  void Init() {
    target_[ENV_SEGMENT_ATTACK] = 65535;
    target_[ENV_SEGMENT_RELEASE] = 0;
    target_[ENV_SEGMENT_DEAD] = 0;

    increment_[ENV_SEGMENT_SUSTAIN] = 0;
    increment_[ENV_SEGMENT_DEAD] = 0;
    shape_[ENV_SEGMENT_SUSTAIN] = ENVELOPE_CURVE_EXPONENTIAL;
    shape_[ENV_SEGMENT_DEAD] = ENVELOPE_CURVE_EXPONENTIAL;
  }

  inline EnvelopeSegment segment() const {
    return static_cast<EnvelopeSegment>(segment_);
  }

  inline void SetADSR(int32_t a, int32_t d, int32_t s, int32_t r) {
    increment_[ENV_SEGMENT_ATTACK] = lut_portamento_increments[a];
    increment_[ENV_SEGMENT_DECAY] = lut_portamento_increments[d];
    increment_[ENV_SEGMENT_RELEASE] = lut_portamento_increments[r];
    target_[ENV_SEGMENT_DECAY] = s << 9;
    target_[ENV_SEGMENT_SUSTAIN] = target_[ENV_SEGMENT_DECAY];
  }

  inline void SetCurves(EnvelopeCurve a, EnvelopeCurve d, EnvelopeCurve r) {
    shape_[ENV_SEGMENT_ATTACK] = a;
    shape_[ENV_SEGMENT_DECAY] = d;
    shape_[ENV_SEGMENT_RELEASE] = r;
  }
  
  inline void Trigger(EnvelopeSegment segment) {
    if (segment == ENV_SEGMENT_DEAD) {
      value_ = 0;
    }
    segment_amp_start = value_;
    segment_amp_end = target_[segment];
    segment_ = segment;
    phase_ = 0;
  }

  static inline int16_t Amplitude(EnvelopeCurve curve, uint32_t phase) {
    
  }

  inline void Refresh() {
    uint32_t increment = increment_[segment_];
    phase_ += increment;
    if (phase_ < increment) {
      value_ = Mix(segment_amp_start, segment_amp_end, 0xffff);
      Trigger(static_cast<EnvelopeSegment>(segment_ + 1));
    }
    if (increment_[segment_]) {
      int16_t segment_amp_scaling;
      switch(shape_[segment_]) {
        case ENVELOPE_CURVE_SQUARE:
          segment_amp_scaling = 0x7fff;
          break;
        case ENVELOPE_CURVE_LINEAR:
          segment_amp_scaling = 0x7fff - (phase_ >> 17);
          break;
        default:
          {
            const int16_t* table = waveform_table[shape_[segment_] - ENVELOPE_CURVE_EXPONENTIAL];
            segment_amp_scaling = Interpolate824(table, phase_);
          }
          break;
      }
      value_ = Mix(segment_amp_start, segment_amp_end, segment_amp_scaling);
    }
  }
  
  inline uint16_t value() const { return value_; }

 private:
  // Phase increments for each segment.
  uint32_t increment_[ENV_NUM_SEGMENTS];
  
  // Value that needs to be reached at the end of each segment.
  uint16_t target_[ENV_NUM_SEGMENTS];
  
  EnvelopeCurve shape_[ENV_NUM_SEGMENTS];

  // Current segment.
  size_t segment_;
  
  // Start and end value of the current segment.
  uint16_t segment_amp_start;
  uint16_t segment_amp_end;
  uint16_t value_; // TODO this is for unipolar envelopes, but some of the trigger shapes are bipolar

  uint32_t phase_increment_;
  uint32_t phase_;

  DISALLOW_COPY_AND_ASSIGN(Envelope);
};

}  // namespace yarns

#endif  // YARNS_ENVELOPE_H_
