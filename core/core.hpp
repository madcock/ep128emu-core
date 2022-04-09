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

namespace Ep128Emu
{

enum LibretroCore_VM_config
{
  EP128_DISK,
  EP128_TAPE,
  EP128_FILE,
  EP64_DISK,
  EP64_TAPE,
  EP64_FILE,
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
