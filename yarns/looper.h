// Copyright 2019 Chris Rogers.
//
// Author: Chris Rogers (teukros@gmail.com)
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
// Looper.

#ifndef YARNS_LOOPER_H_
#define YARNS_LOOPER_H_

#include "stmlib/stmlib.h"
#include <algorithm>

#include "yarns/synced_lfo.h"

namespace yarns {

class Part;

namespace looper {

const uint8_t kMaxNotes = 16;
const uint8_t kNullIndex = UINT8_MAX;
const uint8_t kNoteStackSize = 12; //TODO

struct Link {
  Link() {
    on_index = off_index = kNullIndex;
  }
  uint8_t on_index;
  uint8_t off_index;
};

struct Note {
  Note() { }
  Link next_link;
  uint16_t on_pos;
  uint16_t off_pos;
  uint8_t pitch;
  uint8_t velocity;
};

struct Tape {
  // persistent storage
  // 130 bytes
  // could recoup 33 bytes by packing 4-bit index and 12-bit pos
  // TODO also links don't need to be in storage, they can be reconstructed
  // TODO also links might not be needed at all?
  Note notes[kMaxNotes];
  uint8_t oldest_index;
  uint8_t newest_index;
};

class Deck {
 public:

  Deck() { }
  ~Deck() { }

  void Init(Part* part);
  
  inline void Refresh() {
    synced_lfo_.Refresh();
    needs_advance_ = true;
  }
  
  inline void Tap(uint32_t target_phase) {
    synced_lfo_.Tap(target_phase);
  }

  void Rewind();
  void RemoveAll();

  //TODO glitches caused by:
  // remove -- occasionally causes eternal hang -- try lowering kMaxNotes to see if wrapping issue?
  // dramatically slowing tap tempo
  // adjusting clock div either way

  void RemoveOldestNote();
  void RemoveNewestNote();
  void Advance();
  void RecordNoteOn(uint8_t pressed_key_index, uint8_t pitch, uint8_t velocity);
  void RecordNoteOff(uint8_t pressed_key_index);

 private:

  const Note& note(uint8_t index) const { return tape_->notes[index]; }
  Note* mutable_note(uint8_t index) { return &tape_->notes[index]; }
  void ResetHead();
  bool Passed(uint16_t target, uint16_t before, uint16_t after);
  uint8_t PeekNextOn();
  uint8_t PeekNextOff();
  void InsertOn(uint16_t pos, uint8_t index);
  void InsertOff(uint16_t pos, uint8_t index);
  void RemoveNote(uint8_t index);
  bool IsEmpty() {
    return (head_link_.on_index == kNullIndex);
  }

  Part* part_;
  Tape* tape_;

  Link head_link_;
  SyncedLFO synced_lfo_;
  uint16_t pos_;
  bool needs_advance_;
  uint8_t note_index_for_pressed_key_index_[kNoteStackSize];

  DISALLOW_COPY_AND_ASSIGN(Deck);
};

} // namespace looper
}  // namespace yarns

#endif // YARNS_LOOPER_H_
