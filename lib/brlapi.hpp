#if !defined(BrlCV_BrlAPI_HPP)
#define BrlCV_BrlAPI_HPP

#include <ostream>
#include <sstream>
#include <variant>

#include <experimental/propagate_const>

#include <gsl/gsl>

#include <boost/serialization/strong_typedef.hpp>

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

public:
  KeyCode(std::uint8_t Group, std::uint8_t Number, bool Press) noexcept
  : Group(Group), Number(Number), Press(Press) {}
  explicit KeyCode(std::uint64_t Code) noexcept
  : Group((Code >> 8) & 0b11111111)
  , Number(Code & 0b11111111)
  , Press(Code >> 63) {}

  auto group() const noexcept { return Group; }
  auto number() const noexcept { return Number; }
  auto press() const noexcept { return Press; }
};

namespace Driver {

class HandyTech {
public:
  enum class NavigationKey : std::uint8_t {
    B1, B2, B3, B4, B5, B6, B7, B8, LeftSpace, RightSpace
  };
  BOOST_STRONG_TYPEDEF(std::uint8_t, RoutingKey)
  using Key = std::variant<NavigationKey, RoutingKey>;
  static Key fromKeyCode(KeyCode const &);
};

} // namespace Driver

class TTY {
  Connection &Conn;
  int Number;
  friend class Connection;

  TTY(Connection &Conn, int Number) : Conn(Conn), Number(Number) {}

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
  std::experimental::propagate_const<std::unique_ptr<Implementation>> BrlAPI;
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

  using Driver = std::variant<Driver::HandyTech>;
};

} // namespace BrlAPI

#endif // BrlCV_BrlAPI_HPP
