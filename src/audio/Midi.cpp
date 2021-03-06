#include "Midi.h"

using namespace L;

static Midi* instance(nullptr);

Midi::Midi() {
  instance = this;
}
void Midi::send(const MidiEvent& e) {
  if(instance) {
    instance->send_internal(e);
  }
}
void Midi::set_instrument(uint8_t channel, uint8_t instrument) {
  MidiEvent e;
  e.msg.type = MidiEvent::ProgramChange;
  e.msg.channel = channel;
  e.msg.instrument = instrument;
  send(e);
}
void Midi::play_note(uint8_t channel, uint8_t note, uint8_t velocity) {
  MidiEvent e;
  e.msg.type = MidiEvent::NoteOn;
  e.msg.channel = channel;
  e.msg.note = note;
  e.msg.velocity = velocity;
  send(e);
}
void Midi::stop_note(uint8_t channel, uint8_t note) {
  MidiEvent e;
  e.msg.type = MidiEvent::NoteOff;
  e.msg.channel = channel;
  e.msg.note = note;
  send(e);
}
void Midi::stop_all(uint8_t channel) {
  for(uint8_t t(0); t<128; t++)
    stop_note(channel, t);
}
void Midi::stop_all() {
  for(uint8_t c(0); c<16; c++)
    stop_all(c);
}
