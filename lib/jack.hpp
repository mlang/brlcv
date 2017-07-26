#if !defined(BrlCV_JACK_HPP)
#define BrlCV_JACK_HPP

#include <memory>
#include <string>

#include <gsl/gsl>

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
  friend class MIDIOut;
  
  explicit MIDIBuffer(void *Buffer) : Buffer(Buffer) {
    Expects(Buffer != nullptr);
  }

public:
  void clear();
  gsl::span<gsl::byte> reserve(std::uint32_t FrameOffset, std::uint32_t Size);
  template<std::uint32_t Size>
  gsl::span<gsl::byte, Size> reserve(std::uint32_t FrameOffset) {
    return reserve(FrameOffset, Size);
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
