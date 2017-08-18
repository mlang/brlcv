#define GSL_THROW_ON_CONTRACT_VIOLATION
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>

#include <jack.hpp>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/density.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include <sys/mman.h>

class MIDILatency : public JACK::Client {
  JACK::MIDIIn In; JACK::MIDIOut Out;
  int MonotonicCount = 0;
  boost::lockfree::spsc_queue<int, boost::lockfree::capacity<50>> Measurements;
  std::condition_variable DataReady;
  int MaxEvents;
  bool Done = false;

public:
  explicit MIDILatency(std::chrono::seconds Duration)
  : JACK::Client("MIDILatency")
  , In(createMIDIIn("In")), Out(createMIDIOut("Out"))
  , MaxEvents(sampleRate() * Duration.count() / 64)
  {
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
      throw std::system_error(errno, std::generic_category());
    }
    activate();
  }

  int process(int FrameCount) override {
    auto const Frame = (MonotonicCount / FrameCount) % FrameCount;
    Out.buffer(FrameCount)[Frame] = MIDI::SongPositionPointer {
      (MonotonicCount + Frame) % (1 << 14)
    };

    for (auto &[Offset, Event]: In.buffer(FrameCount)) {
      if (auto SPP = std::get_if<MIDI::SongPositionPointer>(&Event)) {
        Measurements.push((MonotonicCount + Offset - *SPP) % (1 << 14));
        DataReady.notify_one();
      }
    }

    MonotonicCount += FrameCount;

    return 0;
  }

  void done() {
    Done = true;
    DataReady.notify_one();
  }

  template<typename NoSignal, typename Progress>
  auto get(NoSignal noSignal, Progress progress) {
    boost::accumulators::accumulator_set<
      decltype(Measurements)::value_type, boost::accumulators::features<
        boost::accumulators::tag::count, boost::accumulators::tag::density,
        boost::accumulators::tag::min, boost::accumulators::tag::max,
        boost::accumulators::tag::mean, boost::accumulators::tag::variance
      >
    > Accumulator( boost::accumulators::tag::density::num_bins = 20
                 , boost::accumulators::tag::density::cache_size = MaxEvents / 10
                 );
    auto args = [&] {
      auto const SampleRate = sampleRate();
      auto us = [=](auto Frames) {
        return std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::duration<long double>(Frames) / SampleRate
        );
      };
      return std::tuple(
        us(boost::accumulators::min(Accumulator)),
        us(boost::accumulators::mean(Accumulator)),
        us(boost::accumulators::max(Accumulator)),
        us(sqrt(boost::accumulators::variance(Accumulator))),
        boost::accumulators::count(Accumulator) * 100 / MaxEvents
      );
    };

    auto PreviousArgs = std::optional(args());
    std::mutex Mutex;

    for (auto Lock = std::unique_lock(Mutex);
         !Done && boost::accumulators::count(Accumulator) < MaxEvents;
         DataReady.wait_for(Lock, std::chrono::milliseconds(100))) {
      if (Measurements.read_available() == 0) {
        noSignal();
        PreviousArgs.reset();
      } else {
        Measurements.consume_all(Accumulator);
        decltype(PreviousArgs) Args = args();
        if (Args != PreviousArgs) {
          std::apply(progress, Args.value());
          PreviousArgs = std::move(Args);
        }
      }
    }

    deactivate();

    return Accumulator;
  }
  auto get() { return get([]{}, [](auto...){}); }
};

#include <csignal>
#include <iomanip>

using namespace std::literals::chrono_literals;

MIDILatency *Instance = nullptr;

void signal(int) {
  Instance->done();
}

void stopOnSignal(MIDILatency &Latency) {
  Instance = &Latency;
  std::signal(SIGINT, signal);
}

namespace Console {

class Spinner {
  std::string Chars;
  std::size_t Index = 0;
public:
  explicit Spinner(std::string Chars = "|/-\\") : Chars(Chars) {
    Expects(!Chars.empty());
  }

  char operator()() {
    auto Result = Chars[Index++];
    Index %= Chars.size();

    return Result;
  }
};

} // namespace Console

int main() {
  MIDILatency Latency(5s);
  //Latency.connect("MIDILatency:Out", "MIDILatency:In");
  Latency.connect("alsa_midi:Hammerfall DSP HDSP MIDI 1 (out)", "MIDILatency:In");
  Latency.connect("MIDILatency:Out", "alsa_midi:Hammerfall DSP HDSP MIDI 1 (in)");

  stopOnSignal(Latency);

  auto const Accumulator = Latency.get(
    [] { // No signal
      static Console::Spinner Spinner;
      std::flush(std::cout << "\33[2K\r" << "No signal " << Spinner() << '\r');
    },
    [](auto Min, auto Mean, auto Max, auto Deviation, auto Percent) { // Progress
      std::flush(
        std::cout << "\33[2K\r"
                  << "Min=" << Min.count() << "us"
                  << " Mean=" << Mean.count() << "us"
                  << " Max=" << Max.count() << "us"
                  << " StdDev=" << Deviation.count() << "us"
                  << " (" << Percent << "%)"
                  << '\r'
      );
    }
  );
  auto const Count = boost::accumulators::count(Accumulator);
  if (Count > 0) {
    auto const Density = boost::accumulators::density(Accumulator);
    auto const Scale = float(75) / std::max_element(
      Density.begin(), Density.end(),
      [](auto &L, auto &R) { return L.second < R.second; }
    )->second;

    for (auto &Bin: Density) {
      std::cout << "\n" << std::setw(4) << int(Bin.first)
                << std::string(std::size_t(Scale * Bin.second), '-');
    }
    std::cout << std::endl;
  } else {
    std::cout << "\33[2K\r" << "No signal" << std::endl;
  }

  return Count > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
