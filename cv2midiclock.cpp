#include <chrono>
#include <optional>
#include <thread>

#include <boost/lockfree/spsc_queue.hpp>

#include <jack.hpp>
#include <dsp.hpp>

class PulseTracker {
  std::optional<std::uint32_t> FramesSinceLastPulse = std::nullopt;
  BrlCV::FairSegmentation<24> MIDIClockFrames;
  JACK::MIDIOut &MIDIOut;

public:
  PulseTracker(JACK::MIDIOut &MIDIOut) : MIDIOut(MIDIOut) {}

  class Guard {
    PulseTracker &Tracker;
    std::uint32_t const FrameCount;
    std::optional<std::uint32_t> Pulse = std::nullopt;
  public:
    Guard(PulseTracker &Tracker, std::uint32_t FrameCount)
    : Tracker(Tracker), FrameCount(FrameCount) {}
    void operator()(std::uint32_t Offset) {
      Expects(Offset < FrameCount);
      *Pulse = Offset;
    }
    ~Guard() {
      auto Buffer = Tracker.MIDIOut.buffer(FrameCount);
      Buffer.clear();
      if (Tracker.FramesSinceLastPulse && !Pulse) {
        *Tracker.FramesSinceLastPulse += FrameCount;
      }
    }
  };
  [[nodiscard]] Guard operator()(std::uint32_t FrameCount) {
    return { *this, FrameCount };
  };
};

class EdgeDetect : public JACK::Client {
  JACK::AudioIn CVIn;
  JACK::MIDIOut MIDIOut;
  BrlCV::EWMA<decltype(CVIn)::value_type> FastAverage, SlowAverage;
  decltype(CVIn)::value_type PreviousDifference = 0;
  std::size_t FramesSinceLastPulse = 0, FramesPerPulse = 0, FramesUntilNextMIDIClock = 0, MIDIClockPulse = 0;
  float const Threshold;
  BrlCV::FairSegmentation<24> MIDIClockFrameCount;
  boost::lockfree::spsc_queue<std::size_t, boost::lockfree::capacity<8>> FPP;
  PulseTracker CVPulse;

public:
  EdgeDetect(float Threshold = 0.2) : JACK::Client("EdgeDetect")
  , CVIn(createAudioIn("In"))
  , MIDIOut(createMIDIOut("Out"))
  , FastAverage(0.25), SlowAverage(0.0625)
  , FramesUntilNextMIDIClock(0)
  , Threshold(Threshold)
  , CVPulse(MIDIOut)
  {
    Expects(Threshold > 0);
    activate();
  }

  void connectCVIn(std::string Name = "system:capture_1") {
    connect(Name, CVIn);
  }
  void connectMIDIOut(std::string Name = "alsa_midi:Hammerfall DSP HDSP MIDI 1 (in)") {
    connect(MIDIOut, Name);
  }
  std::size_t latency() const {
    auto CaptureLatency = CVIn.latencyRange();
    return std::get<1>(CaptureLatency);
  }
  int process(std::uint32_t FrameCount) override {
    auto Pulse = CVPulse(FrameCount);
    auto MIDIBuffer = MIDIOut.buffer(FrameCount);
    int PulseOffset = -1;
    std::size_t Frame = 0;

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

  std::optional<float> bpm() {
    std::size_t FramesPerPulse;
    if (FPP.pop(FramesPerPulse)) {
      return static_cast<float>(sampleRate() * 60) / FramesPerPulse;
    }
    return std::nullopt;
  }
};

using namespace std::literals::chrono_literals;

int main() {
  EdgeDetect Clock;
  std::string const Chars = "\\|/-";
  unsigned int CurrentChar = 0;
  Clock.connectCVIn();
  Clock.connectMIDIOut();
  std::cout << Clock.latency() << std::endl;
  while (true) {
    if (auto BPM = Clock.bpm(); BPM) {
      std::cout << *BPM << " BPM " << Chars[CurrentChar++] << "        \r";
      std::flush(std::cout);
      CurrentChar %= Chars.size();
    }
    std::this_thread::sleep_for(5ms);
  }

  return EXIT_SUCCESS;
}
