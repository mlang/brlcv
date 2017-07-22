#include <ostream>
#include <system_error>
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

class Connection {
  class Implementation;
  std::unique_ptr<Implementation> BrlAPI;
public:
  Connection();
  ~Connection();
  std::string driverName() const;
  DisplaySize displaySize() const;
};

}
