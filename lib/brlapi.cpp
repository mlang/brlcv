#include "brlapi.hpp"

#include <brlapi.h>

struct BrlAPI::Connection::Implementation {
  std::unique_ptr<gsl::byte[]> HandleStorage;
  brlapi_handle_t *handle() const {
    return static_cast<brlapi_handle_t *>(static_cast<void *>(HandleStorage.get()));
  }
  Implementation() : HandleStorage(new gsl::byte[brlapi_getHandleSize()]) {}
};

BrlAPI::Connection::Connection()
: BrlAPI(std::make_unique<Implementation>())
{
  brlapi_connectionSettings_t Settings = BRLAPI_SETTINGS_INITIALIZER;
  auto FileDescriptor = brlapi__openConnection(BrlAPI->handle(), &Settings, &Settings);
  if (FileDescriptor == -1) {
    throw std::system_error(errno, std::generic_category());
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

