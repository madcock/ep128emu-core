
// ep128emu-core -- libretro core version of the ep128emu emulator
// Copyright (C) 2022 Zoltan Balogh
// https://github.com/zoltanvb/ep128emu-core
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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

