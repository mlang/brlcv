#if !defined(BrlCV_BrlAPI_HPP)
#define BrlCV_BrlAPI_HPP

#include <ostream>
#include <sstream>
#include <gsl/gsl>

namespace BrlAPI {

struct DisplaySize {
  unsigned int X;
  unsigned int Y;
};

bool operator==(DisplaySize const &, DisplaySize const &);
bool operator!=(DisplaySize const &, DisplaySize const &);
bool operator>(DisplaySize const &, DisplaySize const &);
bool operator>=(DisplaySize const &, DisplaySize const &);
bool operator<=(DisplaySize const &, DisplaySize const &);
bool operator<(DisplaySize const &, DisplaySize const &);
std::ostream &operator<<(std::ostream &, DisplaySize const &);

class Connection;

class KeyCode {
  std::uint8_t Group, Number;
  bool Press;
  friend class TTY;

  KeyCode(std::uint64_t Code)
  : Group((Code >> 8) & 0b11111111)
  , Number(Code & 0b11111111)
  , Press(Code >> 63) {}

public:
  auto group() const noexcept { return Group; }
  auto number() const noexcept { return Number; }
  auto press() const noexcept { return Press; }
};

class TTY {
  Connection *Conn;
  int Number;
  friend class Connection;

  TTY(Connection *Conn, int Number) : Conn(Conn), Number(Number) {}

public:
  ~TTY();

  TTY(TTY const &) = delete;
  TTY(TTY &&) noexcept = default;

  TTY &operator=(TTY const &) = delete;
  TTY &operator=(TTY &&) noexcept = default;

  int number() const noexcept { return Number; }

  void writeText(std::string);
  void writeText(std::stringstream const &Stream) { writeText(Stream.str()); }

  KeyCode readKey() const;
  bool readKey(KeyCode &) const;
};

class Connection {
  class Implementation;
  std::unique_ptr<Implementation> BrlAPI;
  friend class TTY;

public:
  Connection();
  ~Connection();

  Connection(Connection const &) = delete;
  Connection &operator=(Connection const &) = delete;

  Connection(Connection &&) noexcept;
  Connection &operator=(Connection &&) noexcept;

  std::string driverName() const;
  DisplaySize displaySize() const;

  TTY tty(int, bool);
};

} // namespace BrlAPI

#endif // BrlCV_BrlAPI_HPP
