#include <iostream>

#include <brlapi.hpp>

using std::cout;
using std::endl;

int main() {
  BrlAPI::Connection Braille;
  cout << Braille.driverName() << " (" << Braille.displaySize() << ")" << endl;
}
