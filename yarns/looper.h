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

namespace yarns {

class Part;

namespace looper {

const uint8_t kMaxNotes = 16;
const uint8_t kNullIndex = UINT8_MAX;

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

class Tape {
 public:

  Tape() { }
  ~Tape() { }

  void RemoveAll();
  bool IsEmpty() const {
    return (head_link_.on_index == kNullIndex);
  }
  void ResetHead();

  void RemoveOldestNote(Part* part, uint16_t current_pos);
  void RemoveNewestNote(Part* part, uint16_t current_pos);
  void Advance(Part* part, uint16_t old_pos, uint16_t new_pos);
  uint8_t RecordNoteOn(Part* part, uint16_t pos, uint8_t pitch, uint8_t velocity);
  bool RecordNoteOff(uint16_t pos, uint8_t index);
  uint8_t PeekNextOn() const;
  uint8_t PeekNextOff() const;

  bool NoteIsPlaying(uint8_t index, uint16_t pos) const;
  uint16_t NoteFractionCompleted(uint8_t index, uint16_t pos) const;
  uint8_t NotePitch(uint8_t index) const;
  uint8_t NoteAgeOrdinal(uint8_t index) const;

  const Note& NoteAt(uint8_t index) const {
    return notes_[index];
  }

 private:

  bool Passed(uint16_t target, uint16_t before, uint16_t after) const;
  void InsertOn(uint16_t pos, uint8_t index);
  void InsertOff(uint16_t pos, uint8_t index);
  void RemoveNote(Part* part, uint16_t current_pos, uint8_t index);

  Note notes_[kMaxNotes];
  Link head_link_;
  uint8_t oldest_index_;
  uint8_t newest_index_;

  DISALLOW_COPY_AND_ASSIGN(Tape);
};

} // namespace looper
}  // namespace yarns

#endif // YARNS_LOOPER_H_
