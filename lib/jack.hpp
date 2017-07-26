#if !defined(BrlCV_JACK_HPP)
#define BrlCV_JACK_HPP

#include <memory>
#include <string>

#include <gsl/gsl>

#include <midi.hpp>

namespace JACK {

class Client;

class Port {
protected:
  struct Implementation;
  std::unique_ptr<Implementation> JACK;
  Port(Client *, std::string Name, std::string Type, bool IsInput);

public:
  ~Port();
  Port(Port &&) noexcept;
  Port &operator=(Port &&) noexcept;
  Port(const Port &) = delete;
  Port &operator=(const Port &) = delete;
};

class AudioIn : public Port {
  friend class Client;
  AudioIn(Client *, std::string Name);
      
public:
  using value_type = float;

  gsl::span<value_type> buffer(std::int32_t Frames);
};

class AudioOut : public Port {
  friend class Client;
  AudioOut(Client *, std::string Name);

public:
  using value_type = float;

  gsl::span<value_type> buffer(std::int32_t Frames);
};

class MIDIBuffer {
  void *Buffer;
  std::uint32_t const Frames;
  friend class MIDIOut;
  
  MIDIBuffer(void *Buffer, std::uint32_t FrameCount) : Buffer(Buffer), Frames(FrameCount) {
    Expects(Buffer != nullptr);
  }

public:
  class Index {
    MIDIBuffer &Buffer;
    std::uint32_t Offset;
    friend class MIDIBuffer;

    Index(MIDIBuffer &Buffer, std::uint32_t Offset)
    : Buffer(Buffer), Offset(Offset) {}
  public:
    Index &operator=(MIDI::SystemRealTimeMessage Message) {
      Buffer.reserve<1>(Offset)[0] = static_cast<gsl::byte>(Message);

      return *this;
    }
  };
  void clear();
  gsl::span<gsl::byte> reserve(std::uint32_t FrameOffset, std::uint32_t Size);
  template<std::uint32_t Size>
  gsl::span<gsl::byte, Size> reserve(std::uint32_t FrameOffset) {
    return reserve(FrameOffset, Size);
  }
  Index operator[](std::uint32_t FrameOffset) {
    Ensures(FrameOffset < Frames);
    return { *this, FrameOffset };
  }
};

class MIDIOut : public Port {
  friend class Client;
  MIDIOut(Client *, std::string Name);
      
public:
  using value_type = gsl::byte;
        
  MIDIBuffer buffer(std::int32_t FrameCount);
};

class Client {
  struct Implementation;
  std::unique_ptr<Implementation> JACK;
  friend class Port;

public:
  explicit Client(std::string Name);
  Client(Client &&) noexcept;
  Client &operator=(Client &&) noexcept;
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;
  virtual ~Client();

  unsigned int sampleRate() const;
  bool isRealtime() const;

  AudioIn createAudioIn(std::string Name);
  AudioOut createAudioOut(std::string Name);
  MIDIOut createMIDIOut(std::string Name);

  void activate();
  void deactivate();

  virtual int process(std::uint32_t nframes) = 0;
};

} // namespace JACK
#endif // BrlCV_JACK_HPP
