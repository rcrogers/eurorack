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
// User interface.

#include "stmlib/system/system_clock.h"

#include "yarns/multi.h"
#include "yarns/ui.h"
#include "yarns/voice.h"

#include <cstring>

namespace yarns {

using namespace std;
using namespace stmlib;

const uint32_t kEncoderLongPressTime = 600;

/* static */
const Ui::Command Ui::commands_[] = {
  { "*LOAD*", UI_MODE_LOAD_SELECT_PROGRAM, NULL },
  { "*SAVE*", UI_MODE_SAVE_SELECT_PROGRAM, NULL },
  { "*INIT*", UI_MODE_PARAMETER_SELECT, &Ui::DoInitCommand },
  { "*QUICK CONFIG*", UI_MODE_LEARNING, &Ui::DoLearnCommand },
  { "*>SYSEX DUMP*", UI_MODE_PARAMETER_SELECT, &Ui::DoDumpCommand },
  { "*CALIBRATE*", UI_MODE_CALIBRATION_SELECT_VOICE, NULL },
  { "*EXIT*", UI_MODE_PARAMETER_SELECT, NULL },
};

/* static */
Ui::Mode Ui::modes_[] = {
  // UI_MODE_PARAMETER_SELECT
  { &Ui::OnIncrementParameterSelect, &Ui::OnClick,
    &Ui::PrintParameterName,
    UI_MODE_PARAMETER_EDIT,
    NULL, 0, 0 },
  
  // UI_MODE_PARAMETER_EDIT
  { &Ui::OnIncrementParameterEdit, &Ui::OnClick,
    &Ui::PrintParameterValue,
    UI_MODE_PARAMETER_SELECT,
    NULL, 0, 0 },
  
  // UI_MODE_MAIN_MENU
  { &Ui::OnIncrement, &Ui::OnClickMainMenu,
    &Ui::PrintMenuName,
    UI_MODE_MAIN_MENU,
    NULL, 0, MAIN_MENU_LAST - 1 },
  
  // UI_MODE_LOAD_SELECT_PROGRAM
  { &Ui::OnIncrement, &Ui::OnClickLoadSave,
    &Ui::PrintProgramNumber,
    UI_MODE_MAIN_MENU,
    NULL, 0, kNumPrograms },
  
  // UI_MODE_SAVE_SELECT_PROGRAM
  { &Ui::OnIncrement, &Ui::OnClickLoadSave,
    &Ui::PrintProgramNumber,
    UI_MODE_MAIN_MENU,
    NULL, 0, kNumPrograms },
  
  // UI_MODE_CALIBRATION_SELECT_VOICE
  { &Ui::OnIncrement, &Ui::OnClickCalibrationSelectVoice,
    &Ui::PrintCalibrationVoiceNumber,
    UI_MODE_CALIBRATION_SELECT_VOICE,
    NULL, 0, kNumSystemVoices },
  
  // UI_MODE_CALIBRATION_SELECT_NOTE
  { &Ui::OnIncrement, &Ui::OnClickCalibrationSelectNote,
    &Ui::PrintCalibrationNote,
    UI_MODE_CALIBRATION_SELECT_NOTE,
    NULL, 0, kNumOctaves },
  
  // UI_MODE_CALIBRATION_ADJUST_LEVEL
  { &Ui::OnIncrementCalibrationAdjustment, &Ui::OnClick,
    &Ui::PrintCalibrationNote,
    UI_MODE_CALIBRATION_SELECT_NOTE,
    NULL, 0, 0 },
  
  // UI_MODE_RECORDING
  { &Ui::OnIncrementRecording, &Ui::OnClickRecording,
    &Ui::PrintRecordingStatus,
    UI_MODE_RECORDING,
    NULL, 0, 0 },

  // UI_MODE_OVERDUBBING
  { &Ui::OnIncrementOverdubbing, &Ui::OnClickOverdubbing,
    &Ui::PrintRecordingStatus,
    UI_MODE_OVERDUBBING,
    NULL, 0, 0 },

  // UI_MODE_PUSH_IT_SELECT_NOTE
  { &Ui::OnIncrementPushItNote, &Ui::OnClick,
    &Ui::PrintPushItNote,
    UI_MODE_PARAMETER_SELECT,
    NULL, 0, 127 },

  // UI_MODE_LEARNING
  { &Ui::OnIncrement, &Ui::OnClickLearning,
    &Ui::PrintLearning,
    UI_MODE_PARAMETER_SELECT,
    NULL, 0, 127 },
    
  // UI_MODE_FACTORY_TESTING
  { &Ui::OnIncrementFactoryTesting, &Ui::OnClickFactoryTesting,
    &Ui::PrintFactoryTesting,
    UI_MODE_PARAMETER_SELECT,
    NULL, 0, 99 },
    
  // UI_MODE_SPLASH
  { &Ui::OnIncrementParameterSelect, &Ui::OnClick,
    &Ui::PrintVersionNumber,
    UI_MODE_PARAMETER_SELECT,
    NULL, 0, 0 },

  // UI_MODE_CHANGED_ACTIVE_PART_OR_PLAY_MODE
  { &Ui::OnIncrementParameterSelect, &Ui::OnClick,
    &Ui::PrintActivePartAndPlayMode,
    UI_MODE_PARAMETER_SELECT,
    NULL, 0, 0 },

  // UI_MODE_LOOPER_RECORDING
  { &Ui::OnIncrement, &Ui::OnClick,
    &Ui::PrintLooperRecordingStatus,
    UI_MODE_LOOPER_RECORDING,
    NULL, 0, 0 },

  // UI_MODE_SEQUENCE_DELETED
};

void Ui::Init() {
  encoder_.Init();
  display_.Init();
  switches_.Init();
  queue_.Init();
  leds_.Init();
  
  mode_ = UI_MODE_PARAMETER_SELECT;
  splash_mode_ = UI_MODE_SPLASH;
  show_splash_ = true;
  current_menu_category_ = &Settings::live_menus;
  previous_tap_time_ = 0;
  tap_tempo_count_ = 0;
  
  start_stop_press_time_ = 0;
  
  push_it_note_ = 60;
  modes_[UI_MODE_MAIN_MENU].incremented_variable = &command_index_;
  modes_[UI_MODE_LOAD_SELECT_PROGRAM].incremented_variable = &program_index_;
  modes_[UI_MODE_SAVE_SELECT_PROGRAM].incremented_variable = &program_index_;
  modes_[UI_MODE_CALIBRATION_SELECT_VOICE].incremented_variable = \
      &calibration_voice_;
  modes_[UI_MODE_CALIBRATION_SELECT_NOTE].incremented_variable = \
      &calibration_note_;
  modes_[UI_MODE_FACTORY_TESTING].incremented_variable = \
      &factory_testing_number_;
  PrintVersionNumber();
}

void Ui::Poll() {
  encoder_.Debounce();
  
  // Handle press and long press on encoder.
  if (encoder_.just_pressed()) {
    encoder_press_time_ = system_clock.milliseconds();
    encoder_long_press_event_sent_ = false;
  }
  if (!encoder_long_press_event_sent_) {
    if (encoder_.pressed()) {
      uint32_t duration = system_clock.milliseconds() - encoder_press_time_;
      if (duration >= kEncoderLongPressTime && !encoder_long_press_event_sent_) {
        queue_.AddEvent(CONTROL_ENCODER_LONG_CLICK, 0, 0);
        encoder_long_press_event_sent_ = true;
      }
    } else if (encoder_.released()) {
      queue_.AddEvent(CONTROL_ENCODER_CLICK, 0, 0);
    }
  }
  
  // Encoder increment.
  int32_t increment = encoder_.increment();
  if (increment != 0) {
    queue_.AddEvent(CONTROL_ENCODER, 0, increment);
  }

  // Switch press and long press.
  switches_.Debounce();
  PollSwitch(UI_SWITCH_REC        , rec_press_time_       , rec_long_press_event_sent_        );
  PollSwitch(UI_SWITCH_START_STOP , start_stop_press_time_, start_stop_long_press_event_sent_ );
  PollSwitch(UI_SWITCH_TAP_TEMPO  , tap_tempo_press_time_ , tap_tempo_long_press_event_sent_  );

  display_.RefreshSlow();
  
  // Read LED brightness from multi and copy to LEDs driver.
  uint8_t leds_brightness[kNumSystemVoices];
  multi.GetLedsBrightness(leds_brightness);
  if (mode_ == UI_MODE_FACTORY_TESTING) {
    ++factory_testing_leds_counter_;
    uint16_t x = factory_testing_leds_counter_;
    leds_brightness[0] = (((x + 384) & 511) < 128) ? 255 : 0;
    leds_brightness[1] = (((x + 256) & 511) < 128) ? 255 : 0;
    leds_brightness[2] = (((x + 128) & 511) < 128) ? 255 : 0;
    leds_brightness[3] = (((x + 000) & 511) < 128) ? 255 : 0;
  } else if (splash_mode_ == UI_MODE_SPLASH && show_splash_) {
    leds_brightness[0] = 255;
    leds_brightness[1] = 0;
    leds_brightness[2] = 0;
    leds_brightness[3] = 0;
  }
  
  leds_.Write(leds_brightness);
  leds_.Write();
}

void Ui::PollSwitch(const UiSwitch ui_switch, uint32_t& press_time, bool& long_press_event_sent) {
  if (switches_.just_pressed(ui_switch)) {
    press_time = system_clock.milliseconds();
    long_press_event_sent = false;
  }
  if (!long_press_event_sent) {
    if (switches_.pressed(ui_switch)) {
      uint32_t duration = system_clock.milliseconds() - press_time;
      if (duration >= kEncoderLongPressTime && !long_press_event_sent) {
        queue_.AddEvent(CONTROL_SWITCH_HOLD, ui_switch, 0);
        long_press_event_sent = true;
      }
    } else if (switches_.released(ui_switch)
               && !long_press_event_sent) {
      queue_.AddEvent(CONTROL_SWITCH, ui_switch, 0);
    }
  }
}

void Ui::FlushEvents() {
  queue_.Flush();
}

// Display refresh functions.
const char* const calibration_strings[] = {
  "-3", "-2", "-1", " 0", "+1", "+2", "+3", "+4", "+5", "+6", "+7", "OK"
};

const char notes_long[] = "C d D e E F g G a A b B ";
const char octave[] = "-0123456789";

void Ui::PrintParameterName() {
  display_.Print(setting().short_name, setting().name);
}

void Ui::PrintParameterValue() {
  settings.Print(setting(), buffer_);
  display_.Print(buffer_, buffer_);
}

void Ui::PrintMenuName() {
  display_.Print(commands_[command_index_].name);
}

void Ui::PrintProgramNumber() {
  if (program_index_ < kNumPrograms) {
    strcpy(buffer_, "P1");
    buffer_[1] += program_index_;
    display_.Print(buffer_);
  } else {
    display_.Print("--");
  } 
}

void Ui::PrintCalibrationVoiceNumber() {
  if (calibration_voice_ < kNumSystemVoices) {
    strcpy(buffer_, "*1");
    buffer_[1] += calibration_voice_;
    display_.Print(buffer_);
  } else {
    display_.Print("OK");
  } 
}

void Ui::PrintCalibrationNote() {
  display_.Print(
      calibration_strings[calibration_note_],
      calibration_strings[calibration_note_]);
}

void Ui::PrintActivePartAndPlayMode() {
  uint8_t play_mode = active_part().midi_settings().play_mode;
  if (multi.running()) {
    SetBrightnessFromBarPhase();
  }
  strcpy(buffer_, "1x");
  buffer_[0] += settings.Get(GLOBAL_ACTIVE_PART);
  buffer_[1] = settings.setting(SETTING_SEQUENCER_PLAY_MODE).values[play_mode][0];
  buffer_[2] = '\0';
  display_.Print(buffer_);
}

void Ui::PrintRecordingStep() {
  SequencerStep step = recording_part().sequencer_settings().step[recording_part().recording_step()];
  if (step.is_rest()) {
    display_.Print("RS");
    return;
  }
  if (step.is_tie()) {
    display_.Print("TI");
    return;
  }
  if (active_part().midi_settings().play_mode != PLAY_MODE_ARPEGGIATOR) {
    PrintNote(step.note());
    return;
  }
  if (
    recording_part().sequencer_settings().arp_direction == ARPEGGIATOR_DIRECTION_SEQUENCER_HIT
  ) {
    PrintArpeggiatorMovementStep(step);
    return;
  }
  if (recording_part().sequencer_settings().arp_direction == ARPEGGIATOR_DIRECTION_SEQUENCER_WRAP) {
    if (step.is_white()) {
      buffer_[0] = '\x90' + step.white_key_value(); // spinner 1
    } else {
      buffer_[0] = '\x98' + step.black_key_value(); // spinner 2
    }
    buffer_[1] = octave[step.octave()];
    display_.Print(buffer_, buffer_);
    return;
  }
}

void Ui::PrintArpeggiatorMovementStep(SequencerStep step) {
  if (step.is_white()) {
    Settings::PrintSignedInteger(buffer_, step.white_key_value());
  } else {
    int8_t value = step.black_key_value();
    Settings::PrintSignedInteger(buffer_, (value >= 0 ? value + 1 : abs(value)));
    if (buffer_[0] == ' ') {
      buffer_[0] = value >= 0 ? '>' : '<';
    }
  }
  display_.Print(buffer_, buffer_);
}

void Ui::SetBrightnessFromBarPhase() {
  display_.set_brightness(UINT16_MAX - (active_part().BarPhase() >> 16));
}

void Ui::PrintLooperRecordingStatus() {
  uint8_t note_index = active_part().LooperCurrentNoteIndex();
  uint32_t pos = active_part().BarPhase();
  if (note_index == looper::kNullIndex) {
    SetBrightnessFromBarPhase();
    display_.Print("__");
    return;
  }
  const looper::Tape& looper_tape = active_part().sequencer_settings().looper_tape;
  uint16_t note_fraction_completed = looper_tape.NoteFractionCompleted(note_index, pos >> 16);
  display_.set_brightness(UINT16_MAX - note_fraction_completed);
  if (recording_mode_is_displaying_pitch_) {
    PrintNote(looper_tape.NotePitch(note_index));
  } else {
    Settings::PrintInteger(buffer_, looper_tape.NoteAgeOrdinal(note_index) + 1);
    display_.Print(buffer_);
  }
}

void Ui::PrintRecordingStatus() {
  if (push_it_) {
    PrintPushItNote();
  } else {
    if (recording_part().recording_step() == recording_part().playing_step()) {
      display_.set_brightness(UINT16_MAX);
    } else {
      // If playing a sequencer step other than the selected one, 2/3 brightness
      display_.set_brightness(43690);
    }
    if (recording_mode_is_displaying_pitch_) {
      PrintRecordingStep();
    } else {
      Settings::PrintInteger(buffer_, recording_part().recording_step() + 1);
      display_.Print(buffer_);
    }
  }
}

void Ui::PrintNote(int16_t note) {
  buffer_[0] = notes_long[2 * (note % 12)];
  buffer_[1] = notes_long[1 + 2 * (note % 12)];
  buffer_[1] = buffer_[1] == ' ' ? octave[note / 12] : buffer_[1];
  buffer_[2] = '\0';
  display_.Print(buffer_, buffer_);
}

void Ui::PrintPushItNote() {
  PrintNote(push_it_note_);
}

void Ui::PrintLearning() {
  display_.Print("++");
}

void Ui::PrintFactoryTesting() {
  switch (factory_testing_display_) {
    case UI_FACTORY_TESTING_DISPLAY_EMPTY:
      display_.Print("\xff\xff");
      break;
      
    case UI_FACTORY_TESTING_DISPLAY_NUMBER:
      {
        strcpy(buffer_, "00");
        buffer_[0] += factory_testing_number_ / 10;
        buffer_[1] += factory_testing_number_ % 10;
        display_.Print(buffer_);
      }
      break;
      
    case UI_FACTORY_TESTING_DISPLAY_CLICK:
      display_.Print("OK");
      break;
      
    case UI_FACTORY_TESTING_DISPLAY_SW_1:
    case UI_FACTORY_TESTING_DISPLAY_SW_2:
    case UI_FACTORY_TESTING_DISPLAY_SW_3:
      {
        strcpy(buffer_, "B1");
        buffer_[1] += factory_testing_display_ - UI_FACTORY_TESTING_DISPLAY_SW_1;
        display_.Print(buffer_);
      }
      break;
  }
}

void Ui::PrintVersionNumber() {
  display_.Print("L2"); // Loom v1.2.0
  display_.set_brightness(UINT16_MAX);
}

void Ui::ChangedActivePartOrPlayMode() {
  splash_mode_ = UI_MODE_CHANGED_ACTIVE_PART_OR_PLAY_MODE;
  show_splash_ = true;
}

// Generic Handlers
void Ui::OnLongClick(const Event& e) {
  switch (mode_) {
    case UI_MODE_MAIN_MENU:
      mode_ = previous_mode_;
      break;
      
    default:
      previous_mode_ = mode_;
      mode_ = UI_MODE_MAIN_MENU;
      command_index_ = 0;
      break;
  }
}

void Ui::OnClick(const Event& e) {
  if (current_menu_category_->setting_index() == SETTING_SETUP_SUBMENU) {
    current_menu_category_ = &Settings::setup_menus;
    return;
  } else if (current_menu_category_ == &Settings::setup_menus && mode_ == UI_MODE_PARAMETER_EDIT) {
    current_menu_category_ = &Settings::live_menus;
  }
  mode_ = modes_[mode_].next_mode; 
}

void Ui::OnIncrement(const Event& e) {
  Mode* mode = &modes_[mode_];
  if (!mode->incremented_variable) {
    return;
  }
  int8_t v = *mode->incremented_variable;
  v += e.data;
  CONSTRAIN(v, mode->min_value, mode->max_value);
  *mode->incremented_variable = v;
}

// Specialized Handlers
void Ui::OnClickMainMenu(const Event& e) {
  if (commands_[command_index_].function) {
    (this->*commands_[command_index_].function)();
  }
  mode_ = commands_[command_index_].next_mode;
}

void Ui::OnClickLoadSave(const Event& e) {
  if (program_index_ == kNumPrograms) {
    program_index_ = active_program_;  // Cancel
  } else {
    active_program_ = program_index_;
    if (mode_ == UI_MODE_SAVE_SELECT_PROGRAM) {
      storage_manager.SaveMulti(program_index_);
    } else {
      storage_manager.LoadMulti(program_index_);
    }
  }
  mode_ = UI_MODE_PARAMETER_SELECT;
}

void Ui::OnClickCalibrationSelectVoice(const Event& e) {
  if (calibration_voice_ == kNumSystemVoices) {
    mode_ = UI_MODE_PARAMETER_SELECT;
    calibration_voice_ = 0;
    storage_manager.SaveCalibration();
  } else {
    mode_ = UI_MODE_CALIBRATION_SELECT_NOTE;
  }
  calibration_note_ = 0;
}

void Ui::OnClickCalibrationSelectNote(const Event& e) {
  if (calibration_note_ == kNumOctaves) {
    mode_ = UI_MODE_CALIBRATION_SELECT_VOICE;
    calibration_note_ = 0;
  } else {
    mode_ = UI_MODE_CALIBRATION_ADJUST_LEVEL;
  }
}

void Ui::OnClickRecording(const Event& e) {
  if (push_it_) {
    multi.PushItNoteOff(push_it_note_);
    push_it_ = false;
    mutable_recording_part()->RecordStep(SequencerStep(push_it_note_, 100));
  } else {
    multi.PushItNoteOn(push_it_note_);
    push_it_ = true;
  }
}

void Ui::OnClickOverdubbing(const Event& e) {
  if (push_it_) {
    push_it_ = false;
    mutable_recording_part()->RecordStep(SequencerStep(push_it_note_, 100));
  } else {
    push_it_ = true;
  }
}

void Ui::OnClickLearning(const Event& e) {
  multi.StopLearning();
  mode_ = UI_MODE_PARAMETER_SELECT;
}

void Ui::OnClickFactoryTesting(const Event& e) {
  factory_testing_display_ = UI_FACTORY_TESTING_DISPLAY_CLICK;
}

void Ui::OnIncrementParameterSelect(const Event& e) {
  current_menu_category_->increment_index(e.data);
}

void Ui::OnIncrementParameterEdit(const stmlib::Event& e) {
  settings.Increment(current_menu_category_->setting(), e.data);
}

void Ui::OnIncrementCalibrationAdjustment(const stmlib::Event& e) {
  CVOutput* voice = multi.mutable_cv_output(calibration_voice_);
  int32_t code = voice->calibration_dac_code(calibration_note_);
  code -= e.data * (switches_.pressed(2) ? 32 : 1);
  CONSTRAIN(code, 0, 65535);
  voice->set_calibration_dac_code(calibration_note_, code);
}

void Ui::OnIncrementRecording(const stmlib::Event& e) {
  if (push_it_) {
    OnIncrementPushItNote(e);
  } else {
    mutable_recording_part()->increment_recording_step_index(e.data);
  }
}

void Ui::OnIncrementOverdubbing(const stmlib::Event& e) {
  if (push_it_) {
    push_it_note_ += e.data;
    CONSTRAIN(push_it_note_, 0, 127);
    mutable_recording_part()->ModifyNoteAtCurrentStep(push_it_note_);
  } else {
    mutable_recording_part()->increment_recording_step_index(e.data);
  }
}

void Ui::OnIncrementPushItNote(const stmlib::Event& e) {
  int16_t previous_note = push_it_note_;
  push_it_note_ += e.data;
  CONSTRAIN(push_it_note_, 0, 127);
  if (push_it_note_ != previous_note) {
    multi.PushItNoteOn(push_it_note_);
    multi.PushItNoteOff(previous_note);
  }
}

void Ui::OnIncrementFactoryTesting(const Event& e) {
  factory_testing_display_ = UI_FACTORY_TESTING_DISPLAY_NUMBER;
  OnIncrement(e);
}

void Ui::StartRecording() {
  previous_mode_ = mode_;
  multi.StartRecording(settings.Get(GLOBAL_ACTIVE_PART));
  if (active_part().sequencer_settings().clock_quantization == 0) {
    mode_ = UI_MODE_LOOPER_RECORDING;
    multi.Start(false);
  } else {
    mode_ = active_part().overdubbing() ?
          UI_MODE_OVERDUBBING : UI_MODE_RECORDING;
  }
}

void Ui::StopRecording() {
  push_it_ = false;
  multi.StopRecording(settings.Get(GLOBAL_ACTIVE_PART));
  mode_ = previous_mode_;
}

void Ui::OnSwitchPress(const Event& e) {
  if (mode_ == UI_MODE_FACTORY_TESTING) {
    factory_testing_display_ = static_cast<UiFactoryTestingDisplay>(
        UI_FACTORY_TESTING_DISPLAY_SW_1 + e.control_id);
    return;
  }
  
  switch (e.control_id) {
    case UI_SWITCH_REC:
      {
        if (multi.recording()) {
          if (recording_mode_is_displaying_pitch_) {
            StopRecording();
          } else {
            // Toggle pitch display on
            recording_mode_is_displaying_pitch_ = true;
          }
        } else if (active_part().midi_settings().play_mode != PLAY_MODE_MANUAL) {
          recording_mode_is_displaying_pitch_ = false;
          StartRecording();
        }
      }
      break;
      
    case UI_SWITCH_START_STOP:
      if (mode_ == UI_MODE_RECORDING || mode_ == UI_MODE_OVERDUBBING) {
        if (push_it_ && mode_ == UI_MODE_RECORDING) {
          multi.PushItNoteOff(push_it_note_);
        }
        push_it_ = false;
        mutable_active_part()->RecordStep(SEQUENCER_STEP_TIE);
      } else if (mode_ == UI_MODE_LOOPER_RECORDING) {
        mutable_active_part()->LooperRemoveOldestNote();
      } else {
        if (!multi.running()) {
          multi.Start(false);
          if (multi.paques()) {
            multi.StartSong();
          }
        } else {
          multi.Stop();
        }
      }
      break;
      
    case UI_SWITCH_TAP_TEMPO:
      if (mode_ == UI_MODE_RECORDING || mode_ == UI_MODE_OVERDUBBING) {
        if (push_it_ && mode_ == UI_MODE_RECORDING) {
          multi.PushItNoteOff(push_it_note_);
        }
        push_it_ = false;
        mutable_active_part()->RecordStep(SEQUENCER_STEP_REST);
      } else if (mode_ == UI_MODE_LOOPER_RECORDING) {
        mutable_active_part()->LooperRemoveNewestNote();
      } else {
        TapTempo();
      }
      break;
  }
}

void Ui::OnSwitchHeld(const Event& e) {
  bool recording_any = (
    mode_ == UI_MODE_RECORDING ||
    mode_ == UI_MODE_OVERDUBBING ||
    mode_ == UI_MODE_LOOPER_RECORDING
  );
  switch (e.control_id) {

    case UI_SWITCH_REC:
      if (multi.recording()) {
        mutable_active_part()->DeleteRecording();
      } else {
        if (active_part().IsLatched()) {
          mutable_active_part()->UnlatchOnNextNoteOn();
        } else if (multi.running() && active_part().has_notes()) {
          mutable_active_part()->Latch();
        } else {
          if (push_it_) {
            multi.PushItNoteOff(push_it_note_);
            push_it_ = false;
            if (mode_ == UI_MODE_PUSH_IT_SELECT_NOTE) {
              mode_ = UI_MODE_PARAMETER_SELECT;
            }
          } else {
            mode_ = UI_MODE_PUSH_IT_SELECT_NOTE;
            push_it_ = true;
            multi.PushItNoteOn(push_it_note_);
          }
        }
      }
      break;

    case UI_SWITCH_START_STOP:
      if (recording_any) {
        StopRecording();
      }
      settings.Set(GLOBAL_ACTIVE_PART, (1 + settings.Get(GLOBAL_ACTIVE_PART)) % multi.num_active_parts());
      if (recording_any) {
        StartRecording();
      }
      ChangedActivePartOrPlayMode();
      break;

    case UI_SWITCH_TAP_TEMPO:
      // Use this to set last step for sequencer?
      if (!recording_any) {
        mutable_active_part()->Set(PART_MIDI_PLAY_MODE, (1 + active_part().midi_settings().play_mode) % PLAY_MODE_LAST);
        ChangedActivePartOrPlayMode();
      }
      break;

    default:
      break;
  }
}

void Ui::DoInitCommand() {
  multi.Init(false);
}

void Ui::DoDumpCommand() {
  storage_manager.SysExSendMulti();
}

void Ui::DoLearnCommand() {
  multi.StartLearning();
}

void Ui::TapTempo() {
  uint32_t tap_time = system_clock.milliseconds();
  uint32_t delta = tap_time - previous_tap_time_;
  if (delta < 1500) {
    if (delta < 250) {
      delta = 250;
    }
    ++tap_tempo_count_;
    tap_tempo_sum_ += delta;
    multi.Set(MULTI_CLOCK_TEMPO, tap_tempo_count_ * 60000 / tap_tempo_sum_);
  } else {
    tap_tempo_count_ = 0;
    tap_tempo_sum_ = 0;
  }
  previous_tap_time_ = tap_time;
}

void Ui::DoEvents() {
  bool refresh_display = false;
  bool scroll_display = false;
  
  while (queue_.available()) {
    Event e = queue_.PullEvent();
    const Mode& mode = modes_[mode_];
    if (e.control_type == CONTROL_ENCODER_CLICK) {
      (this->*mode.on_click)(e);
      if (mode_ == UI_MODE_PARAMETER_EDIT) {
        scroll_display = true;
      }
    } else if (e.control_type == CONTROL_ENCODER) {
      (this->*mode.on_increment)(e);
      scroll_display = true;
    } else if (e.control_type == CONTROL_ENCODER_LONG_CLICK) {
      OnLongClick(e);
    } else if (e.control_type == CONTROL_SWITCH) {
      OnSwitchPress(e);
    } else if (e.control_type == CONTROL_SWITCH_HOLD) {
      OnSwitchHeld(e);
    }
    refresh_display = true;
  }
  if (queue_.idle_time() > 300 && show_splash_) {
    refresh_display = true;
    show_splash_ = false;
    if (mode_ == UI_MODE_PARAMETER_EDIT) {
      scroll_display = true;
    }
  }
  if (queue_.idle_time() > 900) {
    if (!display_.scrolling()) {
      factory_testing_display_ = UI_FACTORY_TESTING_DISPLAY_EMPTY;
      refresh_display = true;
    }
  }
  bool print_latch = active_part().IsLatched();
  bool print_part = !display_.scrolling() && mode_ == UI_MODE_PARAMETER_SELECT;
  if (queue_.idle_time() > 600) {
    if (print_part) {
      PrintActivePartAndPlayMode();
    } else if (print_latch) {
      display_.Print("//");
    }
  } else if (queue_.idle_time() > 300) {
    if (print_latch && print_part) {
      display_.Print("//");
    }
  }

  if (multi.recording()) {
    refresh_display = true;
  }

  if (mode_ == UI_MODE_LEARNING && !multi.learning()) {
    OnClickLearning(Event());
  }

  if (show_splash_) {
    // TODO hypothesis: UI never actually stays in CHANGED mode, just relies
    // on residual display until something else causes an update.  Brightness
    // also doesn't update during this period
    (this->*modes_[splash_mode_].refresh_display)();
  } else if (refresh_display) {
    queue_.Touch();
    (this->*modes_[mode_].refresh_display)();
    if (multi.recording()) {
      // Brightness set in PrintLooperRecordingStatus
    } else {
      display_.set_brightness(UINT16_MAX);
    }
    if (scroll_display) {
      display_.Scroll();
    }
    display_.set_blink(
        mode_ == UI_MODE_CALIBRATION_ADJUST_LEVEL ||
        mode_ == UI_MODE_LEARNING
    );
    if (mode_ == UI_MODE_MAIN_MENU) {
      display_.set_fade(160);
    } else if (mode_ == UI_MODE_PARAMETER_EDIT &&
               setting().unit == SETTING_UNIT_TEMPO) {
      display_.set_fade(multi.tempo() * 235 >> 8);
    } else {
      display_.set_fade(0);
    }
  }
}

}  // namespace yarns
