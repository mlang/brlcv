#include "jack.hpp"
#include "brlapi.hpp"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

template<typename... Features>
using AudioAccumulatorSet = boost::accumulators::accumulator_set<
  float, boost::accumulators::features<Features...>
>;

using Count = boost::accumulators::tag::count;
using Max = boost::accumulators::tag::max;
using Min = boost::accumulators::tag::min;
using Mean = boost::accumulators::tag::mean;
using Variance = boost::accumulators::tag::variance;

class Statistics final : public JACK::Client {
  JACK::AudioIn In;
  AudioAccumulatorSet<Count, Max, Mean, Min, Variance> Accumulator;

public:
  Statistics() : JACK::Client("Statistics"), In(createAudioIn("In")) {}
  int process(std::uint32_t FrameCount) override {
    for (auto &Value: In.buffer(FrameCount)) Accumulator(Value);
    return 0;
  }
  auto max() const { return boost::accumulators::max(Accumulator); }
  auto mean() const { return boost::accumulators::mean(Accumulator); }
  auto min() const { return boost::accumulators::min(Accumulator); }
  auto sampleCount() const { return boost::accumulators::count(Accumulator); }
  auto variance() const { return boost::accumulators::variance(Accumulator); }
};

#include <chrono>
#include <iostream>
#include <thread>

using std::cout;
using std::endl;
using std::this_thread::sleep_for;
using namespace std::literals::chrono_literals;

int main() {
  BrlAPI::Connection Braille;
  cout << Braille.driverName() << " (" << Braille.displaySize() << ")" << endl;
  Statistics Client;
  cout << "Rate: " << Client.sampleRate() << endl;

  Client.activate();
  sleep_for(5s);
  Client.deactivate();

  cout << Client.sampleCount() << ": "
       << "mean=" << Client.mean() << ", variance=" << Client.variance()
       << ", min=" << Client.min() << ", max=" << Client.max()
       << endl;
}
