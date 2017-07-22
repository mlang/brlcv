#include <chrono>
#include <iostream>
#include <thread>

#include <brlapi.hpp>

using std::cout;
using std::endl;
using namespace std::literals::chrono_literals;

int main() {
  BrlAPI::Connection Braille;
  cout << Braille.driverName() << " (" << Braille.displaySize() << ")" << endl;
  {
    auto TTY = Braille.tty(1, true);
    std::stringstream Text;
    Text << "You are on tty" << TTY.number();
    TTY.writeText(Text);
    std::this_thread::sleep_for(5s);
  }
}
