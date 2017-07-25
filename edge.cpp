#include <chrono>
#include <thread>

#include <jack.hpp>
#include <dsp.hpp>

class EdgeDetect : public JACK::Client {
  JACK::AudioIn In;
  decltype(In)::value_type PreviousDifference = 0;
  BrlCV::ExponentiallyWeightedMovingAverage<decltype(In)::value_type>
  FastAverage, SlowAverage;
  
  std::size_t PulseWidth = 0;
  float const Threshold;

public:
  EdgeDetect(float Threshold = 0.2) : JACK::Client("EdgeDetect")
  , In(createAudioIn("In")), FastAverage(0.25), SlowAverage(0.0625)
  , Threshold(Threshold) {}

  int process(std::uint32_t FrameCount) override {
    std::size_t Frame = 0;

    for (auto Sample : In.buffer(FrameCount)) {
      auto Difference = FastAverage(Sample) - SlowAverage(Sample);

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
    std::this_thread::sleep_for(10ms);
  }

  return EXIT_SUCCESS;
}
