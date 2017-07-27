#if !defined(BrlCV_MIDI_HPP)
#define BrlCV_MIDI_HPP

namespace MIDI {

enum class SystemRealTimeMessage {
  Clock         = 0b11111'000,
  Start         = 0b11111'010,
  Continue      = 0b11111'011,
  Stop          = 0b11111'100,
  ActiveSensing = 0b11111'110,
  Reset         = 0b11111'111
};

}

#endif // BrlCV_MIDI_HPP
