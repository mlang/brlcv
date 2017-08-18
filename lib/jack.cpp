#define GSL_THROW_ON_CONTRACT_VIOLATION
#include <jack.hpp>
#include <cstring>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <stdexcept>
#include <system_error>

namespace std {
  template<> struct is_error_code_enum<JackStatus>:true_type{};
  template<> struct is_error_condition_enum<JackStatus>:true_type{};
} // namespace std

namespace {
  
struct JACKErrorCategory : std::error_category {
  const char* name() const noexcept override { return "JACK"; }
  bool
  equivalent(int Value, const std::error_condition& Condition) const noexcept override
  {
    return Condition.category() == *this &&
           (Value & Condition.value()) == Condition.value();
  }
  std::string message(int ev) const override {
    std::string msg;
    if ((ev & JackFailure) == JackFailure) {
      msg += "Overall operation failed";
    }
    if ((ev & JackInvalidOption) == JackInvalidOption) {
      if (!msg.empty()) {
	msg += ", ";
      }
      msg += "The operation contained an invalid or unsupported option";
    }
    if ((ev & JackNameNotUnique) == JackNameNotUnique) {
      if (!msg.empty()) {
	msg += ", ";
      }
      msg += "The desired client name was not unique";
    }
    if ((ev & JackServerFailed) == JackServerFailed) {
      if (!msg.empty()) {
	msg += ", ";
      }
      msg += "Unable to connect to the JACK server";
    }
    if (msg.empty()) {
      msg = "Unknown error ";
      msg += std::to_string(ev);
    }
    return msg;
  }
};

const JACKErrorCategory JACKCategory {};

} // namespace

std::error_code make_error_code(JackStatus e) {
  return { static_cast<int>(e), JACKCategory };
}

std::error_condition make_error_condition(JackStatus e) {
  return { static_cast<int>(e), JACKCategory };
}

template<> struct BrlCV::impl_ptr<JACK::Client>::implementation {
  jack_client_t *const Client;
    
  explicit implementation(std::string Name)
  : Client([&] {
      jack_status_t Status;
      auto Handle = jack_client_open(Name.c_str(), JackNoStartServer, &Status);
      if (Handle == nullptr) {
	throw std::system_error(make_error_code(Status));
      }
      return Handle;
    }())
  {}
  ~implementation() { jack_client_close(Client); }
};

template<> struct BrlCV::impl_ptr<JACK::Port>::implementation {
  JACK::Client &Client;
  jack_port_t * const Port;
  
  implementation(JACK::Client &Client, std::string_view Name, std::string_view Type, JackPortFlags Flags)
  : Client(Client)
  , Port(jack_port_register(Client->Client, Name.data(), Type.data(), Flags, 0))
  {
    if (Port == nullptr) {
      throw std::runtime_error("Failed to register port");
    }
  }
  ~implementation() { jack_port_unregister(Client->Client, Port); }

  auto getBuffer(int FrameCount) { return jack_port_get_buffer(Port, FrameCount); }
};

namespace JACK {

Port::Port(Client &C, std::string_view N, std::string_view T, bool IsInput)
: impl_ptr(C, N, T, static_cast<JackPortFlags>(IsInput ? JackPortIsInput : JackPortIsOutput))
{}

Port::Port(Port &&) noexcept = default;
Port &Port::operator=(Port &&) noexcept = default;

Port::~Port() = default;

std::string Port::name() const {
  return jack_port_name((*this)->Port);
}

std::size_t Port::connections() const {
  return jack_port_connected((*this)->Port);
}

AudioIn::AudioIn(JACK::Client &Client, std::string_view Name)
: Port(Client, Name, JACK_DEFAULT_AUDIO_TYPE, true)
{}

gsl::span<float const> AudioIn::buffer(std::int32_t FrameCount) {
  return { static_cast<float *>((*this)->getBuffer(FrameCount)), FrameCount };
}
    
std::tuple<std::uint32_t, std::uint32_t> AudioIn::latencyRange() const {
  jack_latency_range_t Range{};
  jack_port_get_latency_range((*this)->Port, JackCaptureLatency, &Range);
  return { Range.min, Range.max };
}

AudioOut::AudioOut(JACK::Client &Client, std::string_view Name)
: Port(Client, Name, JACK_DEFAULT_AUDIO_TYPE, false)
{}

gsl::span<float> AudioOut::buffer(std::int32_t FrameCount) {
  return { static_cast<float *>((*this)->getBuffer(FrameCount)), FrameCount };
}

std::tuple<std::uint32_t, std::uint32_t> AudioOut::latencyRange() const {
  jack_latency_range_t Range{};
  jack_port_get_latency_range((*this)->Port, JackPlaybackLatency, &Range);
  return { Range.min, Range.max };
}

void MIDIBuffer::clear() {
  jack_midi_clear_buffer(Buffer);
}

MIDIBuffer::Index &MIDIBuffer::Index::operator=(MIDI::SongPositionPointer const &SPP) {
  std::copy(SPP.begin(), SPP.end(), Buffer.reserve(Offset, SPP.size()).begin());
    
  return *this;
}

MIDIBuffer::Index &MIDIBuffer::Index::operator=(MIDI::SystemRealTimeMessage Message) {
  Buffer.reserve<1>(Offset)[0] = static_cast<std::byte>(Message);
            
  return *this;
}

gsl::span<std::byte>
MIDIBuffer::reserve(std::uint32_t FrameOffset, std::uint32_t Size) {
  Expects(jack_midi_max_event_size(Buffer) >= Size);
  return {
    reinterpret_cast<std::byte *>(
      jack_midi_event_reserve(Buffer, FrameOffset, Size)
    ), Size
  };
}

MIDIBuffer::Iterator::const_reference MIDIBuffer::Iterator::operator*() const
{
  jack_midi_event_t Event;
  jack_midi_event_get(&Event, Buffer.Buffer, Offset);
  gsl::span<std::byte> Span(reinterpret_cast<std::byte*>(Event.buffer), Event.size);

  CurrentEvent = std::nullopt;

  if (!Span.empty()) {
    if (Span[0] == std::byte(0XF2)) {
      CurrentEvent = { Event.time, MIDI::SongPositionPointer(Span) };
    } else if (Span[0] >= static_cast<std::byte>(MIDI::SystemRealTimeMessage::Clock)) {
      CurrentEvent = { Event.time, MIDI::SystemRealTimeMessage(Span[0]) };
    } else {
      CurrentEvent = { Event.time, Span };
    }
  }

  Expects(CurrentEvent.has_value());

  return CurrentEvent.value();
}

MIDIBuffer::Iterator MIDIBuffer::begin() const {
  return { *this, 0, jack_midi_get_event_count(Buffer) };
}

MIDIBuffer::Iterator MIDIBuffer::end() const {
  auto const EventCount = jack_midi_get_event_count(Buffer);
  return { *this, EventCount, EventCount };
}

MIDIOut::MIDIOut(JACK::Client &Client, std::string_view Name)
: Port(Client, Name, JACK_DEFAULT_MIDI_TYPE, false)
{}

MIDIBuffer MIDIOut::buffer(std::uint32_t FrameCount) {
  return { (*this)->getBuffer(FrameCount), FrameCount, &MIDIBuffer::clear };
}

MIDIIn::MIDIIn(JACK::Client &Client, std::string_view Name)
: Port(Client, Name, JACK_DEFAULT_MIDI_TYPE, true)
{}

MIDIBuffer const MIDIIn::buffer(std::uint32_t FrameCount) {
  return { (*this)->getBuffer(FrameCount), FrameCount };
}

extern "C" int process(jack_nframes_t nframes, void *instance)
{
  return static_cast<Client *>(instance)->process(nframes);
}

Client::Client(std::string Name) : impl_ptr(std::move(Name))
{
  jack_set_process_callback((*this)->Client, &JACK::process, this);
}

Client::Client(Client &&) noexcept = default;
Client &Client::operator=(Client &&) noexcept = default;

Client::~Client() = default;

unsigned int Client::sampleRate() const {
  return jack_get_sample_rate((*this)->Client);
}

bool Client::isRealtime() const {
  return jack_is_realtime((*this)->Client) == 1;
}

AudioIn Client::createAudioIn(std::string_view Name) {
  return { *this, Name };
}

AudioOut Client::createAudioOut(std::string_view Name) {
  return { *this, Name };
}

MIDIIn Client::createMIDIIn(std::string_view Name) {
  return { *this, Name };
}

MIDIOut Client::createMIDIOut(std::string_view Name) {
  return { *this, Name };
}

void Client::activate() {
  auto status = jack_activate((*this)->Client);
  if (status < 0) {
    throw std::system_error(-status, std::generic_category());
  }
}

void Client::deactivate() {
  auto Status = jack_deactivate((*this)->Client);
  if (Status != 0) {
    throw std::system_error(Status, std::generic_category());
  }
}

void Client::connect(std::string_view From, std::string_view To) {
  auto Result = jack_connect((*this)->Client, From.data(), To.data());
  if (Result != 0 && Result != EEXIST) {
    throw std::runtime_error("JACK: Unable to connect port");
  }
}

} // namespace JACK
