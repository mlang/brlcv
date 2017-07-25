#include <chrono>
#include <thread>

#include <jack.hpp>

template<typename T> T exponentialAverage(T Value, T Average, T Weight) {
  return Weight * Value + (T(1) - Weight) * Average;
}

class EdgeDetect : public JACK::Client {
  JACK::AudioIn In;
  float SlowAverage, FastAverage, PreviousDifference;
  std::size_t PulseWidth;
  float const Threshold;

public:
  EdgeDetect(float Threshold = 0.2)
  : JACK::Client("EdgeDetect")
  , In(createAudioIn("In"))
  , SlowAverage(0), FastAverage(0), PreviousDifference(0), PulseWidth(0)
  , Threshold(Threshold) {}
  int process(std::uint32_t FrameCount) override {
    std::size_t Frame = 0;
    for (auto Sample : In.buffer(FrameCount)) {
      FastAverage = exponentialAverage(Sample, FastAverage, 0.25f);
      SlowAverage = exponentialAverage(Sample, SlowAverage, 0.0625f);
      auto Difference = FastAverage - SlowAverage;
      if (PreviousDifference < Threshold && Difference > Threshold) {
        std::cout << "Positive Edge (" << Frame << ", " << PulseWidth << ")" << "\n";
        PulseWidth = 0;
      }
      PreviousDifference = Difference;
      Frame += 1; PulseWidth += 1;
    }
    return 0;
  }
};

using namespace std::literals::chrono_literals;

int main() {
  EdgeDetect App;

  App.activate();
  while (true) {
    std::this_thread::sleep_for(1s);
  }
  return EXIT_SUCCESS;
}

