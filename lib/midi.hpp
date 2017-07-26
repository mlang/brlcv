#if !defined(BrlCV_MIDI_HPP)
#define BrlCV_MIDI_HPP

namespace MIDI {

enum class SystemRealTimeMessage {
  Clock         = 0b11111000,
  Start         = 0b11111010,
  Continue      = 0b11111011,
  Stop          = 0b11111100,
  ActiveSensing = 0b11111110,
  Reset         = 0b11111111
};

}

#endif // BrlCV_MIDI_HPP
