
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2009 Istvan Varga <istvanv@users.sourceforge.net>
// http://sourceforge.net/projects/ep128emu/
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

#ifndef EP128EMU_CRTC6845_HPP
#define EP128EMU_CRTC6845_HPP

#include "ep128emu.hpp"

namespace CPC464 {

  class CRTC6845 {
   protected:
    // register  0: horizontal total - 1
    // register  1: horizontal displayed
    // register  2: horizontal sync position
    // register  3: (Vsync_lines & 15) * 16 + (Hsync_chars & 15)
    // register  4: vertical total - 1 (in characters)
    // register  5: vertical total adjust (in scanlines)
    // register  6: vertical displayed (in characters, / 2 if interlaced video)
    // register  7: vertical sync position (in characters)
    // register  8: mode control
    //                0, 1: non-interlaced mode
    //                2:    interlace VSYNC only
    //                3:    interlaced video mode
    // register  9: scanlines per character - 1 (0 to 31)
    // register 10: cursor start line (0 to 31)
    //                +  0: no flash
    //                + 32: cursor not displayed
    //                + 64: flash (16 frames)
    //                + 96: flash (32 frames)
    // register 11: cursor end line (0 to 31)
    // register 12: start address high (bits 8 to 13)
    // register 13: start address low (bits 0 to 7)
    // register 14: cursor address high (bits 8 to 13)
    // register 15: cursor address low (bits 0 to 7)
    // register 16: lightpen address high (bits 8 to 13)
    // register 17: lightpen address low (bits 0 to 7)
    uint8_t     registers[18];
    uint8_t     horizontalPos;
    // bit 0: display enabled H
    // bit 1: display enabled V
    uint8_t     displayEnableFlags;
    // bit 0: horizontal sync
    // bit 1: vertical sync
    uint8_t     syncFlags;
    uint8_t     hSyncCnt;
    uint8_t     vSyncCnt;
    uint8_t     rowAddress;             // 0 to 31
    uint8_t     verticalPos;
    bool        oddField;               // for interlace
    uint16_t    characterAddress;       // 0 to 0x3FFF
    uint16_t    characterAddressLatch;
    uint16_t    cursorAddress;          // copied from registers 14 and 15
    // bit 0: 0 if row address is in cursor line range
    // bit 1: 0 if cursor enable / flash is on
    uint8_t     cursorFlags;
    uint8_t     cursorFlashCnt;
    bool        endOfFrameFlag;
   public:
    CRTC6845();
    virtual ~CRTC6845();
    void reset();
    EP128EMU_REGPARM1 void runOneCycle();

    EP128EMU_INLINE bool getDisplayEnabled() const
    {
      return bool(displayEnableFlags);
    }
    EP128EMU_INLINE bool getHSyncState() const
    {
      return bool(syncFlags & 0x01);
    }
    EP128EMU_INLINE bool getVSyncState() const
    {
      return bool(syncFlags & 0x02);
    }
    EP128EMU_INLINE bool getVSyncInterlace() const
    {
      return (bool(this->registers[8] & 0x02) && oddField);
    }
    EP128EMU_INLINE uint8_t getRowAddress() const
    {
      return rowAddress;
    }
    EP128EMU_INLINE uint16_t getMemoryAddress() const
    {
      return characterAddress;
    }
    EP128EMU_INLINE bool getCursorEnabled() const
    {
      return ((characterAddress == cursorAddress) && (cursorFlags == 0));
    }
    EP128EMU_REGPARM2 uint8_t readRegister(uint16_t addr) const;
    EP128EMU_REGPARM3 void writeRegister(uint16_t addr, uint8_t value);
    // --------
  };

}       // namespace CPC464

#endif  // EP128EMU_CRTC6845_HPP
