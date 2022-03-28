#include "ep128emu.hpp"
#include "system.hpp"
#include "libretrodisp.hpp"

namespace Ep128Emu
{

void LibretroDisplay::decodeLine(unsigned char *outBuf,
                                 const unsigned char *inBuf, size_t nBytes)
{
  const unsigned char *bufp = inBuf;
  unsigned char *endp = outBuf + 768;
  do
  {
    switch (bufp[0])
    {
    case 0x00:                        // blank
      do
      {
        outBuf[15] = outBuf[14] =
        outBuf[13] = outBuf[12] =
        outBuf[11] = outBuf[10] =
        outBuf[ 9] = outBuf[ 8] =
        outBuf[ 7] = outBuf[ 6] =
        outBuf[ 5] = outBuf[ 4] =
        outBuf[ 3] = outBuf[ 2] =
        outBuf[ 1] = outBuf[ 0] = 0x00;
        outBuf = outBuf + 16;
        bufp = bufp + 1;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x00);
      break;
    case 0x01:                        // 1 pixel, 256 colors
      do
      {
        outBuf[15] = outBuf[14] =
        outBuf[13] = outBuf[12] =
        outBuf[11] = outBuf[10] =
        outBuf[ 9] = outBuf[ 8] =
        outBuf[ 7] = outBuf[ 6] =
        outBuf[ 5] = outBuf[ 4] =
        outBuf[ 3] = outBuf[ 2] =
        outBuf[ 1] = outBuf[ 0] = bufp[1];
        outBuf = outBuf + 16;
        bufp = bufp + 2;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x01);
      break;
    case 0x02:                        // 2 pixels, 256 colors
      do
      {
        outBuf[ 7] = outBuf[ 6] =
        outBuf[ 5] = outBuf[ 4] =
        outBuf[ 3] = outBuf[ 2] =
        outBuf[ 1] = outBuf[ 0] = bufp[1];
        outBuf[15] = outBuf[14] =
        outBuf[13] = outBuf[12] =
        outBuf[11] = outBuf[10] =
        outBuf[ 9] = outBuf[ 8] = bufp[2];
        outBuf = outBuf + 16;
        bufp = bufp + 3;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x02);
      break;
    case 0x03:                        // 8 pixels, 2 colors
      do
      {
        unsigned char c0 = bufp[1];
        unsigned char c1 = bufp[2];
        unsigned char b = bufp[3];
        outBuf[ 1] = outBuf[ 0] = ((b & 128) ? c1 : c0);
        outBuf[ 3] = outBuf[ 2] = ((b &  64) ? c1 : c0);
        outBuf[ 5] = outBuf[ 4] = ((b &  32) ? c1 : c0);
        outBuf[ 7] = outBuf[ 6] = ((b &  16) ? c1 : c0);
        outBuf[ 9] = outBuf[ 8] = ((b &   8) ? c1 : c0);
        outBuf[11] = outBuf[10] = ((b &   4) ? c1 : c0);
        outBuf[13] = outBuf[12] = ((b &   2) ? c1 : c0);
        outBuf[15] = outBuf[14] = ((b &   1) ? c1 : c0);
        outBuf = outBuf + 16;
        bufp = bufp + 4;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x03);
      break;
    case 0x04:                        // 4 pixels, 256 colors
      do
      {
        outBuf[ 3] = outBuf[ 2] =
        outBuf[ 1] = outBuf[ 0] = bufp[1];
        outBuf[ 7] = outBuf[ 6] =
        outBuf[ 5] = outBuf[ 4] = bufp[2];
        outBuf[11] = outBuf[10] =
        outBuf[ 9] = outBuf[ 8] = bufp[3];
        outBuf[15] = outBuf[14] =
        outBuf[13] = outBuf[12] = bufp[4];
        outBuf = outBuf + 16;
        bufp = bufp + 5;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x04);
      break;
    case 0x06:                        // 16 (2*8) pixels, 2*2 colors
      do
      {
        unsigned char c0 = bufp[1];
        unsigned char c1 = bufp[2];
        unsigned char b = bufp[3];
        outBuf[ 0] = ((b & 128) ? c1 : c0);
        outBuf[ 1] = ((b &  64) ? c1 : c0);
        outBuf[ 2] = ((b &  32) ? c1 : c0);
        outBuf[ 3] = ((b &  16) ? c1 : c0);
        outBuf[ 4] = ((b &   8) ? c1 : c0);
        outBuf[ 5] = ((b &   4) ? c1 : c0);
        outBuf[ 6] = ((b &   2) ? c1 : c0);
        outBuf[ 7] = ((b &   1) ? c1 : c0);
        c0 = bufp[4];
        c1 = bufp[5];
        b = bufp[6];
        outBuf[ 8] = ((b & 128) ? c1 : c0);
        outBuf[ 9] = ((b &  64) ? c1 : c0);
        outBuf[10] = ((b &  32) ? c1 : c0);
        outBuf[11] = ((b &  16) ? c1 : c0);
        outBuf[12] = ((b &   8) ? c1 : c0);
        outBuf[13] = ((b &   4) ? c1 : c0);
        outBuf[14] = ((b &   2) ? c1 : c0);
        outBuf[15] = ((b &   1) ? c1 : c0);
        outBuf = outBuf + 16;
        bufp = bufp + 7;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x06);
      break;
    case 0x08:                        // 8 pixels, 256 colors
      do
      {
        outBuf[ 1] = outBuf[ 0] = bufp[1];
        outBuf[ 3] = outBuf[ 2] = bufp[2];
        outBuf[ 5] = outBuf[ 4] = bufp[3];
        outBuf[ 7] = outBuf[ 6] = bufp[4];
        outBuf[ 9] = outBuf[ 8] = bufp[5];
        outBuf[11] = outBuf[10] = bufp[6];
        outBuf[13] = outBuf[12] = bufp[7];
        outBuf[15] = outBuf[14] = bufp[8];
        outBuf = outBuf + 16;
        bufp = bufp + 9;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x08);
      break;
    default:                          // invalid flag byte
      do
      {
        *(outBuf++) = 0x00;
      }
      while (outBuf < endp);
      break;
    }
  }
  while (outBuf < endp);

  (void) nBytes;
#if 0
  if (size_t(bufp - inBuf) != nBytes)
    throw std::exception();
#endif
}

// --------------------------------------------------------------------------

void LibretroDisplay::Message_LineData::copyLine(const uint8_t *buf,
    size_t nBytes)
{
  nBytes_ = (unsigned int) nBytes;
  if (nBytes_ & 3U)
    buf_[nBytes_ / 4U] = 0U;
  if (nBytes_)
    std::memcpy(&(buf_[0]), buf, nBytes_);
}

LibretroDisplay::Message_LineData&
LibretroDisplay::Message_LineData::operator=(const Message_LineData& r)
{
  std::memcpy(&nBytes_, &(r.nBytes_),
              size_t((const char *) &(r.buf_[((r.nBytes_ + 7U) & (~7U)) / 4U])
                     - (const char *) &(r.nBytes_)));
  return (*this);
}

void LibretroDisplay::deleteMessage(Message *m)
{
  messageQueueMutex.lock();
  m->nxt = freeMessageStack;
  freeMessageStack = m;
  messageQueueMutex.unlock();
}

void LibretroDisplay::queueMessage(Message *m)
{
  messageQueueMutex.lock();
  if (exitFlag)
  {
    messageQueueMutex.unlock();
    std::free(m);
    return;
  }
  m->nxt = (Message *) 0;
  if (lastMessage)
    lastMessage->nxt = m;
  else
    messageQueue = m;
  lastMessage = m;
//  bool    isFrameDone = (m->msgType == Message::MsgType_FrameDone);
  messageQueueMutex.unlock();
  /*if (EP128EMU_UNLIKELY(isFrameDone))
  {
    if (!videoResampleEnabled)
    {
      threadLock.wait(1);
    }
  }*/
}


void LibretroDisplay::setDisplayParameters(const DisplayParameters& dp)
{
  // No other display parameters are supported.
  displayParameters.indexToRGBFunc = dp.indexToRGBFunc;
}

const VideoDisplay::DisplayParameters&
LibretroDisplay::getDisplayParameters() const
{
  return displayParameters;
}

void LibretroDisplay::drawLine(const uint8_t *buf, size_t nBytes)
{

//    printf("curline %d nBytes %d vsyncCnt %d \n",curLine,nBytes,vsyncCnt);
  if (!skippingFrame)
  {
    if (curLine >= 0 && curLine < (EP128EMU_LIBRETRO_SCREEN_HEIGHT + 2))
    {
      Message_LineData  *m = allocateMessage<Message_LineData>();
      m->lineNum = curLine;
      m->copyLine(buf, nBytes);
      queueMessage(m);
    }
  }
  if (vsyncCnt != 0)
  {
    curLine += 2;
    if (vsyncCnt >= (EP128EMU_VSYNC_MIN_LINES + 2 - EP128EMU_VSYNC_OFFSET) &&
        (vsyncState || vsyncCnt >= (EP128EMU_VSYNC_MAX_LINES
                                    + 2 - EP128EMU_VSYNC_OFFSET)))
    {
      vsyncCnt = 2 - EP128EMU_VSYNC_OFFSET;
    }
    vsyncCnt++;
  }
  else
  {
    curLine = (oddFrame ? -1 : 0);
    vsyncCnt++;
    frameDone();
    frameCount++;
  }
}

void LibretroDisplay::vsyncStateChange(bool newState, unsigned int currentSlot_)
{
  //printf("vsyncstatechange: vsync cnt %d slot %d currState %d newstate %d\n",vsyncCnt, currentSlot_, vsyncState ? 1 : 0, newState ? 1 : 0);
  vsyncState = newState;
  if (newState &&
      vsyncCnt >= (EP128EMU_VSYNC_MIN_LINES + 2 - EP128EMU_VSYNC_OFFSET))
  {
    vsyncCnt = 2 - EP128EMU_VSYNC_OFFSET;
    // Odd frames are detected by vSync activated in the middle of the line (half-line sync)
    oddFrame = (currentSlot_ >= 20U && currentSlot_ < 48U);
    if (oddFrame)
    {
      interlacedFrameCount = 3;
      //frame_bufActive = frame_bufSpare;
    }
    else interlacedFrameCount = interlacedFrameCount > 0 ? interlacedFrameCount - 1 : 0;
  }
}
// --------------------------------------------------------------------------

LibretroDisplay::LibretroDisplay(int xx, int yy, int ww, int hh,
                                 const char *lbl, bool useHalfFrame_)
  :     messageQueue((Message *) 0),
        lastMessage((Message *) 0),
        freeMessageStack((Message *) 0),
        messageQueueMutex(),
        lineBuffers((Message_LineData **) 0),
        curLine(0),
        vsyncCnt(0),
        skippingFrame(false),
        useHalfFrame(useHalfFrame_),
        framesPendingFlag(false),
        vsyncState(false),
        oddFrame(false),
        lineBuf((unsigned char *) 0),
        threadLock1(false),
        threadLock2(true),
        exitFlag(false),
        displayParameters(),
        savedDisplayParameters(),
        redrawFlag(false),
        prvFrameWasOdd(false),
        lastLineNum(-2),
#ifdef EP128EMU_USE_XRGB8888
        frame_buf1((uint32_t *) 0),
        frame_buf2((uint32_t *) 0),
#else
        frame_buf1((uint16_t *) 0),
        frame_buf2((uint16_t *) 0),
#endif // EP128EMU_USE_XRGB8888
        interlacedFrameCount(0),
        frameCount(0)
{
  try
  {
    lineBuffers = new Message_LineData*[EP128EMU_LIBRETRO_SCREEN_HEIGHT + 2];
    for (size_t n = 0; n < (EP128EMU_LIBRETRO_SCREEN_HEIGHT + 2); n++)
      lineBuffers[n] = (Message_LineData *) 0;
  }
  catch (...)
  {
    if (lineBuffers)
      delete[] lineBuffers;
    throw;
  }
  try
  {
    linesChanged = new bool[289];
    for (size_t n = 0; n < 289; n++)
      linesChanged[n] = false;
  }
  catch (...)
  {
    if (linesChanged)
      delete[] linesChanged;
    throw;
  }


#ifdef EP128EMU_USE_XRGB8888
  frameSize = ww * hh * sizeof(uint32_t);
  frame_buf1 = (uint32_t*) calloc(ww * hh, sizeof(uint32_t));
//  frame_buf2 = (uint32_t*) calloc(ww * hh, sizeof(uint32_t));
  frame_buf3 = (uint32_t*) calloc(ww * hh, sizeof(uint32_t));
#else
  frameSize = ww * hh * sizeof(uint16_t);
  frame_buf1 = (uint16_t*) calloc(ww * hh, sizeof(uint16_t));
//  frame_buf2 = (uint16_t*) calloc(ww * hh, sizeof(uint16_t));
  frame_buf3 = (uint16_t*) calloc(ww * hh, sizeof(uint16_t));
#endif // EP128EMU_USE_XRGB8888
  frame_bufActive = frame_buf1;
//  frame_bufReady = frame_buf2;
  frame_bufSpare = frame_buf3;
  lineBuf = (unsigned char*) calloc(ww, sizeof(unsigned char));
  this->start();
}

void LibretroDisplay::wakeDisplay(bool syncRequired)
{
  threadLock1.notify();
  if (syncRequired)
  {
    threadLock2.wait(10);
  }
}

void LibretroDisplay::run()
{
  bool frameDone;
  while (true)
  {
    if (exitFlag) break;
    frameDone = false;
    threadLock1.wait(10);
    do
    {
      frameDone = checkEvents();
      if (frameDone)
      {
        draw(frame_bufActive);
      }
    }
    while (frameDone);
    threadLock2.notify();
  }
}

void LibretroDisplay::frameDone()
{
  Message *m = allocateMessage<Message_FrameDone>();
  queueMessage(m);
}

bool LibretroDisplay::checkEvents()
{
  redrawFlag = false;
  while (true)
  {
    messageQueueMutex.lock();
    Message *m = messageQueue;
    if (m)
    {
      messageQueue = m->nxt;
      if (messageQueue)
      {
        if (!messageQueue->nxt)
          lastMessage = messageQueue;
      }
      else
        lastMessage = (Message *) 0;
    }
    messageQueueMutex.unlock();
    if (!m)
      break;

    if (EP128EMU_EXPECT(m->msgType == Message::MsgType_LineData))
    {
      Message_LineData  *msg;
      msg = static_cast<Message_LineData *>(m);
      int     lineNum = msg->lineNum;
      if (lineNum >= 0 && lineNum < 578)
      {
        lastLineNum = lineNum;
        /*       if ((lineNum & 1) == int(prvFrameWasOdd) &&
                   lineBuffers[lineNum ^ 1] != (Message_LineData *) 0)
               {
                 // non-interlaced mode: clear any old lines in the other field
                 linesChanged[lineNum >> 1] = true;
                 deleteMessage(lineBuffers[lineNum ^ 1]);
                 lineBuffers[lineNum ^ 1] = (Message_LineData *) 0;
               }*/
        // check if this line has changed
        if (lineBuffers[lineNum])
        {
          if (*(lineBuffers[lineNum]) == *msg)
          {
            deleteMessage(m);
            continue;
          }
        }
        linesChanged[lineNum >> 1] = true;
        if (lineBuffers[lineNum])
          deleteMessage(lineBuffers[lineNum]);
        lineBuffers[lineNum] = msg;
        continue;
      }
    }
    else if (m->msgType == Message::MsgType_FrameDone)
    {
      redrawFlag = true;
      deleteMessage(m);
      /*int     n = lastLineNum;
      prvFrameWasOdd = bool(n & 1);
      lastLineNum = (n & 1) - 2;
      if (n < 576)
      {
        // clear any remaining lines
        n = n | 1;
        do
        {
          n++;
          if (lineBuffers[n])
          {
            linesChanged[n >> 1] = true;
            deleteMessage(lineBuffers[n]);
            lineBuffers[n] = (Message_LineData *) 0;
          }
        }
        while (n < 577);
      }*/
      //noInputTimer.reset();
      //if (screenshotCallbackFlag)
      //  checkScreenshotCallback();
      break;
    }
    deleteMessage(m);
  }
  return redrawFlag;
}

LibretroDisplay::~LibretroDisplay()
{
  exitFlag = true;
  threadLock1.notify();
  this->join();

  free(frame_buf1);
  free(frame_buf2);
  free(frame_buf3);
  frame_buf1 = NULL;
  frame_buf2 = NULL;
  frame_buf3 = NULL;
  frame_bufActive = NULL;
  frame_bufReady = NULL;
  frame_bufSpare = NULL;
  free(lineBuf);
  lineBuf = NULL;
  messageQueueMutex.lock();
  while (freeMessageStack)
  {
    Message *m = freeMessageStack;
    freeMessageStack = m->nxt;
    std::free(m);
  }
  while (messageQueue)
  {
    Message *m = messageQueue;
    messageQueue = m->nxt;
    std::free(m);
  }
  lastMessage = (Message *) 0;
  messageQueueMutex.unlock();
  for (size_t n = 0; n < (EP128EMU_LIBRETRO_SCREEN_HEIGHT + 2); n++)
  {
    Message *m = lineBuffers[n];
    if (m)
    {
      lineBuffers[n] = (Message_LineData *) 0;
      std::free(m);
    }
  }
  delete[] lineBuffers;
}

void LibretroDisplay::limitFrameRate(bool isEnabled)
{
  (void) isEnabled;
}

void LibretroDisplay::draw(void * fb)
{
  firstNonzeroLine = EP128EMU_LIBRETRO_SCREEN_HEIGHT;
  int firstNonborderLine = EP128EMU_LIBRETRO_SCREEN_HEIGHT;
  lastNonzeroLine = 0;
  int lastNonborderLine = 0;

  borderColor = 0;
  for (int yc = 0; yc < EP128EMU_LIBRETRO_SCREEN_HEIGHT; yc++)
  {
    if (!interlacedFrameCount && (yc & 1))
    {
      continue;
    }
    if (lineBuffers[yc])
    {
      // decode video data
      const unsigned char *bufp = (unsigned char *) 0;
      size_t  nBytes = 0;
      bool nonzero = false;
      bool nonborder = false;
      lineBuffers[yc]->getLineData(bufp, nBytes);
      decodeLine(lineBuf,bufp,nBytes);

      for(int i=0; i<EP128EMU_LIBRETRO_SCREEN_WIDTH; i++)
      {
        unsigned char pixelValue = lineBuf[i];

        float rF=0.0;
        float gF=0.0;
        float bF=0.0;

        if (displayParameters.indexToRGBFunc)
          displayParameters.indexToRGBFunc(uint8_t(pixelValue), rF, gF, bF);
        rF = rF * 255.0f + 0.5f;
        gF = gF * 255.0f + 0.5f;
        bF = bF * 255.0f + 0.5f;
        unsigned char r = (unsigned char) (rF > 0.0f ? (rF < 255.5f ? rF : 255.5f) : 0.0f);
        unsigned char g = (unsigned char) (gF > 0.0f ? (gF < 255.5f ? gF : 255.5f) : 0.0f);
        unsigned char b = (unsigned char) (bF > 0.0f ? (bF < 255.5f ? bF : 255.5f) : 0.0f);
#ifdef EP128EMU_USE_XRGB8888
        // conversion to RGB_X888
        uint32_t pixel32 = (uint32_t) (r << 16 | g << 8 | b << 0);
        if (!nonzero && pixel32>0) {
          nonzero=true;
          if(borderColor == 0) borderColor = pixel32;
        }
        if (!nonborder && borderColor>0 && pixel32 != borderColor && pixel32 > 0) {nonborder=true;}
#else
        // conversion to RGB_565
        uint16_t pixel16 = (uint16_t) ((r&0xF8) << 8 | (g&0xFC) <<  3 | (b&0xF8) >> 3);
        if (!nonzero && pixel16>0) {
          nonzero=true;
          if (borderColor == 0) borderColor = pixel16;
          }
        if (!nonborder && borderColor>0 && pixel16 != borderColor && pixel16 > 0) {
           //printf("nonborder: at %d %d value %d instead of %d \n",yc,i,pixel16,borderColor);
           nonborder=true;}

#endif // EP128EMU_USE_XRGB8888
        if (interlacedFrameCount)
        {
#ifdef EP128EMU_USE_XRGB8888
          frame_bufActive[yc * EP128EMU_LIBRETRO_SCREEN_WIDTH + i] = pixel32;
#else
          frame_bufActive[yc * EP128EMU_LIBRETRO_SCREEN_WIDTH + i] = pixel16;
#endif // EP128EMU_USE_XRGB8888
          if (yc < EP128EMU_LIBRETRO_SCREEN_HEIGHT-1)
            frame_bufActive[(yc+1) * EP128EMU_LIBRETRO_SCREEN_WIDTH + i] = frame_bufReady[(yc+1) * EP128EMU_LIBRETRO_SCREEN_WIDTH + i];
        }
        else
        {
          if (useHalfFrame)
          {
            // use different addressing to achieve packed frame even in this case
#ifdef EP128EMU_USE_XRGB8888
            frame_bufActive[yc/2 * EP128EMU_LIBRETRO_SCREEN_WIDTH + i] = pixel32;
#else
            frame_bufActive[yc/2 * EP128EMU_LIBRETRO_SCREEN_WIDTH + i] = pixel16;
#endif // EP128EMU_USE_XRGB8888
          }
          else
          {
            // doublescan
#ifdef EP128EMU_USE_XRGB8888
            frame_bufActive[(yc)   * EP128EMU_LIBRETRO_SCREEN_WIDTH + i] = pixel32;
            frame_bufActive[(yc+1) * EP128EMU_LIBRETRO_SCREEN_WIDTH + i] = pixel32;
#else
            frame_bufActive[(yc)   * EP128EMU_LIBRETRO_SCREEN_WIDTH + i] = pixel16;
            frame_bufActive[(yc+1) * EP128EMU_LIBRETRO_SCREEN_WIDTH + i] = pixel16;
#endif // EP128EMU_USE_XRGB8888
          }
        }
      }
      if(nonzero) {
        if(yc > lastNonzeroLine) lastNonzeroLine = yc;
        if(yc < firstNonzeroLine) firstNonzeroLine = yc;
      }
      if(nonborder) {
        if(yc > lastNonborderLine) lastNonborderLine = yc;
        if(yc < firstNonborderLine) firstNonborderLine = yc;
      }

    }

  }
  //printf("nonzero: %d %d %d %d %d\n",firstNonzeroLine,lastNonzeroLine, firstNonborderLine, lastNonborderLine, borderColor);
  if (firstNonzeroLine == 0 ) firstNonzeroLine = firstNonborderLine;
  // screen end can vary, see Beach Head
  if (lastNonzeroLine > EP128EMU_LIBRETRO_SCREEN_HEIGHT-3 || lastNonzeroLine > lastNonborderLine + 20) lastNonzeroLine = lastNonborderLine;
#ifdef EP128EMU_USE_XRGB8888
//  uint32_t* swapPtr = frame_bufReady;
#else
//  uint16_t* swapPtr = frame_bufReady;
#endif
  frame_bufReady = frame_bufActive;
//    frame_bufActive = swapPtr;

}

}       // namespace Ep128Emu

