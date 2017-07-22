#include <ostream>
#include <system_error>
#include <gsl/gsl>
#include <brlapi.h>

namespace BrlAPI {

struct DisplaySize {
  unsigned int X;
  unsigned int Y;
};

bool operator==(DisplaySize const &Lhs, DisplaySize const &Rhs) {
  return Lhs.X == Rhs.X && Lhs.Y == Rhs.Y;
}

bool operator!=(DisplaySize const &Lhs, DisplaySize const &Rhs) {
  return Lhs.X != Rhs.X || Lhs.Y != Rhs.Y;
}

bool operator>(DisplaySize const &Lhs, DisplaySize const &Rhs) {
  return Lhs.X * Lhs.Y > Rhs.X * Rhs.Y;
}

bool operator>=(DisplaySize const &Lhs, DisplaySize const &Rhs) {
  return Lhs.X * Lhs.Y >= Rhs.X * Rhs.Y;
}

bool operator<=(DisplaySize const &Lhs, DisplaySize const &Rhs) {
  return Lhs.X * Lhs.Y <= Rhs.X * Rhs.Y;
}

bool operator<(DisplaySize const &Lhs, DisplaySize const &Rhs) {
  return Lhs.X * Lhs.Y < Rhs.X * Rhs.Y;
}

std::ostream &operator<<(std::ostream &Out, DisplaySize const &Size) {
  Out << Size.X << 'x' << Size.Y;
  return Out;
}

class Connection {
  std::unique_ptr<gsl::byte[]> HandleStorage;
  brlapi_handle_t *handle() const {
    return static_cast<brlapi_handle_t *>(static_cast<void *>(HandleStorage.get()));
  }
public:
  Connection() : HandleStorage(new gsl::byte[brlapi_getHandleSize()])
  {
    brlapi_connectionSettings_t Settings = BRLAPI_SETTINGS_INITIALIZER;
    auto FileDescriptor = brlapi__openConnection(handle(), &Settings, &Settings);
    if (FileDescriptor == -1) {
      throw std::system_error(errno, std::generic_category());
    }
  }
  ~Connection() {
    brlapi__closeConnection(handle());
  }
  std::string driverName() const {
    char Name[32];
    brlapi__getDriverName(handle(), Name, 32);
    return { Name, strlen(Name) };
  }
  DisplaySize displaySize() const {
    DisplaySize Size;
    brlapi__getDisplaySize(handle(), &Size.X, &Size.Y);
    return Size;
  }
};

}
