#if !defined(BrlCV_MIDI_HPP)
#define BrlCV_MIDI_HPP

#include <cstddef>
#include <cstdint>

#include <gsl/gsl>

namespace MIDI {

class SongPositionPointer {
  std::array<std::byte, 3> Storage;
public:
  explicit SongPositionPointer(int Position)
  : Storage { static_cast<std::byte>(0XF2)
            , static_cast<std::byte>(Position & 0X7F)
            , static_cast<std::byte>((Position >> 7) & 0X7F)
            }
  {
    Expects(Position >= 0);
    Expects(Position <= 0b1111111'1111111);
    Ensures(*this == Position);
  }
  explicit SongPositionPointer(gsl::span<std::byte> Span) : Storage() {
    Expects(Span.size() == 3);
    Expects(Span[0] == std::byte(0XF2));
    Expects(((Span[1] & std::byte(0X80)) | (Span[2] & std::byte(0X80))) == std::byte(0));
    std::copy(Span.begin(), Span.end(), Storage.begin());
  }
  SongPositionPointer &operator=(int Position)
  {
    Expects(Position >= 0);
    Expects(Position <= (1 << 7*2) - 1);
    Storage[1] = static_cast<std::byte>(Position & 0X7F);
    Storage[2] = static_cast<std::byte>((Position >> 7) & 0X7F);

    Ensures(*this == Position);

    return *this;
  }
  operator int() const noexcept {
    return static_cast<int>(Storage[2]) << 7
         | static_cast<int>(Storage[1]);
  }
  auto begin() const { return Storage.cbegin(); }
  auto end() const { return Storage.end(); }
  auto size() const noexcept { return Storage.size(); }
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
