#ifndef EP128EMU_CORE_HPP
#define EP128EMU_CORE_HPP
#include "ep128vm.hpp"
#include "tvc64vm.hpp"
#include "cpc464vm.hpp"
#include "zx128vm.hpp"
#include "vmthread.hpp"
#include "emucfg.hpp"
#include "libretro.h"
#include "libretro-funcs.hpp"
#include "libretrodisp.hpp"
#include "libretrosnd.hpp"

namespace Ep128Emu
{

enum LibretroCore_VM_config
{
  EP128_DISK,
  EP128_TAPE,
  EP128_FILE,
  EP128_FILE_DTF,
  EP64_DISK,
  EP64_TAPE,
  EP64_FILE,
  EP64_FILE_DTF,
  TVC64_FILE,
  TVC64_DISK,
  CPC_TAPE,
  CPC_DISK,
  ZX16_TAPE,
  ZX16_FILE,
  ZX48_TAPE,
  ZX48_FILE,
  ZX128_TAPE,
  ZX128_FILE,
  VM_CONFIG_UNKNOWN = INT_MAX
};

enum LibretroCore_VM_type
{
  MACHINE_EP,
  MACHINE_TVC,
  MACHINE_CPC,
  MACHINE_ZX,
  MACHINE_UNKNOWN = INT_MAX
};

const std::map<std::string, std::string> rom_names_ep128emu_tosec = {

{"basic21.rom" , "BASIC Interpreter v2.1 (1985)(Intelligent Software).bin"},
{"exos20.rom"  , "Expandible OS v2.0 (1984)(Intelligent Software).bin"},
{"exos21.rom"  , "Expandible OS v2.1 (1985)(Intelligent Software).bin"},
{"exos23.rom"  , "Expandible OS v2.3 (1987)(Intelligent Software).bin"},
{"exdos13.rom" , "Disk Controller v1.3 (1985)(Intelligent Software).bin"}, // binary does not fully match
{"zt19uk.rom"  , "ZozoTools v1.8 (19xx)(Zoltan Nemeth).bin"}, // version does not match
{"zx48.rom"    , "ZX Spectrum 48K - 1 (1982)(Sinclair Research).rom"},
{"zx128.rom"   , "ZX Spectrum 128K (1986)(Sinclair Research)(128K)[aka Derby].rom"},
{"cpc_amsdos.rom", "Amstrad CPC 664 Amsdos (1985)(Amstrad)[AMSDOS.ROM].rom"},
{"cpc464.rom"  ,"Amstrad CPC 464 OS (19xx)(Amstrad)(da)[h][CPC464DK.ROM].rom"}, // binary does not fully match
//{"cpc464.rom"  ,"Amstrad CPC 464 OS (1985)(Amstrad)[OS.ROM].rom"}, // only first half
//{"cpc664.rom"  ,"Amstrad CPC 664 OS (1985)(Amstrad)[OS.ROM].rom"}, // only first half
//{"cpc6128.rom" ,"Amstrad CPC 6128 OS (1985)(Amstrad)[OS.ROM].rom"}, // only first half

};



class LibretroCore
{
private:
  retro_log_printf_t log_cb;
  int libretro_to_ep128emu_kbmap[RETROK_LAST];
  unsigned int bootframes[64];

public:
  uint16_t audioBuffer[EP128EMU_SAMPLE_RATE*1000*2];
  int inputJoyMap[256][4];
  bool inputStateMap[256][4];
  bool useHalfFrame;
  bool isHalfFrame;
  bool showOverscan;
  bool canSkipFrames;
  bool joypadConfigChanged;
  uint32_t prevFrameCount;
  size_t startSequenceIndex;
  int currWidth;
  int currHeight;
  int fullHeight;
  int halfHeight;
  int defaultHalfHeight;
  int defaultWidth;
  int borderSize;
  int machineType;
  int machineDetailedType;
  retro_usec_t totalTime;
  std::string startSequence;
  std::string infoMessage;

  Ep128Emu::VMThread              *vmThread    ;
  Ep128Emu::LibretroDisplay       *w           ;
  Ep128Emu::EmulatorConfiguration *config      ;
  Ep128Emu::VirtualMachine        *vm          ;
  Ep128Emu::AudioOutput           *audioOutput ;

  // ----------------

  LibretroCore(retro_log_printf_t log_cb_, int machineDetailedType, bool showOverscan_, bool canSkipFrames_, const char* romDirectory_, const char* saveDirectory_,
  const char* startSequence_, const char* cfgFile, bool useHalfFrame, bool enhancedRom);
  virtual ~LibretroCore();



  void initialize_keyboard_map(void);
  void update_keyboard(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers);

  void start(void);
  void run_for(retro_usec_t frameTime, float waitPeriod, void * fb);
  void sync_display();
  char* get_current_message(void);
  void update_input(retro_input_state_t input_state_cb, retro_environment_t environ_cb);
  void render(retro_video_refresh_t video_cb, retro_environment_t environ_cb);
  void change_resolution(int width, int height, retro_environment_t environ_cb);
  void errorCallback(void *userData, const char *msg);
};
}

#endif
