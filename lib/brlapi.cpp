#include "brlapi.hpp"

#include <brlapi.h>

namespace BrlAPI {
  
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
  
} // namespace BrlAPI

class BrlAPI::Connection::Implementation {
  std::unique_ptr<gsl::byte[]> HandleStorage;
public:
  Implementation() : HandleStorage(new gsl::byte[brlapi_getHandleSize()]) {}
  brlapi_handle_t *handle() const {
    return reinterpret_cast<brlapi_handle_t *>(HandleStorage.get());
  }
};

BrlAPI::TTY::~TTY() {
  brlapi__leaveTtyMode(Conn->BrlAPI->handle());
}

void BrlAPI::TTY::writeText(std::string Text) {
  if (brlapi__writeText(Conn->BrlAPI->handle(), -1, Text.c_str()) == -1) {
    throw std::system_error(brlapi_errno, std::generic_category());
  }
}

BrlAPI::KeyCode BrlAPI::TTY::readKey() const {
  brlapi_keyCode_t Key;
  if (brlapi__readKey(Conn->BrlAPI->handle(), 1, &Key) == -1) {
    throw std::system_error(brlapi_errno, std::generic_category());
  }
  return { Key };
}

bool BrlAPI::TTY::readKey(KeyCode &KeyCode) const {
  brlapi_keyCode_t Key;
  auto Result = brlapi__readKey(Conn->BrlAPI->handle(), 0, &Key);
  if (Result == -1) {
    throw std::system_error(brlapi_errno, std::generic_category());
  }
  if (Result == 1) {
    KeyCode = BrlAPI::KeyCode(Key);
  }
  return Result == 1;
}

BrlAPI::Connection::Connection() : BrlAPI(std::make_unique<Implementation>()) {
  brlapi_connectionSettings_t Settings = BRLAPI_SETTINGS_INITIALIZER;
  auto FileDescriptor = brlapi__openConnection(BrlAPI->handle(), &Settings, &Settings);
  if (FileDescriptor == -1) {
    throw std::system_error(brlapi_errno, std::generic_category());
  }
}

BrlAPI::Connection::~Connection() {
  brlapi__closeConnection(BrlAPI->handle());
}

BrlAPI::Connection::Connection(Connection &&) noexcept = default;
BrlAPI::Connection &BrlAPI::Connection::operator=(Connection &&) noexcept = default;

std::string BrlAPI::Connection::driverName() const {
  char Name[32];
  brlapi__getDriverName(BrlAPI->handle(), Name, 32);
  return { Name, strlen(Name) };
}

BrlAPI::DisplaySize BrlAPI::Connection::displaySize() const {
  DisplaySize Size;
  brlapi__getDisplaySize(BrlAPI->handle(), &Size.X, &Size.Y);
  return Size;
}

BrlAPI::TTY BrlAPI::Connection::tty(int TTY, bool Raw) {
  auto Number = brlapi__enterTtyMode(BrlAPI->handle(), TTY, Raw? "HandyTech" : "");
  if (Number == -1) {
    throw std::system_error(brlapi_errno, std::generic_category());
  }
  brlapi_range_t Ranges[] = {
    { 0, std::numeric_limits<brlapi_keyCode_t>::max() }
  };
  brlapi__acceptKeyRanges(
    BrlAPI->handle(),
    Ranges, std::distance(std::begin(Ranges), std::end(Ranges))
  );
  return { this, Number };
}

