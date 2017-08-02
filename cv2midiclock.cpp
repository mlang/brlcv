#include <chrono>
#include <thread>

#include <boost/lockfree/spsc_queue.hpp>

#include <jack.hpp>
#include <dsp.hpp>

boost::lockfree::spsc_queue<std::size_t, boost::lockfree::capacity<8>> FPP;

class EdgeDetect : public JACK::Client {
  JACK::AudioIn CVIn;
  JACK::MIDIOut MIDIOut;
  BrlCV::EWMA<decltype(CVIn)::value_type> FastAverage, SlowAverage;
  decltype(CVIn)::value_type PreviousDifference = 0;
  std::size_t FramesSinceLastPulse = 0, FramesPerPulse = 0, FramesUntilNextMIDIClock = 0, MIDIClockPulse = 0;
  float const Threshold;
  BrlCV::FairSegmentation<24> MIDIClockFrameCount;

  public:
  EdgeDetect(float Threshold = 0.2) : JACK::Client("EdgeDetect")
  , CVIn(createAudioIn("In"))
  , MIDIOut(createMIDIOut("Out"))
  , FastAverage(0.25), SlowAverage(0.0625)
  , FramesUntilNextMIDIClock(0)
                                      , Threshold(Threshold)
  { Expects(Threshold > 0); }

  int process(std::uint32_t FrameCount) override {
    auto MIDIBuffer = MIDIOut.buffer(FrameCount);
    int PulseOffset = -1;
    std::size_t Frame = 0;

    MIDIBuffer.clear();
    for (auto Sample : CVIn.buffer(FrameCount)) {
      auto Difference = FastAverage(Sample) - SlowAverage(Sample);

      if (PreviousDifference < Threshold && Difference > Threshold) {
        PulseOffset = Frame;
        FramesPerPulse = FramesSinceLastPulse;
        FramesSinceLastPulse = 0;
        FPP.push(FramesPerPulse);
      }
      PreviousDifference = Difference;
      Frame += 1; FramesSinceLastPulse += 1;
    }

    if (FramesPerPulse) {
      if (PulseOffset != -1) {
        MIDIClockPulse = 0;
        MIDIClockFrameCount = FramesPerPulse;
        MIDIBuffer[PulseOffset] = MIDI::SystemRealTimeMessage::Clock;
        FramesUntilNextMIDIClock = MIDIClockFrameCount[MIDIClockPulse++];
        FramesUntilNextMIDIClock -= FrameCount - PulseOffset;
      } else if (MIDIClockPulse < 24) {
        if (FramesUntilNextMIDIClock < FrameCount) {
          MIDIBuffer[FramesUntilNextMIDIClock] = MIDI::SystemRealTimeMessage::Clock;
          FramesUntilNextMIDIClock = MIDIClockFrameCount[MIDIClockPulse++] - (FrameCount - FramesUntilNextMIDIClock);
        } else {
          FramesUntilNextMIDIClock -= FrameCount;
        }
      }
    }

    return 0;
  }
};

using namespace std::literals::chrono_literals;

int main() {
  EdgeDetect App;
  std::string const Chars = "\\|/-";
  unsigned int CurrentChar = 0;
  App.activate();
  while (true) {
    std::size_t FramesPerPulse;
    while (FPP.pop(FramesPerPulse)) {
      auto BPM = static_cast<float>(App.sampleRate()) / FramesPerPulse * 60;
      std::cout << BPM << " BPM " << Chars[CurrentChar++] << "        \r";
      std::flush(std::cout);
      if (CurrentChar == Chars.size()) CurrentChar = 0;
    }
    std::this_thread::sleep_for(5ms);
  }

  return EXIT_SUCCESS;
}
