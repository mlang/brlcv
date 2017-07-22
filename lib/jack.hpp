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
  Port(Port &&);
  Port &operator=(Port &&);
  Port(const Port &) = delete;
  Port &operator=(const Port &) = delete;
};

class AudioIn : public Port {
  friend class Client;
  AudioIn(Client *, std::string Name);
      
public:
  gsl::span<float> buffer(std::int32_t Frames);
};

class AudioOut : public Port {
  friend class Client;
  AudioOut(Client *, std::string Name);

public:
  gsl::span<float> buffer(std::int32_t Frames);
};

class Client {
  struct Implementation;
  std::unique_ptr<Implementation> JACK;
  friend class Port;

public:
  Client(std::string Name);
  Client(Client &&);
  Client &operator=(Client &&);
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;
  virtual ~Client();

  unsigned int sampleRate() const;
  bool isRealtime() const;

  AudioIn createAudioIn(std::string Name);
  AudioOut createAudioOut(std::string Name);

  void activate();
  void deactivate();

  virtual int process(std::uint32_t nframes) = 0;
};

}
