#if !defined(BrlCV_MIDI_HPP)
#define BrlCV_MIDI_HPP

#include <cstddef>
#include <cstdint>

#include <gsl/gsl>

namespace MIDI {

class SongPositionPointer : public gsl::span<std::byte const, 3> {
  std::array<std::byte, 3> Storage;
public:
  explicit SongPositionPointer(std::uint16_t Position)
  : Storage { static_cast<std::byte>(0XF2)
            , static_cast<std::byte>(Position & 0X7F)
            , static_cast<std::byte>((Position >> 7) & 0X7F)
            }
  , gsl::span<std::byte const, 3> { Storage }
  {
    Expects(Position <= 0b1111111'1111111);
  }
  SongPositionPointer &operator=(std::uint16_t Position)
  {
    Expects(Position <= 0b1111111'1111111);
    Storage[1] = static_cast<std::byte>(Position & 0X7F);
    Storage[2] = static_cast<std::byte>((Position >> 7) & 0X7F);
  }
};

enum class SystemRealTimeMessage {
  Clock         = 0b11111'000,
  Start         = 0b11111'010,
  Continue      = 0b11111'011,
  Stop          = 0b11111'100,
  ActiveSensing = 0b11111'110,
  Reset         = 0b11111'111
};

} // namespace MIDI

#endif // BrlCV_MIDI_HPP
