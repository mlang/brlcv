#include <ostream>
#include <sstream>
#include <gsl/gsl>

namespace BrlAPI {

struct DisplaySize {
  unsigned int X;
  unsigned int Y;
};

inline bool operator==(DisplaySize const &Lhs, DisplaySize const &Rhs) {
  return Lhs.X == Rhs.X && Lhs.Y == Rhs.Y;
}

inline bool operator!=(DisplaySize const &Lhs, DisplaySize const &Rhs) {
  return Lhs.X != Rhs.X || Lhs.Y != Rhs.Y;
}

inline bool operator>(DisplaySize const &Lhs, DisplaySize const &Rhs) {
  return Lhs.X * Lhs.Y > Rhs.X * Rhs.Y;
}

inline bool operator>=(DisplaySize const &Lhs, DisplaySize const &Rhs) {
  return Lhs.X * Lhs.Y >= Rhs.X * Rhs.Y;
}

inline bool operator<=(DisplaySize const &Lhs, DisplaySize const &Rhs) {
  return Lhs.X * Lhs.Y <= Rhs.X * Rhs.Y;
}

inline bool operator<(DisplaySize const &Lhs, DisplaySize const &Rhs) {
  return Lhs.X * Lhs.Y < Rhs.X * Rhs.Y;
}

inline std::ostream &operator<<(std::ostream &Out, DisplaySize const &Size) {
  Out << Size.X << 'x' << Size.Y;
  return Out;
}

class Connection;

class TTY {
  Connection *Conn;
  int Number;
  friend class Connection;

  TTY(Connection *Conn, int Number) : Conn(Conn), Number(Number) {}

public:
  ~TTY();

  TTY(TTY const &) = delete;
  TTY(TTY &&) = default;

  TTY &operator=(TTY const &) = delete;
  TTY &operator=(TTY &&) = default;

  int number() const { return Number; }

  void writeText(std::string);
  void writeText(std::stringstream const &Stream) { writeText(Stream.str()); }
};

class Connection {
  class Implementation;
  std::unique_ptr<Implementation> BrlAPI;
  friend class TTY;

public:
  Connection();
  ~Connection();

  std::string driverName() const;
  DisplaySize displaySize() const;

  TTY tty(int, bool);
};

}
