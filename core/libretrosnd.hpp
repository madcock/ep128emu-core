#ifndef EP128EMU_LIBRETROSND_HPP
#define EP128EMU_LIBRETROSND_HPP

#include "ep128emu.hpp"
#include "system.hpp"
#include "soundio.hpp"
#include <vector>

namespace Ep128Emu {

class AudioOutput_libretro : public AudioOutput {
   private:
    struct Buffer {
      std::vector< int16_t >    audioData;
      ThreadLock  lrLock;
      ThreadLock  epLock;
      size_t      writePos;
      Buffer()
        : lrLock(true), epLock(true), writePos(0)
      {
      }
      ~Buffer()
      {
      }
    };
    std::vector< Buffer >   buffers;
    size_t        writeBufIndex;
    size_t        readBufIndex;
    size_t        readSubBufIndex;
    Mutex         forwardMutex;

   public:
    AudioOutput_libretro();
    virtual ~AudioOutput_libretro();
    virtual void sendAudioData(const int16_t *buf, size_t nFrames);
    virtual void forwardAudioData(int16_t *buf_out, size_t* nFrames, int expectedFrames);
    virtual void closeDevice();
  };
}       // namespace Ep128Emu

#endif  // EP128EMU_LIBRETROSND_HPP

