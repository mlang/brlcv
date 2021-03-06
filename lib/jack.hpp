#if !defined(BrlCV_JACK_HPP)
#define BrlCV_JACK_HPP

#include <string>
#include <string_view>
#include <variant>

#include <gsl/gsl>

#include <impl_ptr.hpp>
#include <midi.hpp>

namespace JACK {

class Client;

class Port : protected BrlCV::impl_ptr<Port>::unique {
protected:
  Port(Client &, std::string_view N, std::string_view T, bool IsInput);

public:
  ~Port();
  Port(Port &&) noexcept;
  Port &operator=(Port &&) noexcept;
  Port(const Port &) = delete;
  Port &operator=(const Port &) = delete;

  std::string name() const;

  std::size_t connections() const;
};

class AudioIn : public Port {
  friend class Client;
  AudioIn(Client &, std::string_view Name);
      
public:
  using value_type = float;

  gsl::span<value_type const> buffer(std::int32_t FrameCount);

  std::tuple<std::uint32_t, std::uint32_t> latencyRange() const;
};

class AudioOut : public Port {
  friend class Client;
  AudioOut(Client &, std::string_view Name);

public:
  using value_type = float;

  gsl::span<value_type> buffer(std::int32_t FrameCount);

  std::tuple<std::uint32_t, std::uint32_t> latencyRange() const;
};

class MIDIBuffer {
  void *Buffer;
  std::uint32_t const Frames;
  friend class MIDIOut;
  friend class MIDIIn;

  MIDIBuffer(void *Buffer, std::uint32_t FrameCount,
             void (MIDIBuffer::*Prepare)() = nullptr)
  : Buffer(Buffer), Frames(FrameCount) {
    Expects(Buffer != nullptr);
    Expects(FrameCount > 0);
    if (Prepare != nullptr) {
      (this->*Prepare)();
    }
  }

public:
  class Index {
    MIDIBuffer &Buffer;
    std::uint32_t Offset;
    friend class MIDIBuffer;

    Index(MIDIBuffer &Buffer, std::uint32_t Offset)
    : Buffer(Buffer), Offset(Offset) {}
  public:
    Index &operator=(MIDI::SongPositionPointer const &);
    Index &operator=(MIDI::SystemRealTimeMessage);
  };
  void clear();
  gsl::span<std::byte> reserve(std::uint32_t FrameOffset, std::uint32_t Size);
  template<std::uint32_t Size>
  gsl::span<std::byte, Size> reserve(std::uint32_t FrameOffset) {
    return reserve(FrameOffset, Size);
  }
  Index operator[](std::uint32_t FrameOffset) {
    Expects(FrameOffset < Frames);
    return { *this, FrameOffset };
  }
  class Iterator {
  public:
    using value_type = std::tuple<std::uint32_t, std::variant<
      MIDI::SongPositionPointer, MIDI::SystemRealTimeMessage, gsl::span<std::byte>
    >>;
  private:
    MIDIBuffer const &Buffer;
    std::uint32_t Offset, Size;
    mutable std::optional<value_type> CurrentEvent;
    friend class MIDIBuffer;

    Iterator(MIDIBuffer const &Buffer, std::uint32_t Offset, std::uint32_t Size)
    : Buffer(Buffer), Offset(Offset), Size(Size) {}
  public:
    using iterator_category = std::forward_iterator_tag;
    using reference = value_type&;
    using const_reference = value_type const &;
    using pointer = value_type*;
    using const_pointer = value_type const *;

    friend bool operator==(Iterator const &Lhs, Iterator const &Rhs) {
      return (Lhs.Offset == Rhs.Offset) && (Lhs.Size == Rhs.Size);
    }
    friend bool operator!=(Iterator const &Lhs, Iterator const &Rhs) {
      return Lhs.Offset != Rhs.Offset || Lhs.Size != Rhs.Size;
    }
    Iterator &operator++() { Offset += 1; return *this; }
    Iterator const operator++(int) { auto Result = *this; ++*this; return Result; }
    const_reference operator*() const;
    const_pointer operator->() const { return &**this; }
  };
  Iterator begin() const;
  Iterator end() const;
};

class MIDIOut : public Port {
  friend class Client;
  MIDIOut(Client &, std::string_view Name);
      
public:
  MIDIBuffer buffer(std::uint32_t FrameCount);
};

class MIDIIn : public Port {
  friend class Client;
  MIDIIn(Client &, std::string_view Name);
      
public:
  MIDIBuffer const buffer(std::uint32_t FrameCount);
};

class Client : BrlCV::impl_ptr<Client>::unique {
  friend class BrlCV::impl_ptr<JACK::Port>::implementation;
public:
  explicit Client(std::string Name);
  Client(Client &&) noexcept;
  Client &operator=(Client &&) noexcept;
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;
  virtual ~Client();

  unsigned int sampleRate() const;
  bool isRealtime() const;

  AudioIn createAudioIn(std::string_view Name);
  AudioOut createAudioOut(std::string_view Name);
  MIDIIn createMIDIIn(std::string_view Name);
  MIDIOut createMIDIOut(std::string_view Name);

  void activate();
  void deactivate();

  void connect(std::string_view From, std::string_view To);
  void connect(std::string_view From, AudioIn const &To) {
    return connect(From, To.name());
  }
  void connect(AudioOut const &From, std::string_view To) {
    return connect(From.name(), To);
  }
  void connect(MIDIOut const &From, std::string_view To) {
    return connect(From.name(), To);
  }
  virtual int process(int FrameCount) = 0;
};

} // namespace JACK
#endif // BrlCV_JACK_HPP
