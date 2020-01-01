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

#include "yarns/looper.h"

#include "yarns/part.h"

namespace yarns {

namespace looper {

void Deck::Rewind() {
  synced_lfo_.Init();
  pos_ = 0;
  needs_advance_ = false;
  ResetHead();
  std::fill(
    &note_index_for_pressed_key_index_[0],
    &note_index_for_pressed_key_index_[kNoteStackSize],
    kNullIndex
  );
}

void Deck::RemoveAll() {
  std::fill(
    &tape().notes[0],
    &tape().notes[kMaxNotes],
    Note()
  );
  head_link_.on_index = head_link_.off_index = kNullIndex;
  tape().oldest_index = tape().newest_index = 0;
}

void Deck::ResetHead() {
  uint8_t next_index;

  while (true) {
    next_index = PeekNextOn();
    if (
      next_index == kNullIndex ||
      tape().notes[head_link_.on_index].on_pos >= tape().notes[next_index].on_pos
    ) {
      break;
    }
    head_link_.on_index = next_index;
  }

  while (true) {
    next_index = PeekNextOff();
    if (
      next_index == kNullIndex ||
      tape().notes[head_link_.off_index].off_pos >= tape().notes[next_index].off_pos
    ) {
      break;
    }
    head_link_.off_index = next_index;
  }
}

void Deck::RemoveOldestNote() {
  RemoveNote(tape().oldest_index);
  if (!IsEmpty()) {
    tape().oldest_index = stmlib::modulo(tape().oldest_index + 1, kMaxNotes);
  }
}

void Deck::RemoveNewestNote() {
  RemoveNote(tape().newest_index);
  if (!IsEmpty()) {
    tape().newest_index = stmlib::modulo(tape().newest_index - 1, kMaxNotes);
  }
}

uint8_t Deck::PeekNextOn() {
  if (head_link_.on_index == kNullIndex) {
    return kNullIndex;
  }
  return tape().notes[head_link_.on_index].next_link.on_index;
}

uint8_t Deck::PeekNextOff() {
  if (head_link_.off_index == kNullIndex) {
    return kNullIndex;
  }
  return tape().notes[head_link_.off_index].next_link.off_index;
}

void Deck::Advance() {
  if (!needs_advance_) {
    return;
  }

  uint16_t new_pos = synced_lfo_.GetPhase() >> 16;
  bool play = (part_->sequencer_settings().play_mode == PLAY_MODE_LOOPER);

  uint8_t seen_index;
  uint8_t next_index;

  seen_index = looper::kNullIndex;
  while (true) {
    next_index = PeekNextOff();
    if (next_index == kNullIndex || next_index == seen_index) {
      break;
    }
    if (seen_index == kNullIndex) {
      seen_index = next_index;
    }
    Note& next_note = tape().notes[next_index];
    if (!Passed(next_note.off_pos, pos_, new_pos)) {
      break;
    }
    head_link_.off_index = next_index;

    if (play) {
      part_->InternalNoteOff(next_note.pitch);
    }
  }

  seen_index = looper::kNullIndex;
  while (true) {
    next_index = PeekNextOn();
    if (next_index == kNullIndex || next_index == seen_index) {
      break;
    }
    if (seen_index == kNullIndex) {
      seen_index = next_index;
    }
    Note& next_note = tape().notes[next_index];
    if (!Passed(next_note.on_pos, pos_, new_pos)) {
      break;
    }
    head_link_.on_index = next_index;

    if (next_note.next_link.off_index == kNullIndex) {
      // If the next 'on' note doesn't yet have an off_index, it's still held
      // and has been for an entire loop -- instead of redundantly turning the
      // note on, set an off_pos
      InsertOff(next_note.on_pos, next_index);
      for (uint8_t i = 0; i < kNoteStackSize; ++i) {
        if (note_index_for_pressed_key_index_[i] == next_index) {
          note_index_for_pressed_key_index_[i] = looper::kNullIndex;
        }
      }
      continue;
    }

    if (play) {
      part_->InternalNoteOn(next_note.pitch, next_note.velocity);
    }
  }

  pos_ = new_pos;
  needs_advance_ = false;
}

void Deck::RecordNoteOn(uint8_t pressed_key_index, uint8_t pitch, uint8_t velocity) {
  if (!IsEmpty()) {
    tape().newest_index = stmlib::modulo(1 + tape().newest_index, kMaxNotes);
  }
  if (tape().newest_index == tape().oldest_index) {
    RemoveOldestNote();
  }

  note_index_for_pressed_key_index_[pressed_key_index] = tape().newest_index;

  part_->InternalNoteOn(pitch, velocity);
  InsertOn(pos_, tape().newest_index);

  Note& note = tape().notes[tape().newest_index];
  note.pitch = pitch;
  note.velocity = velocity;
  note.off_pos = 0;
  note.next_link.off_index = kNullIndex;
}

void Deck::RecordNoteOff(uint8_t pressed_key_index) {
  uint8_t index = note_index_for_pressed_key_index_[pressed_key_index];
  if (index == kNullIndex) {
    // InsertOff was already called by Advance, so the note was held for an
    // entire loop, so the note should play continuously and not get turned off
    return;
  }

  note_index_for_pressed_key_index_[pressed_key_index] = looper::kNullIndex;

  part_->InternalNoteOff(tape().notes[index].pitch);
  InsertOff(pos_, index);
}

inline Tape& Deck::tape() { return part_->mutable_sequencer_settings()->looper_tape; }

bool Deck::Passed(uint16_t target, uint16_t before, uint16_t after) {
  if (before < after) {
    return (target > before and target <= after);
  } else {
    return (target > before or target <= after);
  }
}

void Deck::InsertOn(uint16_t pos, uint8_t index) {
  Note& note = tape().notes[index];
  note.on_pos = pos;
  if (head_link_.on_index == kNullIndex) {
    // there is no prev note to link to this one, so link it to itself
    note.next_link.on_index = index;
  } else {
    Note& head_note = tape().notes[head_link_.on_index];
    note.next_link.on_index = head_note.next_link.on_index;
    head_note.next_link.on_index = index;
  }
  head_link_.on_index = index;
}

void Deck::InsertOff(uint16_t pos, uint8_t index) {
  Note& note = tape().notes[index];
  note.off_pos = pos;
  if (head_link_.off_index == kNullIndex) {
    // there is no prev note to link to this one, so link it to itself
    note.next_link.off_index = index;
  } else {
    Note& head_note = tape().notes[head_link_.off_index];
    note.next_link.off_index = head_note.next_link.off_index;
    head_note.next_link.off_index = index;
  }
  head_link_.off_index = index;
}

void Deck::RemoveNote(uint8_t target_index) {
  if (IsEmpty()) {
    return;
  }

  Note& target_note = tape().notes[target_index];
  bool target_has_off = (target_note.next_link.off_index != kNullIndex);
  uint8_t search_prev_index;
  uint8_t search_next_index;

  if (target_has_off && Passed(pos_, target_note.on_pos, target_note.off_pos)) {
    // If this note was completely recorded and the looper is currently playing
    // the note, turn it off
    part_->InternalNoteOff(target_note.pitch);
  }

  // general concern -- next index never matches target
  // bc next index points into a separate cycle of notes
  // most likely that cycle would be a single uninitialized note, e.g. if
  // target_note has kNullIndex for its next index

  search_prev_index = target_index;
  while (true) {
    search_next_index = tape().notes[search_prev_index].next_link.on_index;
    if (search_next_index == target_index) {
      break;
    }
    search_prev_index = search_next_index;
  }
  tape().notes[search_prev_index].next_link.on_index = target_note.next_link.on_index;
  target_note.next_link.on_index = kNullIndex; // unneeded?
  if (target_index == search_prev_index) {
    // If this was the last note
    head_link_.on_index = kNullIndex;
  } else if (target_index == head_link_.on_index) {
    head_link_.on_index = search_prev_index;
  }

  if (!target_has_off) {
    // Don't try to relink off_index
    return;
  }

  search_prev_index = target_index;
  while (true) {
    search_next_index = tape().notes[search_prev_index].next_link.off_index;
    if (search_next_index == target_index) {
      break;
    }
    search_prev_index = search_next_index;
  }
  tape().notes[search_prev_index].next_link.off_index = target_note.next_link.off_index;
  target_note.next_link.off_index = kNullIndex;
  if (target_index == search_prev_index) {
    // If this was the last note
    head_link_.off_index = kNullIndex;
  } else if (target_index == head_link_.off_index) {
    head_link_.off_index = search_prev_index;
  }
}

} // namespace looper
}  // namespace yarns
