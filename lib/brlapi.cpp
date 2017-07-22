#include "brlapi.hpp"

#include <brlapi.h>

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
  if (brlapi__writeText(Conn->BrlAPI->handle(), -1, Text.c_str()) == -1)
    throw std::system_error(brlapi_errno, std::generic_category());
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
  auto Number = brlapi__enterTtyMode(BrlAPI->handle(), TTY, "");
  if (Number == -1) throw std::system_error(brlapi_errno, std::generic_category());
  return { this, Number };
}

