#include "brlapi.hpp"

#include <brlapi.h>
#define PACKED
#include <brltty/brldefs-ht.h>
#undef PACKED

#include <netdb.h>

namespace {
  
struct BrlAPIErrorCategory : std::error_category {
  const char* name() const noexcept override { return "BrlAPI"; }
  std::string message(int ev) const override {
    if (ev >= brlapi_nerr) return "Unknown errror";
    return brlapi_errlist[ev];
  }
};
  
const BrlAPIErrorCategory BrlAPICategory {};
  
void throwSystemError() {
  switch (brlapi_errno) {
  case BRLAPI_ERROR_LIBCERR:
    throw std::system_error(brlapi_libcerrno, std::generic_category());
  case BRLAPI_ERROR_GAIERR:
    switch (brlapi_gaierrno) {
    case EAI_SYSTEM:
      throw std::system_error(brlapi_libcerrno, std::generic_category());
    default:
      throw std::runtime_error("Unresolved GAI error");
    }
  default:
    throw std::system_error(brlapi_errno, BrlAPIErrorCategory());
  }
}

} // namespace

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

namespace BrlAPI::Driver {

HandyTech::Key HandyTech::fromKeyCode(BrlAPI::KeyCode const &Key) {
  switch (static_cast<HT_KeyGroup>(Key.group())) {
  case HT_GRP_NavigationKeys:
    switch (static_cast<HT_NavigationKey>(Key.number())) {
    case HT_KEY_B1: return NavigationKey::B1;
    case HT_KEY_B2: return NavigationKey::B2;
    case HT_KEY_B3: return NavigationKey::B3;
    case HT_KEY_B4: return NavigationKey::B4;
    case HT_KEY_B5: return NavigationKey::B5;
    case HT_KEY_B6: return NavigationKey::B6;
    case HT_KEY_B7: return NavigationKey::B7;
    case HT_KEY_B8: return NavigationKey::B8;
    case HT_KEY_Space: return NavigationKey::LeftSpace;
    case HT_KEY_SpaceRight: return NavigationKey::RightSpace;
    default: throw std::runtime_error("Unknown HandyTech navigation key " + std::to_string(Key.number()));
    }
  case HT_GRP_RoutingKeys: return RoutingKey(Key.number());
  default: throw std::runtime_error("Unknown HandyTech key group");
  }
}

}

class BrlAPI::Connection::Implementation {
  std::unique_ptr<std::byte[]> HandleStorage;
public:
  Implementation() : HandleStorage(new std::byte[brlapi_getHandleSize()]) {}
  brlapi_handle_t *handle() const {
    return reinterpret_cast<brlapi_handle_t *>(HandleStorage.get());
  }
};

BrlAPI::TTY::~TTY() {
  brlapi__leaveTtyMode(Conn.BrlAPI->handle());
}

void BrlAPI::TTY::writeText(std::string Text) {
  if (brlapi__writeText(Conn.BrlAPI->handle(), -1, Text.c_str()) == -1) {
    throwSystemError();
  }
}

BrlAPI::KeyCode BrlAPI::TTY::readKey() const {
  brlapi_keyCode_t Key;
  if (brlapi__readKey(Conn.BrlAPI->handle(), 1, &Key) == -1) {
    throwSystemError();
  }
  return { Key };
}

bool BrlAPI::TTY::readKey(KeyCode &KeyCode) const {
  brlapi_keyCode_t Key;
  auto Result = brlapi__readKey(Conn.BrlAPI->handle(), 0, &Key);
  if (Result == -1) {
    throwSystemError();
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
    throwSystemError();
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
    throwSystemError();
  }
  brlapi_range_t Ranges[] = {
    { std::numeric_limits<brlapi_keyCode_t>::min()
    , std::numeric_limits<brlapi_keyCode_t>::max()
    }
  };
  if (brlapi__acceptKeyRanges(
        BrlAPI->handle(),
        Ranges, std::distance(std::begin(Ranges), std::end(Ranges))
      ) == -1) {
    throwSystemError();
  }
  return { *this, Number };
}

