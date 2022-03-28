
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://sourceforge.net/projects/ep128emu/
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

#ifndef EP128EMU_LIBRETRODISP_HPP
#define EP128EMU_LIBRETRODISP_HPP

#include "ep128emu.hpp"
#include "system.hpp"
#include "display.hpp"
#include "libretro-funcs.hpp"

namespace Ep128Emu {

  class LibretroDisplay : public VideoDisplay, private Thread {
   protected:
    class Message {
     public:
      enum {
        MsgType_None = 0,
        MsgType_LineData = 1,
        MsgType_FrameDone = 2,
        MsgType_SetParameters = 3
      };
      Message   *nxt;
      intptr_t  msgType;
      // --------
      Message()
        : nxt((Message *) 0),
          msgType(MsgType_None)
      {
      }
     protected:
      Message(int msgType_)
        : nxt((Message *) 0),
          msgType(msgType_)
      {
      }
    };
    class Message_LineData : public Message {
     private:
      // number of bytes in buffer
      unsigned int  nBytes_;
     public:
      // line number
      int       lineNum;
     private:
      // a line of 768 pixels needs a maximum space of 768 * (9 / 16) = 432
      // ( = 108 * 4) bytes in compressed format
      uint32_t  buf_[108];
     public:
      Message_LineData()
        : Message(MsgType_LineData)
      {
        nBytes_ = 0U;
        lineNum = 0;
      }
      // copy a line (768 pixels in compressed format) to the buffer
      void copyLine(const uint8_t *buf, size_t nBytes);
      inline void getLineData(const unsigned char*& buf, size_t& nBytes)
      {
        buf = reinterpret_cast<unsigned char *>(&(buf_[0]));
        nBytes = nBytes_;
      }
      bool operator==(const Message_LineData& r) const
      {
        if (r.nBytes_ != nBytes_)
          return false;
        size_t  n = (nBytes_ + 3U) >> 2;
        for (size_t i = 0; i < n; i++) {
          if (r.buf_[i] != buf_[i])
            return false;
        }
        return true;
      }
      Message_LineData& operator=(const Message_LineData& r);
    };
    class Message_FrameDone : public Message {
     public:
      Message_FrameDone()
        : Message(MsgType_FrameDone)
      {
      }
    };
    class Message_SetParameters : public Message {
     public:
      DisplayParameters dp;
      Message_SetParameters()
        : Message(MsgType_SetParameters),
          dp()
      {
      }
    };
    template <typename T>
    T * allocateMessage()
    {
      void  *m_ = (void *) 0;
      messageQueueMutex.lock();
      if (freeMessageStack) {
        Message *m = freeMessageStack;
        freeMessageStack = m->nxt;
        m_ = m;
      }
      messageQueueMutex.unlock();
      if (!m_) {
        // allocate space that is enough for the largest message type
        m_ = std::malloc((sizeof(Message_LineData) | 15) + 1);
        if (!m_)
          throw std::bad_alloc();
      }
      T *m;
      try {
        m = new(m_) T();
      }
      catch (...) {
        std::free(m_);
        throw;
      }
      return m;
    }
    void deleteMessage(Message *m);
    void queueMessage(Message *m);
    void decodeLine(unsigned char *outBuf,
                           const unsigned char *inBuf, size_t nBytes);
    void frameDone();
    void run();
    // ----------------
    Message       *messageQueue;
    Message       *lastMessage;
    Message       *freeMessageStack;
    Mutex         messageQueueMutex;
    // for 578 lines (576 + 2 border)
    Message_LineData  **lineBuffers;
    int           curLine;
    int           vsyncCnt;
    int           framesPending;
    bool          skippingFrame;
    bool          useHalfFrame;
    bool          framesPendingFlag;
    bool          vsyncState;
    bool          oddFrame;
    unsigned char * lineBuf;
    ThreadLock      threadLock1;
    ThreadLock      threadLock2;
    volatile bool videoResampleEnabled;
    volatile bool exitFlag;
    volatile bool limitFrameRateFlag;
    DisplayParameters   displayParameters;
    DisplayParameters   savedDisplayParameters;
    Timer         limitFrameRateTimer;
//    ThreadLock    threadLock;
    void          (*screenshotCallback)(void *,
                                        const unsigned char *, int, int);
    void          *screenshotCallbackUserData;
    bool          screenshotCallbackFlag;
    bool          redrawFlag;
    bool          prvFrameWasOdd;
    int           lastLineNum;
        bool          *linesChanged;
   public:
#ifdef EP128EMU_USE_XRGB8888
    uint32_t *frame_buf1;
    uint32_t *frame_buf2;
    uint32_t *frame_buf3;
    uint32_t *frame_bufActive;
    uint32_t *frame_bufReady;
    uint32_t *frame_bufSpare;
    uint32_t *frame_bufLastReleased;
    uint32_t borderColor;
#else
    uint16_t *frame_buf1;
    uint16_t *frame_buf2;
    uint16_t *frame_buf3;
    uint16_t *frame_bufActive;
    uint16_t *frame_bufReady;
    uint16_t *frame_bufSpare;
    uint16_t *frame_bufLastReleased;
    uint16_t borderColor;
#endif
    uint32_t frameSize;
    uint32_t interlacedFrameCount;
    uint32_t frameCount;
    int           firstNonzeroLine;
    int           lastNonzeroLine;
    LibretroDisplay(int xx, int yy, int ww, int hh,
                               const char *lbl, bool useHalfFrame_);
    virtual ~LibretroDisplay();
    /*!
     * Set color correction and other display parameters
     * (see 'struct DisplayParameters' above for more information).
     */
    virtual void setDisplayParameters(const DisplayParameters& dp);
    virtual const DisplayParameters& getDisplayParameters() const;
    /*!
     * Draw next line of display.
     * 'buf' defines a line of 768 pixels, as 48 groups of 16 pixels each,
     * in the following format: the first byte defines the number of
     * additional bytes that encode the 16 pixels to be displayed. The data
     * length also determines the pixel format, and can have the following
     * values:
     *   0x01: one 8-bit color index (pixel width = 16)
     *   0x02: two 8-bit color indices (pixel width = 8)
     *   0x03: two 8-bit color indices for background (bit value = 0) and
     *         foreground (bit value = 1) color, followed by a 8-bit bitmap
     *         (msb first, pixel width = 2)
     *   0x04: four 8-bit color indices (pixel width = 4)
     *   0x06: similar to 0x03, but there are two sets of colors/bitmap
     *         (c0a, c1a, bitmap_a, c0b, c1b, bitmap_b) and the pixel width
     *         is 1
     *   0x08: eight 8-bit color indices (pixel width = 2)
     * The buffer contains 'nBytes' (in the range of 96 to 432) bytes of data.
     */
    virtual void drawLine(const uint8_t *buf, size_t nBytes);
    /*!
     * Should be called at the beginning (newState = true) and end
     * (newState = false) of VSYNC. 'currentSlot_' is the position within
     * the current line (0 to 56).
     */
    virtual void vsyncStateChange(bool newState, unsigned int currentSlot_);
    /*!
     * Read and process messages sent by the child thread. Returns true if
     * redraw() needs to be called to update the display.
     */
    virtual bool checkEvents();
    /*!
     * If enabled, limit the number of frames displayed per second to a
     * maximum of 50.
     */
    virtual void limitFrameRate(bool isEnabled);
    virtual void draw(void* fb);
    void wakeDisplay(bool syncRequired);

  };


}       // namespace Ep128Emu

#endif  // EP128EMU_FLDISP_HPP

