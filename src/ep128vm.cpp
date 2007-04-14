
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2007 Istvan Varga <istvanv@users.sourceforge.net>
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

#include "ep128emu.hpp"
#include "z80/z80.hpp"
#include "memory.hpp"
#include "ioports.hpp"
#include "dave.hpp"
#include "nick.hpp"
#include "soundio.hpp"
#include "display.hpp"
#include "vm.hpp"
#include "ep128vm.hpp"
#include "z80/disasm.hpp"

#include <cstdio>
#include <vector>

static void writeDemoTimeCnt(Ep128Emu::File::Buffer& buf, uint64_t n)
{
  uint64_t  mask = uint64_t(0x7F) << 49;
  uint8_t   rshift = 49;
  while (rshift != 0 && !(n & mask)) {
    mask >>= 7;
    rshift -= 7;
  }
  while (rshift != 0) {
    buf.writeByte(uint8_t((n & mask) >> rshift) | 0x80);
    mask >>= 7;
    rshift -= 7;
  }
  buf.writeByte(uint8_t(n) & 0x7F);
}

static uint64_t readDemoTimeCnt(Ep128Emu::File::Buffer& buf)
{
  uint64_t  n = 0U;
  uint8_t   i = 8, c;
  do {
    c = buf.readByte();
    n = (n << 7) | uint64_t(c & 0x7F);
    i--;
  } while ((c & 0x80) != 0 && i != 0);
  return n;
}

namespace Ep128 {

  inline void Ep128VM::updateCPUCycles(int cycles)
  {
    cpuCyclesRemaining -= (int64_t(cycles) << 32);
    if (memoryTimingEnabled) {
      while (cpuSyncToNickCnt >= cpuCyclesRemaining)
        cpuSyncToNickCnt -= cpuCyclesPerNickCycle;
    }
  }

  inline void Ep128VM::videoMemoryWait()
  {
    // use a fixed latency setting of 0.5625 Z80 cycles
    cpuCyclesRemaining -= (((cpuCyclesRemaining - cpuSyncToNickCnt)
                            + (int64_t(0xC8000000UL) << 1))
                           & (int64_t(-1) - int64_t(0xFFFFFFFFUL)));
    cpuSyncToNickCnt -= cpuCyclesPerNickCycle;
  }

  Ep128VM::Z80_::Z80_(Ep128VM& vm_)
    : Z80(),
      vm(vm_),
      defaultDeviceIsFILE(false)
  {
  }

  Ep128VM::Z80_::~Z80_()
  {
    closeAllFiles();
  }

  uint8_t Ep128VM::Z80_::readMemory(uint16_t addr)
  {
    int     nCycles = 3;
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          nCycles++;
      }
      else
        vm.videoMemoryWait();
    }
    vm.updateCPUCycles(nCycles);
    return (vm.memory.read(addr));
  }

  uint16_t Ep128VM::Z80_::readMemoryWord(uint16_t addr)
  {
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        vm.updateCPUCycles(vm.memoryWaitMode == 0 ? 4 : 3);
      }
      else {
        vm.videoMemoryWait();
        vm.updateCPUCycles(3);
      }
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC) {
        vm.updateCPUCycles(vm.memoryWaitMode == 0 ? 4 : 3);
      }
      else {
        vm.videoMemoryWait();
        vm.updateCPUCycles(3);
      }
    }
    else
      vm.updateCPUCycles(6);
    uint16_t  retval = vm.memory.read(addr);
    retval |= (uint16_t(vm.memory.read((addr + 1) & 0xFFFF) << 8));
    return retval;
  }

  uint8_t Ep128VM::Z80_::readOpcodeFirstByte()
  {
    int       nCycles = 4;
    uint16_t  addr = uint16_t(R.PC.W.l);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode < 2)
          nCycles++;
      }
      else
        vm.videoMemoryWait();
    }
    vm.updateCPUCycles(nCycles);
    if (!vm.singleStepModeEnabled)
      return vm.memory.read(addr);
    // single step mode
    return vm.checkSingleStepModeBreak();
  }

  uint8_t Ep128VM::Z80_::readOpcodeSecondByte()
  {
    int       nCycles = 4;
    uint16_t  addr = (uint16_t(R.PC.W.l) + uint16_t(1)) & uint16_t(0xFFFF);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode < 2)
          nCycles++;
      }
      else
        vm.videoMemoryWait();
    }
    vm.updateCPUCycles(nCycles);
    return vm.memory.read(addr);
  }

  uint8_t Ep128VM::Z80_::readOpcodeByte(int offset)
  {
    int       nCycles = 3;
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          nCycles++;
      }
      else
        vm.videoMemoryWait();
    }
    vm.updateCPUCycles(nCycles);
    return (vm.memory.read(addr));
  }

  uint16_t Ep128VM::Z80_::readOpcodeWord(int offset)
  {
    uint16_t  addr = uint16_t((int(R.PC.W.l) + offset) & 0xFFFF);
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        vm.updateCPUCycles(vm.memoryWaitMode == 0 ? 4 : 3);
      }
      else {
        vm.videoMemoryWait();
        vm.updateCPUCycles(3);
      }
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC) {
        vm.updateCPUCycles(vm.memoryWaitMode == 0 ? 4 : 3);
      }
      else {
        vm.videoMemoryWait();
        vm.updateCPUCycles(3);
      }
    }
    else
      vm.updateCPUCycles(6);
    uint16_t  retval = vm.memory.read(addr);
    retval |= (uint16_t(vm.memory.read((addr + 1) & 0xFFFF) << 8));
    return retval;
  }

  void Ep128VM::Z80_::writeMemory(uint16_t addr, uint8_t value)
  {
    int     nCycles = 3;
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        if (vm.memoryWaitMode == 0)
          nCycles++;
      }
      else
        vm.videoMemoryWait();
    }
    vm.updateCPUCycles(nCycles);
    vm.memory.write(addr, value);
  }

  void Ep128VM::Z80_::writeMemoryWord(uint16_t addr, uint16_t value)
  {
    if (vm.memoryTimingEnabled) {
      if (vm.pageTable[addr >> 14] < 0xFC) {
        vm.updateCPUCycles(vm.memoryWaitMode == 0 ? 4 : 3);
      }
      else {
        vm.videoMemoryWait();
        vm.updateCPUCycles(3);
      }
      if (vm.pageTable[((addr + 1) & 0xFFFF) >> 14] < 0xFC) {
        vm.updateCPUCycles(vm.memoryWaitMode == 0 ? 4 : 3);
      }
      else {
        vm.videoMemoryWait();
        vm.updateCPUCycles(3);
      }
    }
    else
      vm.updateCPUCycles(6);
    vm.memory.write(addr, uint8_t(value) & 0xFF);
    vm.memory.write((addr + 1) & 0xFFFF, uint8_t(value >> 8) & 0xFF);
  }

  void Ep128VM::Z80_::doOut(uint16_t addr, uint8_t value)
  {
    vm.ioPorts.write(addr, value);
  }

  uint8_t Ep128VM::Z80_::doIn(uint16_t addr)
  {
    return vm.ioPorts.read(addr);
  }

  void Ep128VM::Z80_::updateCycle()
  {
    vm.cpuCyclesRemaining -= (int64_t(1) << 32);
    // assume cpuFrequency > nickFrequency
    if (vm.cpuSyncToNickCnt >= vm.cpuCyclesRemaining)
      vm.cpuSyncToNickCnt -= vm.cpuCyclesPerNickCycle;
  }

  void Ep128VM::Z80_::updateCycles(int cycles)
  {
    vm.updateCPUCycles(cycles);
  }

  void Ep128VM::Z80_::tapePatch()
  {
    if ((R.PC.W.l & 0x3FFF) > 0x3FFC ||
        !(vm.fileIOEnabled &&
          vm.memory.isSegmentROM(vm.getMemoryPage(int(R.PC.W.l >> 14)))))
      return;
    if (vm.readMemory((uint16_t(R.PC.W.l) + 2) & 0xFFFF, true) != 0xFE)
      return;
    uint8_t n = vm.readMemory((uint16_t(R.PC.W.l) + 3) & 0xFFFF, true);
    if (n > 0x0F)
      return;
    if (vm.isRecordingDemo || vm.isPlayingDemo) {
      if (n >= 0x01 && n <= 0x0B) {
        // file I/O is disabled while recording or playing demo
        R.AF.B.h = 0xE7;
        return;
      }
    }
    std::map< uint8_t, std::FILE * >::iterator  i_;
    i_ = fileChannels.find(uint8_t(R.AF.B.h));
    if ((n >= 0x01 && n <= 0x02) && i_ != fileChannels.end()) {
      R.AF.B.h = 0xF9;          // channel already exists
      return;
    }
    if ((n >= 0x03 && n <= 0x0B) && i_ == fileChannels.end()) {
      R.AF.B.h = 0xFB;          // channel does not exist
      return;
    }
    switch (n) {
    case 0x00:                          // INTERRUPT
      R.AF.B.h = 0x00;
      break;
    case 0x01:                          // OPEN CHANNEL
      {
        std::string fileName;
        uint16_t    nameAddr = uint16_t(R.DE.W);
        uint8_t     nameLen = vm.readMemory(nameAddr, true);
        while (nameLen) {
          nameAddr = (nameAddr + 1) & 0xFFFF;
          char  c = char(vm.readMemory(nameAddr, true));
          if (c == '\0')
            break;
          fileName += c;
          nameLen--;
        }
        uint8_t   chn = uint8_t(R.AF.B.h);
        std::FILE *f = (std::FILE *) 0;
        R.AF.B.h = 0x00;
        try {
          int   err = vm.openFileInWorkingDirectory(f, fileName, "rb");
          if (err == 0)
            fileChannels.insert(std::pair< uint8_t, std::FILE * >(chn, f));
          else {
            switch (err) {
            case -2:
              R.AF.B.h = 0xA6;
              break;
            case -3:
              R.AF.B.h = 0xCF;
              break;
            case -4:
            case -5:
              R.AF.B.h = 0xA1;
              break;
            default:
              R.AF.B.h = 0xBA;
            }
          }
        }
        catch (...) {
          if (f)
            std::fclose(f);
          R.AF.B.h = 0xBA;
        }
      }
      break;
    case 0x02:                          // CREATE CHANNEL
      {
        std::string fileName;
        uint16_t    nameAddr = uint16_t(R.DE.W);
        uint8_t     nameLen = vm.readMemory(nameAddr, true);
        while (nameLen) {
          nameAddr = (nameAddr + 1) & 0xFFFF;
          char  c = char(vm.readMemory(nameAddr, true));
          if (c == '\0')
            break;
          fileName += c;
          nameLen--;
        }
        uint8_t   chn = uint8_t(R.AF.B.h);
        std::FILE *f = (std::FILE *) 0;
        R.AF.B.h = 0x00;
        try {
          int   err = vm.openFileInWorkingDirectory(f, fileName, "wb");
          if (err == 0)
            fileChannels.insert(std::pair< uint8_t, std::FILE * >(chn, f));
          else {
            switch (err) {
            case -2:
              R.AF.B.h = 0xA6;
              break;
            case -3:
              R.AF.B.h = 0xCF;
              break;
            case -4:
            case -5:
              R.AF.B.h = 0xA1;
              break;
            default:
              R.AF.B.h = 0xBA;
            }
          }
        }
        catch (...) {
          if (f)
            std::fclose(f);
          R.AF.B.h = 0xBA;
        }
      }
      break;
    case 0x03:                          // CLOSE CHANNEL
      if (std::fclose((*i_).second) == 0)
        R.AF.B.h = 0x00;
      else
        R.AF.B.h = 0xE4;
      (*i_).second = (std::FILE *) 0;
      fileChannels.erase(i_);
      break;
    case 0x04:                          // DESTROY CHANNEL
      // FIXME: this should remove the file;
      // for now, it is the same as "close channel"
      if (std::fclose((*i_).second) == 0)
        R.AF.B.h = 0x00;
      else
        R.AF.B.h = 0xE4;
      (*i_).second = (std::FILE *) 0;
      fileChannels.erase(i_);
      break;
    case 0x05:                          // READ CHARACTER
      {
        int   c = std::fgetc((*i_).second);
        R.AF.B.h = uint8_t(c != EOF ? 0x00 : 0xE4);
        R.BC.B.h = uint8_t(c & 0xFF);
      }
      break;
    case 0x06:                          // READ BLOCK
      R.AF.B.h = 0x00;
      while (R.BC.W != 0x0000) {
        int     c = std::fgetc((*i_).second);
        if (c == EOF) {
          R.AF.B.h = 0xE4;
          break;
        }
        writeUserMemory(uint16_t(R.DE.W), uint8_t(c & 0xFF));
        R.DE.W = (R.DE.W + 1) & 0xFFFF;
        R.BC.W = (R.BC.W - 1) & 0xFFFF;
      }
      break;
    case 0x07:                          // WRITE CHARACTER
      if (std::fputc(R.BC.B.h, (*i_).second) != EOF)
        R.AF.B.h = 0x00;
      else
        R.AF.B.h = 0xE4;
      if (std::fseek((*i_).second, 0L, SEEK_CUR) < 0)
        R.AF.B.h = 0xE4;
      break;
    case 0x08:                          // WRITE BLOCK
      R.AF.B.h = 0x00;
      while (R.BC.W != 0x0000) {
        uint8_t c = readUserMemory(uint16_t(R.DE.W));
        if (std::fputc(c, (*i_).second) == EOF) {
          R.AF.B.h = 0xE4;
          break;
        }
        R.DE.W = (R.DE.W + 1) & 0xFFFF;
        R.BC.W = (R.BC.W - 1) & 0xFFFF;
      }
      if (std::fseek((*i_).second, 0L, SEEK_CUR) < 0)
        R.AF.B.h = 0xE4;
      break;
    case 0x09:                          // CHANNEL READ STATUS
      R.BC.B.l = uint8_t(std::feof((*i_).second) == 0 ? 0x00 : 0xFF);
      R.AF.B.h = 0x00;
      break;
    case 0x0A:                          // SET/GET CHANNEL STATUS
      // TODO: implement this
      R.AF.B.h = 0xE7;
      R.BC.B.l = 0x00;
      break;
    case 0x0B:                          // SPECIAL FUNCTION
      R.AF.B.h = 0xE7;
      break;
    case 0x0C:                          // INITIALIZATION
      closeAllFiles();
      R.AF.B.h = 0x00;
      break;
    case 0x0D:                          // BUFFER MOVED
      R.AF.B.h = 0x00;
      break;
    case 0x0E:                          // get if default device is 'FILE:'
      R.AF.B.h = uint8_t(defaultDeviceIsFILE &&
                         !(vm.isRecordingDemo ||
                           vm.isPlayingDemo) ? 0x01 : 0x00);
      break;
    case 0x0F:                          // set if default device is 'FILE:'
      if (!(vm.isRecordingDemo || vm.isPlayingDemo))
        defaultDeviceIsFILE = (R.AF.B.h != 0x00);
      break;
    default:
      R.AF.B.h = 0xE7;
    }
  }

  uint8_t Ep128VM::Z80_::readUserMemory(uint16_t addr)
  {
    uint8_t   segment = vm.memory.readRaw(0x003FFFFCU | uint32_t(addr >> 14));
    uint32_t  addr_ = (uint32_t(segment) << 14) | uint32_t(addr & 0x3FFF);
    return vm.memory.readRaw(addr_);
  }

  void Ep128VM::Z80_::writeUserMemory(uint16_t addr, uint8_t value)
  {
    uint8_t   segment = vm.memory.readRaw(0x003FFFFCU | uint32_t(addr >> 14));
    uint32_t  addr_ = (uint32_t(segment) << 14) | uint32_t(addr & 0x3FFF);
    vm.memory.writeRaw(addr_, value);
  }

  void Ep128VM::Z80_::closeAllFiles()
  {
    std::map< uint8_t, std::FILE * >::iterator  i;
    for (i = fileChannels.begin(); i != fileChannels.end(); i++) {
      if ((*i).second != (std::FILE *) 0) {
        std::fclose((*i).second);
        (*i).second = (std::FILE *) 0;
      }
    }
    fileChannels.clear();
  }

  // --------------------------------------------------------------------------

  Ep128VM::Memory_::Memory_(Ep128VM& vm_)
    : Memory(),
      vm(vm_)
  {
  }

  Ep128VM::Memory_::~Memory_()
  {
  }

  void Ep128VM::Memory_::breakPointCallback(bool isWrite,
                                            uint16_t addr, uint8_t value)
  {
    if (vm.noBreakOnDataRead && !isWrite) {
      if (uint16_t(vm.z80.getReg().PC.W.l) != addr)
        return;
    }
    if (!vm.memory.checkIgnoreBreakPoint(vm.z80.getReg().PC.W.l)) {
      vm.breakPointCallback(vm.breakPointCallbackUserData,
                            false, isWrite, addr, value);
    }
  }

  // --------------------------------------------------------------------------

  Ep128VM::IOPorts_::IOPorts_(Ep128VM& vm_)
    : IOPorts(),
      vm(vm_)
  {
  }

  Ep128VM::IOPorts_::~IOPorts_()
  {
  }

  void Ep128VM::IOPorts_::breakPointCallback(bool isWrite,
                                             uint16_t addr, uint8_t value)
  {
    if (!vm.memory.checkIgnoreBreakPoint(vm.z80.getReg().PC.W.l)) {
      vm.breakPointCallback(vm.breakPointCallbackUserData,
                            true, isWrite, addr, value);
    }
  }

  // --------------------------------------------------------------------------

  Ep128VM::Dave_::Dave_(Ep128VM& vm_)
    : Dave(),
      vm(vm_)
  {
  }

  Ep128VM::Dave_::~Dave_()
  {
  }

  void Ep128VM::Dave_::setMemoryPage(uint8_t page, uint8_t segment)
  {
    vm.pageTable[page] = segment;
    vm.memory.setPage(page, segment);
  }

  void Ep128VM::Dave_::setMemoryWaitMode(int mode)
  {
    vm.memoryWaitMode = uint8_t(mode);
  }

  void Ep128VM::Dave_::setRemote1State(int state)
  {
    vm.isRemote1On = (state != 0);
    vm.setTapeMotorState(vm.isRemote1On || vm.isRemote2On);
  }

  void Ep128VM::Dave_::setRemote2State(int state)
  {
    vm.isRemote2On = (state != 0);
    vm.setTapeMotorState(vm.isRemote1On || vm.isRemote2On);
  }

  void Ep128VM::Dave_::interruptRequest()
  {
    vm.z80.triggerInterrupt();
  }

  void Ep128VM::Dave_::clearInterruptRequest()
  {
    vm.z80.clearInterrupt();
  }

  // --------------------------------------------------------------------------

  Ep128VM::Nick_::Nick_(Ep128VM& vm_)
    : Nick(vm_.memory),
      vm(vm_)
  {
  }

  Ep128VM::Nick_::~Nick_()
  {
  }

  void Ep128VM::Nick_::irqStateChange(bool newState)
  {
    vm.dave.setInt1State(newState ? 1 : 0);
  }

  void Ep128VM::Nick_::drawLine(const uint8_t *buf, size_t nBytes)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.drawLine(buf, nBytes);
  }

  void Ep128VM::Nick_::vsyncStateChange(bool newState,
                                        unsigned int currentSlot_)
  {
    if (vm.getIsDisplayEnabled())
      vm.display.vsyncStateChange(newState, currentSlot_);
  }

  // --------------------------------------------------------------------------

  void Ep128VM::stopDemoPlayback()
  {
    if (isPlayingDemo) {
      isPlayingDemo = false;
      demoTimeCnt = 0U;
      demoBuffer.clear();
      // clear keyboard state at end of playback
      for (int i = 0; i < 128; i++)
        dave.setKeyboardState(i, 0);
    }
  }

  void Ep128VM::stopDemoRecording(bool writeFile_)
  {
    isRecordingDemo = false;
    if (writeFile_ && demoFile != (Ep128Emu::File *) 0) {
      try {
        // put end of demo event
        writeDemoTimeCnt(demoBuffer, demoTimeCnt);
        demoTimeCnt = 0U;
        demoBuffer.writeByte(0x00);
        demoBuffer.writeByte(0x00);
        demoFile->addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_DEMO_STREAM,
                           demoBuffer);
      }
      catch (...) {
        demoFile = (Ep128Emu::File *) 0;
        demoTimeCnt = 0U;
        demoBuffer.clear();
        throw;
      }
      demoFile = (Ep128Emu::File *) 0;
      demoTimeCnt = 0U;
      demoBuffer.clear();
    }
  }

  void Ep128VM::updateTimingParameters()
  {
    stopDemoPlayback();         // changing configuration implies stopping
    stopDemoRecording(false);   // any demo playback or recording
    cpuCyclesPerNickCycle =
        (int64_t(cpuFrequency) << 32) / int64_t(nickFrequency);
    daveCyclesPerNickCycle =
        (int64_t(daveFrequency) << 32) / int64_t(nickFrequency);
    if (haveTape()) {
      tapeSamplesPerDaveCycle =
          (int64_t(getTapeSampleRate()) << 32) / int64_t(daveFrequency);
    }
    else
      tapeSamplesPerDaveCycle = 0;
    cpuCyclesRemaining = 0;
    cpuSyncToNickCnt = 0;
  }

  uint8_t Ep128VM::davePortReadCallback(void *userData, uint16_t addr)
  {
    return (reinterpret_cast<Ep128VM *>(userData)->dave.readPort(addr));
  }

  void Ep128VM::davePortWriteCallback(void *userData,
                                      uint16_t addr, uint8_t value)
  {
    reinterpret_cast<Ep128VM *>(userData)->dave.writePort(addr, value);
  }

  uint8_t Ep128VM::nickPortReadCallback(void *userData, uint16_t addr)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    if (vm.memoryTimingEnabled)
      vm.videoMemoryWait();
    return (vm.nick.readPort(addr));
  }

  void Ep128VM::nickPortWriteCallback(void *userData,
                                      uint16_t addr, uint8_t value)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    if (vm.memoryTimingEnabled)
      vm.videoMemoryWait();
    vm.nick.writePort(addr, value);
  }

  uint8_t Ep128VM::nickPortDebugReadCallback(void *userData, uint16_t addr)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    return (vm.nick.readPortDebug(addr));
  }

  uint8_t Ep128VM::exdosPortReadCallback(void *userData, uint16_t addr)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    // floppy emulation is disabled while recording or playing demo
    if (!(vm.isRecordingDemo || vm.isPlayingDemo)) {
      if (vm.currentFloppyDrive <= 3) {
        Ep128Emu::WD177x& floppyDrive = vm.floppyDrives[vm.currentFloppyDrive];
        switch (addr) {
        case 0x00:
          return floppyDrive.readStatusRegister();
        case 0x01:
          return floppyDrive.readTrackRegister();
        case 0x02:
          return floppyDrive.readSectorRegister();
        case 0x03:
          return floppyDrive.readDataRegister();
        case 0x08:
          return uint8_t(  (floppyDrive.getInterruptRequestFlag() ? 0x02 : 0x00)
                         | (floppyDrive.getDiskChangeFlag() ? 0x40 : 0x00)
                         | (floppyDrive.getDataRequestFlag() ? 0x80 : 0x00));
        }
      }
    }
    return 0x00;
  }

  void Ep128VM::exdosPortWriteCallback(void *userData,
                                       uint16_t addr, uint8_t value)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    // floppy emulation is disabled while recording or playing demo
    if (!(vm.isRecordingDemo || vm.isPlayingDemo)) {
      if (vm.currentFloppyDrive <= 3) {
        Ep128Emu::WD177x& floppyDrive = vm.floppyDrives[vm.currentFloppyDrive];
        switch (addr) {
        case 0x00:
          floppyDrive.writeCommandRegister(value);
          break;
        case 0x01:
          floppyDrive.writeTrackRegister(value);
          break;
        case 0x02:
          floppyDrive.writeSectorRegister(value);
          break;
        case 0x03:
          floppyDrive.writeDataRegister(value);
          break;
        }
      }
      if (addr == 0x08) {
        vm.currentFloppyDrive = 0xFF;
        if ((value & 0x01) != 0)
          vm.currentFloppyDrive = 0;
        else if ((value & 0x02) != 0)
          vm.currentFloppyDrive = 1;
        else if ((value & 0x04) != 0)
          vm.currentFloppyDrive = 2;
        else if ((value & 0x08) != 0)
          vm.currentFloppyDrive = 3;
        else
          return;
        if ((value & 0x40) != 0)
          vm.floppyDrives[vm.currentFloppyDrive].clearDiskChangeFlag();
        vm.floppyDrives[vm.currentFloppyDrive].setSide((value & 0x10) >> 4);
      }
    }
  }

  uint8_t Ep128VM::exdosPortDebugReadCallback(void *userData, uint16_t addr)
  {
    Ep128VM&  vm = *(reinterpret_cast<Ep128VM *>(userData));
    if (vm.currentFloppyDrive <= 3) {
      Ep128Emu::WD177x& floppyDrive = vm.floppyDrives[vm.currentFloppyDrive];
      switch (addr) {
      case 0x00:
        return floppyDrive.readStatusRegisterDebug();
      case 0x01:
        return floppyDrive.readTrackRegister();
      case 0x02:
        return floppyDrive.readSectorRegister();
      case 0x03:
        return floppyDrive.readDataRegisterDebug();
      case 0x08:
        return uint8_t(  (floppyDrive.getInterruptRequestFlag() ? 0x02 : 0x00)
                       | (floppyDrive.getDiskChangeFlag() ? 0x40 : 0x00)
                       | (floppyDrive.getDataRequestFlag() ? 0x80 : 0x00));
      }
    }
    return 0x00;
  }

  uint8_t Ep128VM::checkSingleStepModeBreak()
  {
    uint16_t  addr = z80.getReg().PC.W.l;
    uint8_t   b0 = memory.readNoDebug(addr);
    if (singleStepModeStepOverFlag) {
      if (singleStepModeNextAddr >= 0 &&
          int32_t(addr) != singleStepModeNextAddr)
        return b0;
    }
    int32_t   nxtAddr = int32_t(-1);
    switch (b0) {
    case 0x10:                                  // DJNZ
    case 0xF7:                                  // EXOS
      nxtAddr = (addr + 2) & 0xFFFF;
      break;
    case 0x76:                                  // HLT
    case 0xC7:                                  // RST
    case 0xCF:
    case 0xD7:
    case 0xDF:
    case 0xE7:
    case 0xEF:
    case 0xFF:
      nxtAddr = (addr + 1) & 0xFFFF;
      break;
    case 0xC4:                                  // CALL
    case 0xCC:
    case 0xCD:
    case 0xD4:
    case 0xDC:
    case 0xE4:
    case 0xEC:
    case 0xF4:
    case 0xFC:
      nxtAddr = (addr + 3) & 0xFFFF;
      break;
    case 0xED:
      {
        uint8_t   b1 = memory.readNoDebug((addr + 1) & 0xFFFF);
        switch (b1) {
        case 0xB0:                              // LDIR
        case 0xB1:                              // CPIR
        case 0xB2:                              // INIR
        case 0xB3:                              // OTIR
        case 0xB8:                              // LDDR
        case 0xB9:                              // CPDR
        case 0xBA:                              // INDR
        case 0xBB:                              // OTDR
          nxtAddr = (addr + 2) & 0xFFFF;
          break;
        }
      }
      break;
    }
    singleStepModeNextAddr = nxtAddr;
    if (!memory.checkIgnoreBreakPoint(addr))
      breakPointCallback(breakPointCallbackUserData, false, false, addr, b0);
    return b0;
  }

  // --------------------------------------------------------------------------

  Ep128VM::Ep128VM(Ep128Emu::VideoDisplay& display_,
                   Ep128Emu::AudioOutput& audioOutput_)
    : VirtualMachine(display_, audioOutput_),
      z80(*this),
      memory(*this),
      ioPorts(*this),
      dave(*this),
      nick(*this),
      cpuFrequency(4000000),
      daveFrequency(500000),
      nickFrequency(890625),
      nickCyclesRemaining(0),
      cpuCyclesPerNickCycle(0),
      cpuCyclesRemaining(0),
      cpuSyncToNickCnt(0),
      daveCyclesPerNickCycle(0),
      daveCyclesRemaining(0),
      memoryWaitMode(1),
      memoryTimingEnabled(true),
      singleStepModeEnabled(false),
      singleStepModeStepOverFlag(false),
      singleStepModeNextAddr(int32_t(-1)),
      tapeSamplesPerDaveCycle(0),
      tapeSamplesRemaining(0),
      isRemote1On(false),
      isRemote2On(false),
      demoFile((Ep128Emu::File *) 0),
      demoBuffer(),
      isRecordingDemo(false),
      isPlayingDemo(false),
      snapshotLoadFlag(false),
      demoTimeCnt(0U),
      currentFloppyDrive(0xFF),
      breakPointPriorityThreshold(0)
  {
    for (size_t i = 0; i < 4; i++)
      pageTable[i] = 0x00;
    updateTimingParameters();
    // register I/O callbacks
    ioPorts.setReadCallback(0xA0, 0xBF, &davePortReadCallback, this, 0xA0);
    ioPorts.setWriteCallback(0xA0, 0xBF, &davePortWriteCallback, this, 0xA0);
    ioPorts.setDebugReadCallback(0xB0, 0xB6, &davePortReadCallback, this, 0xA0);
    for (uint16_t i = 0x80; i <= 0x8F; i++) {
      ioPorts.setReadCallback(i, i, &nickPortReadCallback, this, (i & 0x8C));
      ioPorts.setWriteCallback(i, i, &nickPortWriteCallback, this, (i & 0x8C));
      ioPorts.setDebugReadCallback(i, i, &nickPortDebugReadCallback,
                                   this, (i & 0x8C));
    }
    // set up initial memory (segments 0xFC to 0xFF are always present)
    memory.loadSegment(0xFC, false, (uint8_t *) 0, 0);
    memory.loadSegment(0xFD, false, (uint8_t *) 0, 0);
    memory.loadSegment(0xFE, false, (uint8_t *) 0, 0);
    memory.loadSegment(0xFF, false, (uint8_t *) 0, 0);
    // hack to get some programs using interrupt mode 2 working
    z80.setVectorBase(0xFF);
    // reset
    z80.reset();
    for (uint16_t i = 0x80; i <= 0x83; i++)
      nick.writePort(i, 0x00);
    dave.reset();
    // use NICK colormap
    Ep128Emu::VideoDisplay::DisplayParameters
        dp(display.getDisplayParameters());
    dp.indexToRGBFunc = &Nick::convertPixelToRGB;
    display.setDisplayParameters(dp);
    setAudioConverterSampleRate(float(long(daveFrequency)));
    floppyDrives[0].setIsWD1773(false);
    floppyDrives[0].reset();
    floppyDrives[1].setIsWD1773(false);
    floppyDrives[1].reset();
    floppyDrives[2].setIsWD1773(false);
    floppyDrives[2].reset();
    floppyDrives[3].setIsWD1773(false);
    floppyDrives[3].reset();
    ioPorts.setReadCallback(0x10, 0x13, &exdosPortReadCallback, this, 0x10);
    ioPorts.setReadCallback(0x14, 0x17, &exdosPortReadCallback, this, 0x14);
    ioPorts.setReadCallback(0x18, 0x18, &exdosPortReadCallback, this, 0x10);
    ioPorts.setReadCallback(0x1C, 0x1C, &exdosPortReadCallback, this, 0x14);
    ioPorts.setWriteCallback(0x10, 0x13, &exdosPortWriteCallback, this, 0x10);
    ioPorts.setWriteCallback(0x14, 0x17, &exdosPortWriteCallback, this, 0x14);
    ioPorts.setWriteCallback(0x18, 0x18, &exdosPortWriteCallback, this, 0x10);
    ioPorts.setWriteCallback(0x1C, 0x1C, &exdosPortWriteCallback, this, 0x14);
    ioPorts.setDebugReadCallback(0x10, 0x13,
                                 &exdosPortDebugReadCallback, this, 0x10);
    ioPorts.setDebugReadCallback(0x14, 0x17,
                                 &exdosPortDebugReadCallback, this, 0x14);
    ioPorts.setDebugReadCallback(0x18, 0x18,
                                 &exdosPortDebugReadCallback, this, 0x10);
    ioPorts.setDebugReadCallback(0x1C, 0x1C,
                                 &exdosPortDebugReadCallback, this, 0x14);
  }

  Ep128VM::~Ep128VM()
  {
    try {
      // FIXME: cannot handle errors here
      stopDemo();
    }
    catch (...) {
    }
  }

  void Ep128VM::run(size_t microseconds)
  {
    Ep128Emu::VirtualMachine::run(microseconds);
    if (snapshotLoadFlag) {
      snapshotLoadFlag = false;
      // if just loaded a snapshot, and not playing a demo,
      // clear keyboard state
      if (!isPlayingDemo) {
        for (int i = 0; i < 128; i++)
          dave.setKeyboardState(i, 0);
      }
    }
    nickCyclesRemaining +=
        ((int64_t(microseconds) << 26) * int64_t(nickFrequency)
         / int64_t(15625));     // 10^6 / 2^6
    while (nickCyclesRemaining > 0) {
      if (isPlayingDemo) {
        while (!demoTimeCnt) {
          if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
            stopDemoPlayback();
          try {
            uint8_t evtType = demoBuffer.readByte();
            uint8_t evtBytes = demoBuffer.readByte();
            uint8_t evtData = 0;
            while (evtBytes) {
              evtData = demoBuffer.readByte();
              evtBytes--;
            }
            switch (evtType) {
            case 0x00:
              stopDemoPlayback();
              break;
            case 0x01:
              dave.setKeyboardState(evtData, 1);
              break;
            case 0x02:
              dave.setKeyboardState(evtData, 0);
              break;
            }
            demoTimeCnt = readDemoTimeCnt(demoBuffer);
          }
          catch (...) {
            stopDemoPlayback();
          }
          if (!isPlayingDemo) {
            demoBuffer.clear();
            demoTimeCnt = 0U;
            break;
          }
        }
        if (demoTimeCnt)
          demoTimeCnt--;
      }
      daveCyclesRemaining += daveCyclesPerNickCycle;
      while (daveCyclesRemaining > 0) {
        daveCyclesRemaining -= (int64_t(1) << 32);
        uint32_t  tmp = dave.runOneCycle();
        sendAudioOutput(tmp);
        if (haveTape()) {
          tapeSamplesRemaining += tapeSamplesPerDaveCycle;
          if (tapeSamplesRemaining > 0) {
            // assume tape sample rate < daveFrequency
            tapeSamplesRemaining -= (int64_t(1) << 32);
            int   daveTapeInput = runTape(int(tmp & 0xFFFFU));
            dave.setTapeInput(daveTapeInput, daveTapeInput);
          }
        }
      }
      cpuCyclesRemaining += cpuCyclesPerNickCycle;
      cpuSyncToNickCnt += cpuCyclesPerNickCycle;
      while (cpuCyclesRemaining > 0)
        z80.executeInstruction();
      nick.runOneSlot();
      nickCyclesRemaining -= (int64_t(1) << 32);
      if (isRecordingDemo)
        demoTimeCnt++;
    }
  }

  void Ep128VM::reset(bool isColdReset)
  {
    stopDemoPlayback();         // TODO: should be recorded as an event ?
    stopDemoRecording(false);
    z80.reset();
    dave.reset();
    isRemote1On = false;
    isRemote2On = false;
    setTapeMotorState(false);
    currentFloppyDrive = 0xFF;
    floppyDrives[0].reset();
    floppyDrives[1].reset();
    floppyDrives[2].reset();
    floppyDrives[3].reset();
    if (isColdReset) {
      for (int i = 0; i < 256; i++) {
        if (memory.isSegmentRAM(uint8_t(i)))
          memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
      }
    }
  }

  void Ep128VM::resetMemoryConfiguration(size_t memSize)
  {
    stopDemo();
    // calculate new number of RAM segments
    size_t  nSegments = (memSize + 15) >> 4;
    nSegments = (nSegments > 4 ? (nSegments < 232 ? nSegments : 232) : 4);
    // delete all ROM segments
    for (int i = 0; i < 256; i++) {
      if (memory.isSegmentROM(uint8_t(i)))
        memory.deleteSegment(uint8_t(i));
    }
    // resize RAM
    for (int i = 0; i <= (0xFF - int(nSegments)); i++) {
      if (memory.isSegmentRAM(uint8_t(i)))
        memory.deleteSegment(uint8_t(i));
    }
    for (int i = 0xFF; i > (0xFF - int(nSegments)); i--) {
      if (!memory.isSegmentRAM(uint8_t(i)))
        memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
    }
    // cold reset
    this->reset(true);
  }

  void Ep128VM::loadROMSegment(uint8_t n, const char *fileName, size_t offs)
  {
    stopDemo();
    if (fileName == (char *) 0 || fileName[0] == '\0') {
      // empty file name: delete segment
      if (memory.isSegmentROM(n))
        memory.deleteSegment(n);
      else if (memory.isSegmentRAM(n)) {
        memory.deleteSegment(n);
        // if there was RAM at the specified segment, relocate it
        for (int i = 0xFF; i >= 0x08; i--) {
          if (!(memory.isSegmentROM(uint8_t(i)) ||
                memory.isSegmentRAM(uint8_t(i)))) {
            memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
            break;
          }
        }
      }
      return;
    }
    // load file into memory
    std::vector<uint8_t>  buf;
    buf.resize(0x4000);
    std::FILE   *f = std::fopen(fileName, "rb");
    if (!f)
      throw Ep128Emu::Exception("cannot open ROM file");
    std::fseek(f, 0L, SEEK_END);
    if (ftell(f) < long(offs + 0x4000)) {
      std::fclose(f);
      throw Ep128Emu::Exception("ROM file is shorter than expected");
    }
    std::fseek(f, long(offs), SEEK_SET);
    std::fread(&(buf.front()), 1, 0x4000, f);
    std::fclose(f);
    if (memory.isSegmentRAM(n)) {
      memory.loadSegment(n, true, &(buf.front()), 0x4000);
      // if there was RAM at the specified segment, relocate it
      for (int i = 0xFF; i >= 0x08; i--) {
        if (!(memory.isSegmentROM(uint8_t(i)) ||
              memory.isSegmentRAM(uint8_t(i)))) {
          memory.loadSegment(uint8_t(i), false, (uint8_t *) 0, 0);
          break;
        }
      }
    }
    else {
      // otherwise just load new segment, or replace existing ROM
      memory.loadSegment(n, true, &(buf.front()), 0x4000);
    }
  }

  void Ep128VM::setCPUFrequency(size_t freq_)
  {
    // NOTE: Z80 frequency should always be greater than NICK frequency,
    // so that the memory timing functions in ep128vm.hpp work correctly
    size_t  freq = (freq_ > 2000000 ? (freq_ < 250000000 ? freq_ : 250000000)
                                      : 2000000);
    if (cpuFrequency != freq) {
      cpuFrequency = freq;
      updateTimingParameters();
    }
  }

  void Ep128VM::setVideoFrequency(size_t freq_)
  {
    size_t  freq;
    // allow refresh rates in the range 10 Hz to 100 Hz
    freq = (freq_ > 178125 ? (freq_ < 1781250 ? freq_ : 1781250) : 178125);
    if (nickFrequency != freq) {
      nickFrequency = freq;
      updateTimingParameters();
    }
  }

  void Ep128VM::setSoundClockFrequency(size_t freq_)
  {
    size_t  freq;
    freq = (freq_ > 250000 ? (freq_ < 1000000 ? freq_ : 1000000) : 250000);
    if (daveFrequency != freq) {
      daveFrequency = freq;
      updateTimingParameters();
      setAudioConverterSampleRate(float(long(daveFrequency)));
    }
  }

  void Ep128VM::setEnableMemoryTimingEmulation(bool isEnabled)
  {
    if (memoryTimingEnabled != isEnabled) {
      stopDemoPlayback();       // changing configuration implies stopping
      stopDemoRecording(false); // any demo playback or recording
      memoryTimingEnabled = isEnabled;
      cpuCyclesRemaining = 0;
      cpuSyncToNickCnt = 0;
    }
  }

  void Ep128VM::setKeyboardState(int keyCode, bool isPressed)
  {
    if (!isPlayingDemo)
      dave.setKeyboardState(keyCode, (isPressed ? 1 : 0));
    if (isRecordingDemo) {
      if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0) {
        stopDemoRecording(false);
        return;
      }
      writeDemoTimeCnt(demoBuffer, demoTimeCnt);
      demoTimeCnt = 0U;
      demoBuffer.writeByte(isPressed ? 0x01 : 0x02);
      demoBuffer.writeByte(0x01);
      demoBuffer.writeByte(uint8_t(keyCode & 0x7F));
    }
  }

  void Ep128VM::setDiskImageFile(int n, const std::string& fileName_,
                                 int nTracks_, int nSides_,
                                 int nSectorsPerTrack_)
  {
    if (n < 0 || n > 3)
      throw Ep128Emu::Exception("invalid floppy drive number");
    floppyDrives[n].setDiskImageFile(fileName_,
                                     nTracks_, nSides_, nSectorsPerTrack_);
  }

  void Ep128VM::setTapeFileName(const std::string& fileName)
  {
    Ep128Emu::VirtualMachine::setTapeFileName(fileName);
    setTapeMotorState(isRemote1On || isRemote2On);
    if (haveTape()) {
      tapeSamplesPerDaveCycle =
          (int64_t(getTapeSampleRate()) << 32) / int64_t(daveFrequency);
    }
    tapeSamplesRemaining = 0;
  }

  void Ep128VM::tapePlay()
  {
    Ep128Emu::VirtualMachine::tapePlay();
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void Ep128VM::tapeRecord()
  {
    Ep128Emu::VirtualMachine::tapeRecord();
    if (haveTape() && getIsTapeMotorOn() && getTapeButtonState() != 0)
      stopDemo();
  }

  void Ep128VM::setBreakPoints(const Ep128Emu::BreakPointList& bpList)
  {
    for (size_t i = 0; i < bpList.getBreakPointCnt(); i++) {
      const Ep128Emu::BreakPoint& bp = bpList.getBreakPoint(i);
      if (bp.isIO())
        ioPorts.setBreakPoint(bp.addr(),
                              bp.priority(), bp.isRead(), bp.isWrite());
      else if (bp.haveSegment())
        memory.setBreakPoint(bp.segment(), bp.addr(),
                             bp.priority(), bp.isRead(), bp.isWrite(),
                             bp.isIgnore());
      else
        memory.setBreakPoint(bp.addr(),
                             bp.priority(), bp.isRead(), bp.isWrite(),
                             bp.isIgnore());
    }
  }

  Ep128Emu::BreakPointList Ep128VM::getBreakPoints()
  {
    Ep128Emu::BreakPointList  bpl1(ioPorts.getBreakPointList());
    Ep128Emu::BreakPointList  bpl2(memory.getBreakPointList());
    for (size_t i = 0; i < bpl1.getBreakPointCnt(); i++) {
      const Ep128Emu::BreakPoint& bp = bpl1.getBreakPoint(i);
      bpl2.addIOBreakPoint(bp.addr(), bp.isRead(), bp.isWrite(), bp.priority());
    }
    return bpl2;
  }

  void Ep128VM::clearBreakPoints()
  {
    memory.clearAllBreakPoints();
    ioPorts.clearBreakPoints();
  }

  void Ep128VM::setBreakPointPriorityThreshold(int n)
  {
    breakPointPriorityThreshold = uint8_t(n > 0 ? (n < 4 ? n : 4) : 0);
    if (!singleStepModeEnabled) {
      memory.setBreakPointPriorityThreshold(n);
      ioPorts.setBreakPointPriorityThreshold(n);
    }
  }

  void Ep128VM::setSingleStepMode(bool isEnabled, bool stepOverFlag)
  {
    singleStepModeEnabled = isEnabled;
    singleStepModeStepOverFlag = stepOverFlag;
    int   n = 4;
    if (!isEnabled) {
      n = int(breakPointPriorityThreshold);
      singleStepModeNextAddr = int32_t(-1);
    }
    memory.setBreakPointPriorityThreshold(n);
    ioPorts.setBreakPointPriorityThreshold(n);
  }

  uint8_t Ep128VM::getMemoryPage(int n) const
  {
    return memory.getPage(uint8_t(n & 3));
  }

  uint8_t Ep128VM::readMemory(uint32_t addr, bool isCPUAddress) const
  {
    if (isCPUAddress)
      addr = ((uint32_t(memory.getPage(uint8_t(addr >> 14) & uint8_t(3))) << 14)
              | (addr & uint32_t(0x3FFF)));
    else
      addr &= uint32_t(0x003FFFFF);
    return memory.readRaw(addr);
  }

  void Ep128VM::writeMemory(uint32_t addr, uint8_t value, bool isCPUAddress)
  {
    if (isRecordingDemo || isPlayingDemo) {
      stopDemoPlayback();
      stopDemoRecording(false);
    }
    if (isCPUAddress)
      addr = ((uint32_t(memory.getPage(uint8_t(addr >> 14) & uint8_t(3))) << 14)
              | (addr & uint32_t(0x3FFF)));
    else
      addr &= uint32_t(0x003FFFFF);
    memory.writeRaw(addr, value);
  }

  uint16_t Ep128VM::getProgramCounter() const
  {
    return uint16_t(z80.getReg().PC.W.l);
  }

  uint16_t Ep128VM::getStackPointer() const
  {
    return uint16_t(z80.getReg().SP.W);
  }

  void Ep128VM::listCPURegisters(std::string& buf) const
  {
    char    tmpBuf[192];
    const Z80_REGISTERS&  r = z80.getReg();
    std::sprintf(&(tmpBuf[0]),
                 " PC   AF   BC   DE   HL   SP   IX   IY\n"
                 "%04X %04X %04X %04X %04X %04X %04X %04X\n"
                 "      AF'  BC'  DE'  HL'  IM   I    R\n"
                 "     %04X %04X %04X %04X  %02X   %02X   %02X",
                 (unsigned int) r.PC.W.l, (unsigned int) r.AF.W,
                 (unsigned int) r.BC.W, (unsigned int) r.DE.W,
                 (unsigned int) r.HL.W, (unsigned int) r.SP.W,
                 (unsigned int) r.IX.W, (unsigned int) r.IY.W,
                 (unsigned int) r.altAF.W, (unsigned int) r.altBC.W,
                 (unsigned int) r.altDE.W, (unsigned int) r.altHL.W,
                 (unsigned int) r.IM, (unsigned int) r.I, (unsigned int) r.R);
    buf = &(tmpBuf[0]);
  }

  void Ep128VM::listIORegisters(std::string& buf) const
  {
    char    tmpBuf[192];
    char    *bufp = &(tmpBuf[0]);
    int     n = std::sprintf(bufp, "DAVE (A0)\n\n");
    bufp = bufp + n;
    for (uint16_t i = 0x00A0; i <= 0x00BF; i++) {
      if (!(i & 3)) {
        n = std::sprintf(bufp, "  %02X", (unsigned int) ioPorts.readDebug(i));
        bufp = bufp + n;
      }
      else {
        n = std::sprintf(bufp, " %02X", (unsigned int) ioPorts.readDebug(i));
        bufp = bufp + n;
      }
      if ((i & 15) == 15)
        *(bufp++) = '\n';
    }
    n = std::sprintf(bufp, "\nNICK (80)\n\n ");
    bufp = bufp + n;
    for (uint16_t i = 0x0080; i <= 0x0083; i++) {
      n = std::sprintf(bufp, " %02X", (unsigned int) ioPorts.readDebug(i));
      bufp = bufp + n;
    }
    n = std::sprintf(bufp, "\n\nWD177x (10)\n\n ");
    bufp = bufp + n;
    for (uint16_t i = 0x0010; i <= 0x0013; i++) {
      n = std::sprintf(bufp, " %02X", (unsigned int) ioPorts.readDebug(i));
      bufp = bufp + n;
    }
    std::sprintf(bufp, "\n\nEXDOS (18)\n\n  %02X",
                 (unsigned int) ioPorts.readDebug(0x0018));
    buf = &(tmpBuf[0]);
  }

  uint32_t Ep128VM::disassembleInstruction(std::string& buf,
                                           uint32_t addr, bool isCPUAddress,
                                           int32_t offs) const
  {
    return Z80Disassembler::disassembleInstruction(buf, (*this),
                                                   addr, isCPUAddress, offs);
  }

  const Z80_REGISTERS& Ep128VM::getZ80Registers() const
  {
    return z80.getReg();
  }

  void Ep128VM::saveState(Ep128Emu::File& f)
  {
    ioPorts.saveState(f);
    memory.saveState(f);
    nick.saveState(f);
    dave.saveState(f);
    z80.saveState(f);
    {
      Ep128Emu::File::Buffer  buf;
      buf.setPosition(0);
      buf.writeUInt32(0x01000000);      // version number
      buf.writeByte(memory.getPage(0));
      buf.writeByte(memory.getPage(1));
      buf.writeByte(memory.getPage(2));
      buf.writeByte(memory.getPage(3));
      buf.writeByte(memoryWaitMode & 3);
      buf.writeUInt32(uint32_t(cpuFrequency));
      buf.writeUInt32(uint32_t(daveFrequency));
      buf.writeUInt32(uint32_t(nickFrequency));
      buf.writeUInt32(62U);     // video memory latency for compatibility only
      buf.writeBoolean(memoryTimingEnabled);
      int64_t tmp[3];
      tmp[0] = cpuCyclesRemaining;
      tmp[1] = cpuSyncToNickCnt;
      tmp[2] = daveCyclesRemaining;
      for (size_t i = 0; i < 3; i++) {
        buf.writeUInt32(uint32_t(uint64_t(tmp[i]) >> 32)
                        & uint32_t(0xFFFFFFFFUL));
        buf.writeUInt32(uint32_t(uint64_t(tmp[i]))
                        & uint32_t(0xFFFFFFFFUL));
      }
      f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_VM_STATE, buf);
    }
  }

  void Ep128VM::saveMachineConfiguration(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
    buf.writeUInt32(uint32_t(cpuFrequency));
    buf.writeUInt32(uint32_t(daveFrequency));
    buf.writeUInt32(uint32_t(nickFrequency));
    buf.writeUInt32(62U);       // video memory latency for compatibility only
    buf.writeBoolean(memoryTimingEnabled);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_VM_CONFIG, buf);
  }

  void Ep128VM::recordDemo(Ep128Emu::File& f)
  {
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    isRemote1On = false;
    isRemote2On = false;
    setTapeMotorState(false);
    dave.setTapeInput(0, 0);
    stopDemo();
    for (int i = 0; i < 128; i++)
      dave.setKeyboardState(i, 0);
    // floppy emulation is disabled while recording or playing demo
    currentFloppyDrive = 0xFF;
    floppyDrives[0].reset();
    floppyDrives[1].reset();
    floppyDrives[2].reset();
    floppyDrives[3].reset();
    z80.closeAllFiles();
    // save full snapshot, including timing and clock frequency settings
    saveMachineConfiguration(f);
    saveState(f);
    demoBuffer.clear();
    demoBuffer.writeUInt32(0x00020001); // version 2.0.1
    demoFile = &f;
    isRecordingDemo = true;
    demoTimeCnt = 0U;
  }

  void Ep128VM::stopDemo()
  {
    stopDemoPlayback();
    stopDemoRecording(true);
  }

  bool Ep128VM::getIsRecordingDemo()
  {
    if (demoFile != (Ep128Emu::File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    return isRecordingDemo;
  }

  bool Ep128VM::getIsPlayingDemo() const
  {
    return isPlayingDemo;
  }

  // --------------------------------------------------------------------------

  void Ep128VM::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible ep128 snapshot format");
    }
    isRemote1On = false;
    isRemote2On = false;
    setTapeMotorState(false);
    stopDemo();
    snapshotLoadFlag = true;
    // reset floppy emulation, as its state is not saved
    currentFloppyDrive = 0xFF;
    floppyDrives[0].reset();
    floppyDrives[1].reset();
    floppyDrives[2].reset();
    floppyDrives[3].reset();
    z80.closeAllFiles();
    try {
      uint8_t   p0, p1, p2, p3;
      p0 = buf.readByte();
      p1 = buf.readByte();
      p2 = buf.readByte();
      p3 = buf.readByte();
      pageTable[0] = p0;
      memory.setPage(0, p0);
      pageTable[1] = p1;
      memory.setPage(1, p1);
      pageTable[2] = p2;
      memory.setPage(2, p2);
      pageTable[3] = p3;
      memory.setPage(3, p3);
      memoryWaitMode = buf.readByte() & 3;
      uint32_t  tmpCPUFrequency = buf.readUInt32();
      uint32_t  tmpDaveFrequency = buf.readUInt32();
      uint32_t  tmpNickFrequency = buf.readUInt32();
      uint32_t  tmpVideoMemoryLatency = buf.readUInt32();
      (void) tmpVideoMemoryLatency;
      bool      tmpMemoryTimingEnabled = buf.readBoolean();
      int64_t   tmp[3];
      for (size_t i = 0; i < 3; i++) {
        uint64_t  n = uint64_t(buf.readUInt32()) << 32;
        n |= uint64_t(buf.readUInt32());
        tmp[i] = int64_t(n);
      }
      if (tmpCPUFrequency == cpuFrequency &&
          tmpDaveFrequency == daveFrequency &&
          tmpNickFrequency == nickFrequency &&
          tmpMemoryTimingEnabled == memoryTimingEnabled) {
        cpuCyclesRemaining = tmp[0];
        cpuSyncToNickCnt = tmp[1];
        daveCyclesRemaining = tmp[2];
      }
      else {
        cpuCyclesRemaining = 0;
        cpuSyncToNickCnt = 0;
        daveCyclesRemaining = 0;
      }
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "ep128 snapshot data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void Ep128VM::loadMachineConfiguration(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible ep128 "
                                "machine configuration format");
    }
    try {
      setCPUFrequency(buf.readUInt32());
      setSoundClockFrequency(buf.readUInt32());
      setVideoFrequency(buf.readUInt32());
      uint32_t  tmpVideoMemoryLatency = buf.readUInt32();
      (void) tmpVideoMemoryLatency;
      setEnableMemoryTimingEmulation(buf.readBoolean());
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "ep128 machine configuration data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void Ep128VM::loadDemo(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
#if 0
    if (version != 0x00020001) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible ep128 demo format");
    }
#endif
    (void) version;
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    isRemote1On = false;
    isRemote2On = false;
    setTapeMotorState(false);
    dave.setTapeInput(0, 0);
    stopDemo();
    for (int i = 0; i < 128; i++)
      dave.setKeyboardState(i, 0);
    // floppy emulation is disabled while recording or playing demo
    currentFloppyDrive = 0xFF;
    floppyDrives[0].reset();
    floppyDrives[1].reset();
    floppyDrives[2].reset();
    floppyDrives[3].reset();
    // initialize time counter with first delta time
    demoTimeCnt = readDemoTimeCnt(buf);
    isPlayingDemo = true;
    // copy any remaining demo data to local buffer
    demoBuffer.clear();
    demoBuffer.writeData(buf.getData() + buf.getPosition(),
                         buf.getDataSize() - buf.getPosition());
    demoBuffer.setPosition(0);
  }

  class ChunkType_Ep128VMConfig : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Ep128VM&  ref;
   public:
    ChunkType_Ep128VMConfig(Ep128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Ep128VMConfig()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_VM_CONFIG;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadMachineConfiguration(buf);
    }
  };

  class ChunkType_Ep128VMSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Ep128VM&  ref;
   public:
    ChunkType_Ep128VMSnapshot(Ep128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Ep128VMSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_VM_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  class ChunkType_DemoStream : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Ep128VM&  ref;
   public:
    ChunkType_DemoStream(Ep128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_DemoStream()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_DEMO_STREAM;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadDemo(buf);
    }
  };

  void Ep128VM::registerChunkTypes(Ep128Emu::File& f)
  {
    ChunkType_Ep128VMConfig   *p1 = (ChunkType_Ep128VMConfig *) 0;
    ChunkType_Ep128VMSnapshot *p2 = (ChunkType_Ep128VMSnapshot *) 0;
    ChunkType_DemoStream      *p3 = (ChunkType_DemoStream *) 0;

    try {
      p1 = new ChunkType_Ep128VMConfig(*this);
      f.registerChunkType(p1);
      p1 = (ChunkType_Ep128VMConfig *) 0;
      p2 = new ChunkType_Ep128VMSnapshot(*this);
      f.registerChunkType(p2);
      p2 = (ChunkType_Ep128VMSnapshot *) 0;
      p3 = new ChunkType_DemoStream(*this);
      f.registerChunkType(p3);
      p3 = (ChunkType_DemoStream *) 0;
    }
    catch (...) {
      if (p1)
        delete p1;
      if (p2)
        delete p2;
      if (p3)
        delete p3;
      throw;
    }
    ioPorts.registerChunkType(f);
    z80.registerChunkType(f);
    memory.registerChunkType(f);
    dave.registerChunkType(f);
    nick.registerChunkType(f);
  }

}       // namespace Ep128

