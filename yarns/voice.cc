// Copyright 2013 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Voice.

#include "yarns/voice.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "stmlib/midi/midi.h"
#include "stmlib/utils/dsp.h"
#include "stmlib/utils/random.h"

#include "yarns/resources.h"

namespace yarns {
  
using namespace stmlib;
using namespace stmlib_midi;

const int32_t kOctave = 12 << 7;
const int32_t kMaxNote = 120 << 7;

void Voice::Init() {
  note_ = -1;
  note_source_ = note_target_ = note_portamento_ = 60 << 7;
  gate_ = false;
  
  mod_velocity_ = 0;
  ResetAllControllers();
  
  modulation_rate_ = 0;
  pitch_bend_range_ = 2;
  vibrato_range_ = 0;
  
  synced_lfo_.Init();
  envelope_.Init();
  portamento_phase_ = 0;
  portamento_phase_increment_ = 1U << 31;
  portamento_exponential_shape_ = false;
  
  trigger_duration_ = 2;
}

void CVOutput::Init(bool reset_calibration) {
  if (reset_calibration) {
    for (uint8_t i = 0; i < kNumOctaves; ++i) {
      calibrated_dac_code_[i] = 54586 - 5133 * i;
    }
  }
  dirty_ = false;
}

void CVOutput::Calibrate(uint16_t* calibrated_dac_code) {
  std::copy(
      &calibrated_dac_code[0],
      &calibrated_dac_code[kNumOctaves],
      &calibrated_dac_code_[0]);
}

inline void CVOutput::NoteToDacCode() {
  int32_t note = main_voice()->note();
  if (note <= 0) {
    note = 0;
  }
  if (note >= kMaxNote) {
    note = kMaxNote - 1;
  }
  uint8_t octave = 0;
  while (note >= kOctave) {
    note -= kOctave;
    ++octave;
  }
  
  // Note is now between 0 and kOctave
  // Octave indicates the octave. Look up in the DAC code table.
  int32_t a = calibrated_dac_code_[octave];
  int32_t b = calibrated_dac_code_[octave + 1];
  note_dac_code_ = a + ((b - a) * note / kOctave);
}

void Voice::ResetAllControllers() {
  mod_pitch_bend_ = 8192;
  mod_wheel_ = 0;
  std::fill(&mod_aux_[0], &mod_aux_[MOD_AUX_LAST - 1], 0);
}

bool Voice::Refresh() {
  // Compute base pitch with portamento.
  portamento_phase_ += portamento_phase_increment_;
  if (portamento_phase_ < portamento_phase_increment_) {
    portamento_phase_ = 0;
    portamento_phase_increment_ = 0;
    note_source_ = note_target_;
  }
  uint16_t portamento_level = portamento_exponential_shape_
      ? Interpolate824(lut_env_expo, portamento_phase_)
      : portamento_phase_ >> 16;
  int32_t note = note_source_ + \
      ((note_target_ - note_source_) * portamento_level >> 16);

  note_portamento_ = note;
  
  // Add pitch-bend.
  note += static_cast<int32_t>(mod_pitch_bend_ - 8192) * pitch_bend_range_ >> 6;
  
  // Add transposition/fine tuning.
  note += tuning_;
  
  // Add vibrato.
  if (modulation_rate_ < 100) {
    synced_lfo_.Increment(lut_lfo_increments[modulation_rate_]);
  } else {
    synced_lfo_.Refresh();
  }
  int32_t lfo = synced_lfo_.Triangle(synced_lfo_.GetPhase());
  uint16_t vibrato_control_value = 0;
  switch (vibrato_control_source_) {
    case VIBRATO_CONTROL_SOURCE_MODWHEEL:
      vibrato_control_value = mod_wheel_;
      break;
    case VIBRATO_CONTROL_SOURCE_AFTERTOUCH:
      vibrato_control_value = mod_aux_[MOD_AUX_AFTERTOUCH];
      break;
  }
  vibrato_control_value += vibrato_initial_;
  CONSTRAIN(vibrato_control_value, 0, 127);
  note += lfo * vibrato_control_value * vibrato_range_ >> 15;
  mod_aux_[MOD_AUX_VELOCITY] = mod_velocity_ << 9;
  mod_aux_[MOD_AUX_MODULATION] = mod_wheel_ << 9;
  mod_aux_[MOD_AUX_BEND] = static_cast<uint16_t>(mod_pitch_bend_) << 2;
  mod_aux_[MOD_AUX_VIBRATO_LFO] = (lfo * vibrato_control_value >> 7) + 32768;
  mod_aux_[MOD_AUX_FULL_LFO] = lfo + 32768;
  
  // Use quadrature phase for PWM LFO
  lfo = synced_lfo_.Triangle(synced_lfo_.GetPhase() + 0x40000000);
  // Initial and mod each have a full sweep of the PWM range
  uint32_t pw_21bit = lfo * oscillator_pw_mod_ + (oscillator_pw_initial_ << (21 - 7));
  // But clip combined modulation at 0-1
  CONSTRAIN(pw_21bit, 0, (1 << 21) - 1);
  oscillator_.SetPulseWidth(pw_21bit << (32 - 21));

  if (retrigger_delay_) {
    --retrigger_delay_;
  }
  
  if (trigger_pulse_) {
    --trigger_pulse_;
  }
  
  envelope_.Refresh();
  bool changed = note != note_;
  note_ = note;
  return changed;
}

void CVOutput::Refresh() {
  for (uint8_t i = 0; i < num_voices_; ++i) {
    bool changed = voices_[i]->Refresh();
    if (i == 0 && (changed || dirty_)) {
      NoteToDacCode();
      dirty_ = false;
    }
  }
}

void Voice::NoteOn(
    int16_t note,
    uint8_t velocity,
    uint8_t portamento,
    bool trigger) {
  note_source_ = note_portamento_;  
  note_target_ = note;
  if (!portamento) {
    note_source_ = note_target_;
  }
  portamento_phase_ = 0;
  uint32_t num_portamento_increments_per_shape = LUT_PORTAMENTO_INCREMENTS_SIZE >> 1;
  if (portamento < num_portamento_increments_per_shape) {
    portamento_phase_increment_ = lut_portamento_increments[portamento << 1];
    portamento_exponential_shape_ = true;
  } else {
    uint32_t base_increment = lut_portamento_increments[(portamento - num_portamento_increments_per_shape) << 1];
    uint32_t delta = abs(note_target_ - note_source_) + 1;
    portamento_phase_increment_ = (1536 * (base_increment >> 11) / delta) << 11;
    CONSTRAIN(portamento_phase_increment_, 1, 2147483647);
    portamento_exponential_shape_ = false;
  }

  mod_velocity_ = velocity;

  if (gate_ && trigger) {
    retrigger_delay_ = 2;
  }
  if (trigger) {
    envelope_.Trigger(ENV_SEGMENT_ATTACK);
    trigger_pulse_ = trigger_duration_ * 8;
  }
  gate_ = true;
}

void Voice::NoteOff() {
  gate_ = false;
}

void Voice::ControlChange(uint8_t controller, uint8_t value) {
  switch (controller) {
    case kCCModulationWheelMsb:
      mod_wheel_ = value;
      break;
    
    case kCCBreathController:
      mod_aux_[MOD_AUX_BREATH] = value << 9;
      break;
      
    case kCCFootPedalMsb:
      mod_aux_[MOD_AUX_PEDAL] = value << 9;
      break;
  }
}

int32_t Voice::trigger_value() const {
  int32_t velocity_coefficient = trigger_scale_ ? mod_velocity_ << 8 : 32768;
  return envelope_.value() * velocity_coefficient >> 15;
}

static const uint16_t kHighestNote = 128 * 128;
static const uint16_t kPitchTableStart = 116 * 128;


void Oscillator::Init(int32_t scale, int32_t offset) {
  audio_buffer_.Init();
  phase_ = 0;
  next_sample_ = 0;
  high_ = false;
  scale_ = scale;
  offset_ = offset;
  integrator_state_ = 0;
  pulse_width_ = 0x80000000;
}

uint32_t Oscillator::ComputePhaseIncrement(int16_t midi_pitch) {
  if (midi_pitch >= kHighestNote) {
    midi_pitch = kHighestNote - 1;
  }

  int32_t ref_pitch = midi_pitch;
  ref_pitch -= kPitchTableStart;

  size_t num_shifts = 0;
  while (ref_pitch < 0) {
    ref_pitch += kOctave;
    ++num_shifts;
  }

  uint32_t a = lut_oscillator_increments[ref_pitch >> 4];
  uint32_t b = lut_oscillator_increments[(ref_pitch >> 4) + 1];
  uint32_t phase_increment = a + \
      (static_cast<int32_t>(b - a) * (ref_pitch & 0xf) >> 4);
  phase_increment >>= num_shifts;
  return phase_increment;
}

void Oscillator::RenderSilence() {
  size_t size = kAudioBlockSize;
  while (size--) {
    WriteSample(0);
  }
}

void Oscillator::RenderSine(uint32_t phase_increment) {
  size_t size = kAudioBlockSize;
  while (size--) {
    phase_ += phase_increment;
    WriteSample(Interpolate1022(wav_sine, phase_));
  }
}

void Oscillator::RenderNoise() {
  size_t size = kAudioBlockSize;
  while (size--) {
    WriteSample(Random::GetSample());
  }
}

void Oscillator::RenderSaw(uint32_t phase_increment) {
  uint32_t phase = phase_;
  int32_t next_sample = next_sample_;
  size_t size = kAudioBlockSize;

  while (size--) {
    int32_t this_sample = next_sample;
    next_sample = 0;
    phase += phase_increment;
    if (phase < phase_increment) {
      uint32_t t = phase / (phase_increment >> 16);
      this_sample -= ThisBlepSample(t);
      next_sample -= NextBlepSample(t);
    }
    next_sample += phase >> 17;
    this_sample = (this_sample - 16384) << 1;
    WriteSample(this_sample);
  }
  next_sample_ = next_sample;
  phase_ = phase;
}

void Oscillator::RenderSquare(
    uint32_t phase_increment,
    uint32_t pw,
    bool integrate) {
  uint32_t phase = phase_;
  int32_t next_sample = next_sample_;
  int32_t integrator_state = integrator_state_;
  int16_t integrator_coefficient = phase_increment >> 18;
  size_t size = kAudioBlockSize;

  while (size--) {
    int32_t this_sample = next_sample;
    next_sample = 0;
    phase += phase_increment;

    if (!high_) {
      if (phase >= pw) {
        uint32_t t = (phase - pw) / (phase_increment >> 16);
        this_sample += ThisBlepSample(t);
        next_sample += NextBlepSample(t);
        high_ = true;
      }
    }
    if (high_ && (phase < phase_increment)) {
      uint32_t t = phase / (phase_increment >> 16);
      this_sample -= ThisBlepSample(t);
      next_sample -= NextBlepSample(t);
      high_ = false;
    }
    next_sample += phase < pw ? 0 : 32767;
    this_sample = (this_sample - 16384) << 1;
    if (integrate) {
      integrator_state += integrator_coefficient * (this_sample - integrator_state) >> 15;
      this_sample = integrator_state << 3;
    }
    WriteSample(this_sample);
  }
  integrator_state_ = integrator_state;
  next_sample_ = next_sample;
  phase_ = phase;
}

void Oscillator::Render(uint8_t mode, int16_t note, bool gate) {
  if (mode == AUDIO_MODE_OFF || audio_buffer_.writable() < kAudioBlockSize) {
    return;
  }
  
  if ((mode & 0x80) && !gate) { // See 'paques'
    RenderSilence();
    return;
  }
  
  uint32_t phase_increment = ComputePhaseIncrement(note);
  switch (mode & 0x0f) {
    case AUDIO_MODE_SAW:
      RenderSaw(phase_increment);
      break;
    case AUDIO_MODE_PULSE_VARIABLE:
      RenderSquare(phase_increment, pulse_width_, false);
      break;
    case AUDIO_MODE_PULSE_50:
      RenderSquare(phase_increment, 0x80000000, false);
      break;
    case AUDIO_MODE_TRIANGLE:
      RenderSquare(phase_increment, 0x80000000, true);
      break;
    case AUDIO_MODE_SINE:
      RenderSine(phase_increment);
      break;
    case AUDIO_MODE_NOISE:
      RenderNoise();
      break;
  }
}

}  // namespace yarns