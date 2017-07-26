#include <chrono>
#include <thread>

#include <jack.hpp>
#include <dsp.hpp>

class EdgeDetect : public JACK::Client {
  JACK::AudioIn CVIn;
  JACK::MIDIOut MIDIOut;
  BrlCV::EWMA<decltype(CVIn)::value_type> FastAverage, SlowAverage;
  decltype(CVIn)::value_type PreviousDifference = 0;
  std::size_t FramesPerPulse = 0;
  float const Threshold;

public:
  EdgeDetect(float Threshold = 0.2) : JACK::Client("EdgeDetect")
  , CVIn(createAudioIn("In"))
  , MIDIOut(createMIDIOut("Out"))
  , FastAverage(0.25), SlowAverage(0.0625)
  , Threshold(Threshold)
  {
    Expects(Threshold > 0);
  }

  int process(std::uint32_t FrameCount) override {
    auto MIDIBuffer = MIDIOut.buffer(FrameCount);
    std::size_t Frame = 0;

    MIDIBuffer.clear();
    for (auto Sample : CVIn.buffer(FrameCount)) {
      auto Difference = FastAverage(Sample) - SlowAverage(Sample);

      if (PreviousDifference < Threshold && Difference > Threshold) {
        MIDIBuffer.reserve<1>(Frame)[0] = gsl::byte(0XFB);
        std::cout << "Positive Edge (" << Frame << ", " << FramesPerPulse << ")" << "\n";
        FramesPerPulse = 0;
      }
      PreviousDifference = Difference;
      Frame += 1; FramesPerPulse += 1;
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
