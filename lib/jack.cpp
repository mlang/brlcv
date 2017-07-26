#include "jack.hpp"
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

struct JACK::Client::Implementation {
  jack_client_t *const Client;
    
  explicit Implementation(std::string Name)
  : Client([&] {
      jack_status_t Status;
      auto Handle = jack_client_open(Name.c_str(), JackNoStartServer, &Status);
      if (Handle == nullptr) {
	throw std::system_error(make_error_code(Status));
      }
      return Handle;
    }())
  {}
  ~Implementation() { jack_client_close(Client); }

  Implementation(Implementation const &) = delete;
  Implementation(Implementation &&) = delete;

  Implementation &operator=(Implementation const &) = delete;
  Implementation &operator=(Implementation &&) = delete;
};

struct JACK::Port::Implementation {
  JACK::Client *Client;
  jack_port_t * const Port;
  
  Implementation(JACK::Client *Client, std::string Name, std::string Type, JackPortFlags Flags)
  : Client(Client)
  , Port(jack_port_register(Client->JACK->Client, Name.c_str(), Type.c_str(), Flags, 0))
  {
    if (Port == nullptr) {
      throw std::runtime_error("Failed to register port");
    }
  }
  ~Implementation() { jack_port_unregister(Client->JACK->Client, Port); }
};

namespace JACK {

Port::Port(Client *C, std::string N, std::string T, bool IsInput)
: JACK(std::make_unique<Implementation>(C, N, T, static_cast<JackPortFlags>(IsInput ? JackPortIsInput : JackPortIsOutput)))
{}

Port::Port(Port &&) noexcept = default;
Port &Port::operator=(Port &&) noexcept = default;

Port::~Port() = default;

AudioIn::AudioIn(JACK::Client *Client, std::string Name)
: Port(Client, std::move(Name), JACK_DEFAULT_AUDIO_TYPE, true)
{}

gsl::span<float> AudioIn::buffer(std::int32_t FrameCount) {
  return {
    static_cast<float *>(jack_port_get_buffer(JACK->Port, FrameCount)),
    FrameCount
  };
}
    
AudioOut::AudioOut(JACK::Client *Client, std::string Name)
: Port(Client, std::move(Name), JACK_DEFAULT_AUDIO_TYPE, false)
{}

gsl::span<float> AudioOut::buffer(std::int32_t FrameCount) {
  return {
    static_cast<float *>(jack_port_get_buffer(JACK->Port, FrameCount)),
    FrameCount
  };
}

void MIDIBuffer::clear() {
  jack_midi_clear_buffer(Buffer);
}

gsl::span<gsl::byte> MIDIBuffer::reserve(std::uint32_t FrameOffset, std::uint32_t Size) {
  return {
    reinterpret_cast<gsl::byte *>(jack_midi_event_reserve(Buffer, FrameOffset, Size)),
    Size
  };
}

MIDIOut::MIDIOut(JACK::Client *Client, std::string Name)
: Port(Client, std::move(Name), JACK_DEFAULT_MIDI_TYPE, false)
{}

MIDIBuffer MIDIOut::buffer(std::int32_t FrameCount) {
  return MIDIBuffer(jack_port_get_buffer(JACK->Port, FrameCount));
}

extern "C" int process(jack_nframes_t nframes, void *instance)
{
  return static_cast<Client *>(instance)->process(nframes);
}

Client::Client(std::string Name)
: JACK(std::make_unique<Implementation>(std::move(Name)))
{
  jack_set_process_callback(JACK->Client, &JACK::process, this);
}

Client::Client(Client &&) noexcept = default;
Client &Client::operator=(Client &&) noexcept = default;

Client::~Client() = default;

unsigned int Client::sampleRate() const {
  return jack_get_sample_rate(JACK->Client);
}

bool Client::isRealtime() const {
  return jack_is_realtime(JACK->Client) == 1;
}

AudioIn Client::createAudioIn(std::string Name) {
  return { this, std::move(Name) };
}

AudioOut Client::createAudioOut(std::string Name) {
  return { this, std::move(Name) };
}

MIDIOut Client::createMIDIOut(std::string Name) {
  return { this, std::move(Name) };
}

void Client::activate() {
  auto status = jack_activate(JACK->Client);
  if (status < 0) {
    throw std::system_error(-status, std::generic_category());
  }
}

void Client::deactivate() {
  auto Status = jack_deactivate(JACK->Client);
  if (Status != 0) {
    throw std::system_error(Status, std::generic_category());
  }
}

} // namespace JACK
