#include "core.hpp"
#include "libretro_keys_reverse.h"
namespace Ep128Emu
{

LibretroCore::LibretroCore(int machineDetailedType_, bool showOverscan_, bool canSkipFrames_, const char* romDirectory_, const char* saveDirectory_,
                           const char* startSequence_, const char* cfgFile)
  : useHalfFrame(true),
    isHalfFrame(true),
    showOverscan(showOverscan_),
    canSkipFrames(canSkipFrames_),
    joypadConfigChanged(false),
    prevFrameCount(0),
    startSequenceIndex(0),
    currWidth(EP128EMU_LIBRETRO_SCREEN_WIDTH),
    fullHeight(EP128EMU_LIBRETRO_SCREEN_HEIGHT),
    halfHeight(EP128EMU_LIBRETRO_SCREEN_HEIGHT/2),
    defaultHalfHeight(EP128EMU_LIBRETRO_SCREEN_HEIGHT/2),
    lineOffset(0),
    machineType(MACHINE_EP),
    machineDetailedType(machineDetailedType_),
    totalTime(0),
    vmThread(NULL),
    config(NULL)
{
  if(machineDetailedType == TVC64_DISK || machineDetailedType == TVC64_FILE)
  {
    machineType = MACHINE_TVC;
  }
  else if(machineDetailedType == CPC_DISK || machineDetailedType == CPC_TAPE)
  {
    machineType = MACHINE_CPC;
  }
  else if(machineDetailedType == ZX16_TAPE || machineDetailedType == ZX16_FILE ||
          machineDetailedType == ZX48_TAPE || machineDetailedType == ZX48_FILE ||
          machineDetailedType == ZX128_TAPE || machineDetailedType == ZX128_FILE
         )
  {
    machineType = MACHINE_ZX;
  }

  audioOutput = new Ep128Emu::AudioOutput_libretro();
  w = new Ep128Emu::LibretroDisplay(32, 32, EP128EMU_LIBRETRO_SCREEN_WIDTH, EP128EMU_LIBRETRO_SCREEN_HEIGHT, "", useHalfFrame);
  if(machineType == MACHINE_TVC)
  {
    vm = new TVC64::TVC64VM(*(dynamic_cast<Ep128Emu::VideoDisplay *>(w)),
                            *audioOutput);
  }
  else if (machineType == MACHINE_CPC)
  {
    vm = new CPC464::CPC464VM(*(dynamic_cast<Ep128Emu::VideoDisplay *>(w)),
                              *audioOutput);
  }
  else if (machineType == MACHINE_ZX)
  {
    vm = new ZX128::ZX128VM(*(dynamic_cast<Ep128Emu::VideoDisplay *>(w)),
                            *audioOutput);
  }
  else
  {
    vm = new Ep128::Ep128VM(*(dynamic_cast<Ep128Emu::VideoDisplay *>(w)),
                            *audioOutput);
  }

  config = new Ep128Emu::EmulatorConfiguration(
    *vm, *(dynamic_cast<Ep128Emu::VideoDisplay *>(w)), *audioOutput
#ifdef ENABLE_MIDI_PORT
    , *midiPort
#endif
  );
  //config->setErrorCallback(&LibretroCore::errorCallback, (void *) 0);
  currHeight = useHalfFrame ? EP128EMU_LIBRETRO_SCREEN_HEIGHT / 2 : EP128EMU_LIBRETRO_SCREEN_HEIGHT;
  if(!showOverscan)
  {
    fullHeight -= 40;
    halfHeight -= 20;
    defaultHalfHeight = halfHeight;
    currHeight = useHalfFrame ? halfHeight : fullHeight;
    // lineOffset is irrespective of half frame or not
    lineOffset = currWidth*20;
  }
  std::string romBasePath(romDirectory_);
  romBasePath.append("/ep128emu/roms/");
  std::string contentBasePath(romDirectory_);
  contentBasePath.append("/ep128emu/content/");
  startSequence = startSequence_;
  infoMessage = "Key map: X->space Y->enter L->0 R->1 L2->2 L3->3 ";

  config->memory.configFile = "";
  if(machineType == MACHINE_EP)
  {
    bootframes[machineDetailedType] = 40*10;
    config->memory.ram.size=128;
    config->memory.rom[0x00].file=romBasePath+"exos21.rom";
    config->memory.rom[0x00].offset=0;
    config->memory.rom[0x01].file=romBasePath+"exos21.rom";
    config->memory.rom[0x01].offset=16384;
    config->memory.rom[0x04].file=romBasePath+"basic21.rom";
    config->memory.rom[0x04].offset=0;
    if(machineDetailedType == EP128_FILE)
    {
      config->memory.rom[0x10].file=romBasePath+"epfileio.rom";
      config->memory.rom[0x10].offset=0;
    }
    if(machineDetailedType == EP128_DISK || machineDetailedType == EP128_FILE)
    {
      config->memory.rom[0x20].file=romBasePath+"exdos13.rom";
      config->memory.rom[0x20].offset=0;
      config->memory.rom[0x21].file=romBasePath+"exdos13.rom";
      config->memory.rom[0x21].offset=16384;
    }
  }
  else if(machineType == MACHINE_TVC)
  {
    bootframes[machineDetailedType] = 50*10;
    config->memory.ram.size=128;
    config->memory.rom[0x00].file=romBasePath+"tvc22_sys.rom";
    config->memory.rom[0x00].offset=0;
    config->memory.rom[0x02].file=romBasePath+"tvc22_ext.rom";
    config->memory.rom[0x02].offset=0;
    if(machineDetailedType == TVC64_FILE)
    {
      config->memory.rom[0x04].file=romBasePath+"tvcfileio.rom";
      config->memory.rom[0x04].offset=0;
    }
    if(machineDetailedType == TVC64_DISK)
    {
      config->memory.rom[0x03].file=romBasePath+"tvc_dos12d.rom";
      config->memory.rom[0x03].offset=0;
    }
  }
  else if(machineType == MACHINE_CPC)
  {
    bootframes[machineDetailedType] = 20*10;
    config->memory.ram.size=128;
    config->memory.rom[0x00].file=romBasePath+"cpc6128.rom";
    config->memory.rom[0x00].offset=16384;
    config->memory.rom[0x10].file=romBasePath+"cpc6128.rom";
    config->memory.rom[0x10].offset=0;
    if(machineDetailedType == CPC_DISK)
    {
      config->memory.rom[0x07].file=romBasePath+"cpc_amsdos.rom";
      config->memory.rom[0x07].offset=0;
    }
  }
  else if(machineType == MACHINE_ZX)
  {
    bootframes[machineDetailedType] = 20*10;
    if(machineDetailedType == ZX16_TAPE || machineDetailedType == ZX16_FILE)
    {
      config->memory.ram.size=16;
    }
    else if (machineDetailedType == ZX48_TAPE || machineDetailedType == ZX48_FILE)
    {
      config->memory.ram.size=48;
    }
    else
    {
      config->memory.ram.size=128;
    }

    if(machineDetailedType == ZX128_TAPE || machineDetailedType == ZX128_FILE)
    {
      config->memory.rom[0x00].file=romBasePath+"zx128.rom";
      config->memory.rom[0x00].offset=0;
      config->memory.rom[0x01].file=romBasePath+"zx128.rom";
      config->memory.rom[0x01].offset=16384;
    }
    else
    {
      config->memory.rom[0x00].file=romBasePath+"zx48.rom";
      config->memory.rom[0x00].offset=0;
    }
  }

  config->memoryConfigurationChanged = true;

  if(machineType == MACHINE_EP)
  {
    config->vm.cpuClockFrequency=4000000;
    config->vm.enableMemoryTimingEmulation=true;
    config->vm.soundClockFrequency=500000;
    config->vm.videoClockFrequency=889846;
  }
  else if(machineType == MACHINE_TVC)
  {
    config->vm.cpuClockFrequency=3125000;
    config->vm.enableMemoryTimingEmulation=true;
    config->vm.soundClockFrequency=390625;
    config->vm.videoClockFrequency=1562500;
  }
  else if(machineType == MACHINE_CPC)
  {
    config->vm.cpuClockFrequency=4000000;
    config->vm.enableMemoryTimingEmulation=true;
    config->vm.soundClockFrequency=125000;
    config->vm.videoClockFrequency=1000000;
  }
  else if(machineType == MACHINE_ZX)
  {
    if(machineDetailedType == ZX128_TAPE || machineDetailedType == ZX128_FILE)
    {
      config->vm.cpuClockFrequency=3546896;
      config->vm.soundClockFrequency=221681;
      config->vm.videoClockFrequency=886724;
    }
    else
    {
      config->vm.cpuClockFrequency=3500000;
      config->vm.soundClockFrequency=218750;
      config->vm.videoClockFrequency=875000;
    }
    config->vm.enableMemoryTimingEmulation=true;
  }
  config->vmConfigurationChanged = true;

  config->sound.sampleRate = EP128EMU_SAMPLE_RATE_FLOAT;
  config->sound.hwPeriods = 16;
  //config->sound.swPeriods = 16;
  //config->sound.enabled = false;
  config->sound.highQuality = true;
  config->soundSettingsChanged = true;

  if (cfgFile[0])
  {
    config->loadState(cfgFile,false);
  }
  initialize_keyboard_map();
  if (config->contentFileName != "")
  {
    startSequence = startSequence + config->contentFileName + "\xfe\r";
  }

  //config->display.enabled = false;
  //config->displaySettingsChanged = true;
  config->applySettings();

  vmThread = new Ep128Emu::VMThread(*vm);
}

LibretroCore::~LibretroCore()
{
  if (vmThread)
    delete vmThread;
  if (vm)
    delete vm;
  if (w)
    delete w;
  if (config)
    delete config;
  if (audioOutput)
    delete audioOutput;

}


void LibretroCore::initialize_keyboard_map(void)
{
  for(int i=0; i<RETROK_LAST; i++)
  {
    libretro_to_ep128emu_kbmap[i]=-1;
  }
  for(int port=0; port<4; port++)
  {
    for(int i=0; i<256; i++)
    {
      inputJoyMap[i][port] = -1;
    }
  }

  libretro_to_ep128emu_kbmap[RETROK_n]         = 0x00;
  libretro_to_ep128emu_kbmap[RETROK_BACKSLASH] = 0x01; // ERASE
  libretro_to_ep128emu_kbmap[RETROK_b]         = 0x02;
  libretro_to_ep128emu_kbmap[RETROK_c]         = 0x03;
  libretro_to_ep128emu_kbmap[RETROK_v]         = 0x04;
  libretro_to_ep128emu_kbmap[RETROK_x]         = 0x05;
  libretro_to_ep128emu_kbmap[RETROK_z]         = 0x06;
  libretro_to_ep128emu_kbmap[RETROK_LSHIFT]    = 0x07;
  libretro_to_ep128emu_kbmap[RETROK_h]         = 0x08;
  libretro_to_ep128emu_kbmap[RETROK_CAPSLOCK]  = 0x09; // LOCK
  libretro_to_ep128emu_kbmap[RETROK_g]         = 0x0A;
  libretro_to_ep128emu_kbmap[RETROK_d]         = 0x0B;
  libretro_to_ep128emu_kbmap[RETROK_f]         = 0x0C;
  libretro_to_ep128emu_kbmap[RETROK_s]         = 0x0D;
  libretro_to_ep128emu_kbmap[RETROK_a]         = 0x0E;
  libretro_to_ep128emu_kbmap[RETROK_LCTRL]     = 0x0F;
  libretro_to_ep128emu_kbmap[RETROK_RCTRL]     = 0x0F;

  libretro_to_ep128emu_kbmap[RETROK_u]         = 0x10;
  libretro_to_ep128emu_kbmap[RETROK_q]         = 0x11;
  libretro_to_ep128emu_kbmap[RETROK_y]         = 0x12;
  libretro_to_ep128emu_kbmap[RETROK_r]         = 0x13;
  libretro_to_ep128emu_kbmap[RETROK_t]         = 0x14;
  libretro_to_ep128emu_kbmap[RETROK_e]         = 0x15;
  libretro_to_ep128emu_kbmap[RETROK_w]         = 0x16;
  libretro_to_ep128emu_kbmap[RETROK_TAB]       = 0x17;
  libretro_to_ep128emu_kbmap[RETROK_7]         = 0x18;
  libretro_to_ep128emu_kbmap[RETROK_1]         = 0x19;
  libretro_to_ep128emu_kbmap[RETROK_6]         = 0x1A;
  libretro_to_ep128emu_kbmap[RETROK_4]         = 0x1B;
  libretro_to_ep128emu_kbmap[RETROK_5]         = 0x1C;
  libretro_to_ep128emu_kbmap[RETROK_3]         = 0x1D;
  libretro_to_ep128emu_kbmap[RETROK_2]         = 0x1E;
  libretro_to_ep128emu_kbmap[RETROK_ESCAPE]    = 0x1F;

  libretro_to_ep128emu_kbmap[RETROK_F4]        = 0x20;
  libretro_to_ep128emu_kbmap[RETROK_F8]        = 0x21;
  libretro_to_ep128emu_kbmap[RETROK_F3]        = 0x22;
  libretro_to_ep128emu_kbmap[RETROK_F6]        = 0x23;
  libretro_to_ep128emu_kbmap[RETROK_F5]        = 0x24;
  libretro_to_ep128emu_kbmap[RETROK_F7]        = 0x25;
  libretro_to_ep128emu_kbmap[RETROK_F2]        = 0x26;
  libretro_to_ep128emu_kbmap[RETROK_F1]        = 0x27;
  libretro_to_ep128emu_kbmap[RETROK_8]         = 0x28;
  //libretro_to_ep128emu_kbmap[???]            = 0x29;
  libretro_to_ep128emu_kbmap[RETROK_9]         = 0x2A;
  libretro_to_ep128emu_kbmap[RETROK_MINUS]     = 0x2B;
  libretro_to_ep128emu_kbmap[RETROK_0]         = 0x2C;
  libretro_to_ep128emu_kbmap[RETROK_CARET]     = 0x2D; // ? how is it mapped in the frontend?
  libretro_to_ep128emu_kbmap[RETROK_EQUALS]    = 0x2D; // probably there is no "caret" key on the keyboard
  libretro_to_ep128emu_kbmap[RETROK_BACKSPACE] = 0x2E;
  //libretro_to_ep128emu_kbmap[???]            = 0x2F;

  libretro_to_ep128emu_kbmap[RETROK_j]         = 0x30;
  //libretro_to_ep128emu_kbmap[???]            = 0x31;
  libretro_to_ep128emu_kbmap[RETROK_k]         = 0x32;
  libretro_to_ep128emu_kbmap[RETROK_SEMICOLON] = 0x33;
  libretro_to_ep128emu_kbmap[RETROK_l]         = 0x34;
  libretro_to_ep128emu_kbmap[RETROK_COLON]     = 0x35; // ? how is it mapped in the frontend?
  libretro_to_ep128emu_kbmap[RETROK_QUOTE]     = 0x35; // probably there is no "colon" key on the keyboard
  libretro_to_ep128emu_kbmap[RETROK_RIGHTBRACKET] = 0x36;
  //libretro_to_ep128emu_kbmap[???]            = 0x37;
  libretro_to_ep128emu_kbmap[RETROK_F10]       = 0x38; // STOP
  libretro_to_ep128emu_kbmap[RETROK_SYSREQ]    = 0x38; // STOP
  libretro_to_ep128emu_kbmap[RETROK_DOWN]      = 0x39;
  libretro_to_ep128emu_kbmap[RETROK_RIGHT]     = 0x3A;
  libretro_to_ep128emu_kbmap[RETROK_UP]        = 0x3B;
  libretro_to_ep128emu_kbmap[RETROK_F9]        = 0x3C; // HOLD
  libretro_to_ep128emu_kbmap[RETROK_SCROLLOCK] = 0x3C; // HOLD
  libretro_to_ep128emu_kbmap[RETROK_LEFT]      = 0x3D;
  libretro_to_ep128emu_kbmap[RETROK_RETURN]    = 0x3E;
  libretro_to_ep128emu_kbmap[RETROK_LALT]      = 0x3F;
  libretro_to_ep128emu_kbmap[RETROK_RALT]      = 0x3F;

  libretro_to_ep128emu_kbmap[RETROK_m]         = 0x40;
  libretro_to_ep128emu_kbmap[RETROK_DELETE]    = 0x41;
  libretro_to_ep128emu_kbmap[RETROK_COMMA]     = 0x42;
  libretro_to_ep128emu_kbmap[RETROK_SLASH]     = 0x43;
  libretro_to_ep128emu_kbmap[RETROK_PERIOD]    = 0x44;
  libretro_to_ep128emu_kbmap[RETROK_RSHIFT]    = 0x45;
  libretro_to_ep128emu_kbmap[RETROK_SPACE]     = 0x46;
  libretro_to_ep128emu_kbmap[RETROK_INSERT]    = 0x47;
  libretro_to_ep128emu_kbmap[RETROK_i]         = 0x48;
  //libretro_to_ep128emu_kbmap[???]            = 0x49;
  libretro_to_ep128emu_kbmap[RETROK_o]         = 0x4A;
  libretro_to_ep128emu_kbmap[RETROK_AT]        = 0x4B; // ? how is it mapped in the frontend?
  libretro_to_ep128emu_kbmap[RETROK_BACKQUOTE] = 0x4B; // probably there is no "at" key on the keyboard
  libretro_to_ep128emu_kbmap[RETROK_p]         = 0x4C;
  libretro_to_ep128emu_kbmap[RETROK_LEFTBRACKET] = 0x4D;
  //libretro_to_ep128emu_kbmap[???]            = 0x4E;
  //libretro_to_ep128emu_kbmap[???]            = 0x4F;

  // Numeric keypad mapped to ext 1 joystick, fire = 0/ins
  libretro_to_ep128emu_kbmap[RETROK_KP6]        = 0x70;
  libretro_to_ep128emu_kbmap[RETROK_KP4]        = 0x71;
  libretro_to_ep128emu_kbmap[RETROK_KP2]        = 0x72;
  libretro_to_ep128emu_kbmap[RETROK_KP8]        = 0x73;
  libretro_to_ep128emu_kbmap[RETROK_KP0]        = 0x74;

  if(machineType == MACHINE_TVC)
  {
    /* tvc extra keys in original ep128emu (tvc key - EP key - default PC key):
    - ö - @ - `
    - í - Esc - Esc
    - @ - F1 - F1
    - ; - F2,Tab - F2,Tab
    - <> - F3 - F3
    - \ - F4 - F4
    - * - F5,Pause - F5,Home
    - ^ - F6 - F6
    - [ - F7 - F7
    - ] - F8 - F8
    - Esc - Stop - End
    */
    libretro_to_ep128emu_kbmap[RETROK_ESCAPE]     = 0x38; // move Esc back to Esc
    libretro_to_ep128emu_kbmap[RETROK_F9]         = 0x4B; // use F9 for 0
    libretro_to_ep128emu_kbmap[RETROK_BACKQUOTE]  = 0x4B;
    libretro_to_ep128emu_kbmap[RETROK_AT]         = -1;   // there can be only 2 mappings
    libretro_to_ep128emu_kbmap[RETROK_F10]        = 0x1F; // use F10 for í
    libretro_to_ep128emu_kbmap[RETROK_OEM_102]    = 0x1F; // use extra 102. key for í

  }
  else if (machineType == MACHINE_CPC)
  {
    // rshift mapped to enter
    // F0 is Pause
    // F9 is Stop (red)
    // Copy is Insert
    // F Dot is Alt
    // Clear is Delete
    // Delete is Erase
  }


  for(int i=0; i<RETROK_LAST; i++)
  {
    if(libretro_to_ep128emu_kbmap[i] >= 0)
    {
      if(config->keyboard[libretro_to_ep128emu_kbmap[i]][0] > 0)
      {
        config->keyboard[libretro_to_ep128emu_kbmap[i]][1] = i;
      }
      else
        config->keyboard[libretro_to_ep128emu_kbmap[i]][0] = i;
    }
  }
  config->keyboardMapChanged = true;
  if (machineType == MACHINE_CPC)
  {
    // external joystick 1: controller 1 (cursor keys usually not used for game control)
    inputJoyMap[0x73][0] = RETRO_DEVICE_ID_JOYPAD_UP;
    inputJoyMap[0x72][0] = RETRO_DEVICE_ID_JOYPAD_DOWN;
    inputJoyMap[0x71][0] = RETRO_DEVICE_ID_JOYPAD_LEFT;
    inputJoyMap[0x70][0] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
    inputJoyMap[0x75][0] = RETRO_DEVICE_ID_JOYPAD_X; // fire 2 default
    inputJoyMap[0x74][0] = RETRO_DEVICE_ID_JOYPAD_B; // fire 1
    // external joystick 2: controller 2
    inputJoyMap[0x7b][1] = RETRO_DEVICE_ID_JOYPAD_UP;
    inputJoyMap[0x7a][1] = RETRO_DEVICE_ID_JOYPAD_DOWN;
    inputJoyMap[0x79][1] = RETRO_DEVICE_ID_JOYPAD_LEFT;
    inputJoyMap[0x78][1] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
    inputJoyMap[0x7d][1] = RETRO_DEVICE_ID_JOYPAD_X; // fire 2 default
    inputJoyMap[0x7c][1] = RETRO_DEVICE_ID_JOYPAD_B; // fire 1
  }
  else if (machineType == MACHINE_ZX)
  {
    // Kempston interface mapped to ext 1
    inputJoyMap[0x73][0] = RETRO_DEVICE_ID_JOYPAD_UP;
    inputJoyMap[0x72][0] = RETRO_DEVICE_ID_JOYPAD_DOWN;
    inputJoyMap[0x71][0] = RETRO_DEVICE_ID_JOYPAD_LEFT;
    inputJoyMap[0x70][0] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
    inputJoyMap[0x74][0] = RETRO_DEVICE_ID_JOYPAD_X; // fire
    // The 'left' Sinclair joystick maps the joystick directions and the fire button to the 1 (left), 2 (right), 3 (down), 4 (up) and 5 (fire) keys
    inputJoyMap[0x1b][1] = RETRO_DEVICE_ID_JOYPAD_UP;
    inputJoyMap[0x1d][1] = RETRO_DEVICE_ID_JOYPAD_DOWN;
    inputJoyMap[0x19][1] = RETRO_DEVICE_ID_JOYPAD_LEFT;
    inputJoyMap[0x1e][1] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
    inputJoyMap[0x1c][1] = RETRO_DEVICE_ID_JOYPAD_X; // fire
    // The 'right' Sinclair joystick maps to keys 6 (left), 7 (right), 8 (down), 9 (up) and 0 (fire)
    inputJoyMap[0x2a][2] = RETRO_DEVICE_ID_JOYPAD_UP;
    inputJoyMap[0x28][2] = RETRO_DEVICE_ID_JOYPAD_DOWN;
    inputJoyMap[0x1a][2] = RETRO_DEVICE_ID_JOYPAD_LEFT;
    inputJoyMap[0x18][2] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
    inputJoyMap[0x2c][2] = RETRO_DEVICE_ID_JOYPAD_X; // fire
    // A cursor joystick interfaces maps to keys 5 (left), 6 (down), 7 (up), 8 (right) and 0 (fire). (Protek and AGF)
    // todo - it is not supported now
  }
  else
  {
    // internal joystick: controller 0
    inputJoyMap[0x3b][0] = RETRO_DEVICE_ID_JOYPAD_UP;
    inputJoyMap[0x39][0] = RETRO_DEVICE_ID_JOYPAD_DOWN;
    inputJoyMap[0x3d][0] = RETRO_DEVICE_ID_JOYPAD_LEFT;
    inputJoyMap[0x3a][0] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
    inputJoyMap[0x46][0] = RETRO_DEVICE_ID_JOYPAD_X; // space
    // external joystick 1: controller 1
    inputJoyMap[0x73][1] = RETRO_DEVICE_ID_JOYPAD_UP;
    inputJoyMap[0x72][1] = RETRO_DEVICE_ID_JOYPAD_DOWN;
    inputJoyMap[0x71][1] = RETRO_DEVICE_ID_JOYPAD_LEFT;
    inputJoyMap[0x70][1] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
    inputJoyMap[0x74][1] = RETRO_DEVICE_ID_JOYPAD_X; // fire
    // external joystick 2: controller 2
    inputJoyMap[0x7b][2] = RETRO_DEVICE_ID_JOYPAD_UP;
    inputJoyMap[0x7a][2] = RETRO_DEVICE_ID_JOYPAD_DOWN;
    inputJoyMap[0x79][2] = RETRO_DEVICE_ID_JOYPAD_LEFT;
    inputJoyMap[0x78][2] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
    inputJoyMap[0x7c][2] = RETRO_DEVICE_ID_JOYPAD_X; // fire
  }
  inputJoyMap[0xff][0] = RETRO_DEVICE_ID_JOYPAD_A; // special: info button
  inputJoyMap[0xfe][0] = RETRO_DEVICE_ID_JOYPAD_R3; // special: fit to content
  inputJoyMap[0x3e][0] = RETRO_DEVICE_ID_JOYPAD_Y; // enter
  inputJoyMap[0x2c][0] = RETRO_DEVICE_ID_JOYPAD_L; // 0
  inputJoyMap[0x19][0] = RETRO_DEVICE_ID_JOYPAD_R; // 1
  inputJoyMap[0x1e][0] = RETRO_DEVICE_ID_JOYPAD_L2; // 2
  inputJoyMap[0x1d][0] = RETRO_DEVICE_ID_JOYPAD_R2; // 3

  std::string buttonprefix("EPKEY_");
  std::string button;
  std::map< std::string, unsigned char >::const_iterator  iter_epkey;
  std::string joypadPrefix("RETRO_DEVICE_ID_JOYPAD_");
  std::string joypadButton;
  std::map< std::string, unsigned char >::const_iterator  iter_joypad;

  for (int i=0; i<16; i++)
  {
    if(config->joypad[i] != "")
    {
      button = buttonprefix + config->joypad[i];
      joypadButton = joypadPrefix + config->joypadButtons[i];
      //printf("processing %s -> %s \n",joypadButton.c_str(),button.c_str());
      iter_epkey = epkey_reverse.find(button);
      iter_joypad = retro_joypad_reverse.find(joypadButton);
      if (iter_epkey != epkey_reverse.end() && iter_joypad != retro_joypad_reverse.end())
      {
        inputJoyMap[((*iter_epkey).second)][0] = (*iter_joypad).second;
        infoMessage = infoMessage + config->joypadButtons[i] + "->" + config->joypad[i] + " ";
        //printf("assigning %d -> %d \n",(*iter_joypad).second, (*iter_epkey).second );
      }
    }
  }
}

void LibretroCore::update_keyboard(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers)
{
  //printf("incoming keycode: %d ",keycode);
  uint32_t convertedKeycode = config->convertKeyCode(keycode);
  //printf("converted: %d \n",convertedKeycode);
  if (convertedKeycode >= 0)
  {
    vmThread->setKeyboardState(convertedKeycode,down);
  }
}

void LibretroCore::update_input(retro_input_state_t input_state_cb, retro_environment_t environ_cb)
{
  int i;
  int port;
  bool currInputState;

  for(port=0; port<4; port++)
  {
    for(i=0; i<256; i++)
    {
      if(inputJoyMap[i][port]>=0)
      {
        currInputState = input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, inputJoyMap[i][port]);
        if (currInputState && !inputStateMap[i][port])
        {
          if(i<128)
          {
            vmThread->setKeyboardState(i,true);
          }
          else
          {
            if(i == 0xFF)
            {
              struct retro_message message;
              std::string longMsg = "Wait for end of startup sequence... ";
              if (startSequenceIndex < startSequence.length())
              {
                longMsg += infoMessage;
                message.msg = longMsg.c_str();
              }
              else
              {
                message.msg = infoMessage.c_str();
              }
              message.frames = 50 * 4;
              environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &message);
            }
            if(i == 0xFE && isHalfFrame)
            {
              //printf("button pressed halfframe: %d ",isHalfFrame?1:0);
              if (halfHeight != defaultHalfHeight || lineOffset > 0)
              {
                lineOffset = 0;
                halfHeight = defaultHalfHeight;
                //printf("restore to %d \n",halfHeight);
              }
              else
              {
                int keepBorders = 6;
                int firstNonzeroLine = w->firstNonzeroLine - keepBorders > 0 ? w->firstNonzeroLine -keepBorders : 0;
                int lastNonzeroLine = w->lastNonzeroLine + keepBorders < EP128EMU_LIBRETRO_SCREEN_HEIGHT ? w->lastNonzeroLine + keepBorders : EP128EMU_LIBRETRO_SCREEN_HEIGHT;
                int detectedHeight = (lastNonzeroLine - firstNonzeroLine) / 2 + 1;

                int detectedEmptyRangeBottom = EP128EMU_LIBRETRO_SCREEN_HEIGHT - lastNonzeroLine;
                int proposedLineOffset = currWidth * ((firstNonzeroLine)/2);
                if (detectedHeight <= 150)
                {
                  if (firstNonzeroLine < detectedEmptyRangeBottom )
                  {
                    if (firstNonzeroLine < 150)
                    {
                      detectedHeight = (EP128EMU_LIBRETRO_SCREEN_HEIGHT - 2*firstNonzeroLine)/2+1;
                    }
                  }
                  else
                  {
                    if (detectedEmptyRangeBottom  < 150)
                    {
                      detectedHeight = (EP128EMU_LIBRETRO_SCREEN_HEIGHT - 2*detectedEmptyRangeBottom)/2+1;
                      proposedLineOffset = currWidth * (detectedEmptyRangeBottom/2);
                    }
                  }
                }
                printf("detected %d \n",detectedHeight);
                if (detectedHeight > 150 && detectedHeight < defaultHalfHeight-50)
                {
                  halfHeight = detectedHeight+1;
                  lineOffset = proposedLineOffset;
                }

              }
            }

          }
        }
        else if (inputStateMap[i][port] && !currInputState)
        {
          if(i<128)
            vmThread->setKeyboardState(i,false);
        }
        inputStateMap[i][port] = currInputState;
      }
    }
  }
  //printf("frame %d startseq %d \n",w->frameCount,startSequenceIndex);
  if (startSequenceIndex < startSequence.length())
  {
    if (w->frameCount == (bootframes[machineDetailedType] + startSequenceIndex*20))
    {
      if((unsigned char)startSequence.at(startSequenceIndex) == 254)
      {
        update_keyboard(true,RETROK_LSHIFT,0,0);
        update_keyboard(true,RETROK_2,0,0);
      }
      else
      {
        update_keyboard(true,startSequence.at(startSequenceIndex),0,0);
      }
      startSequenceIndex++;
    }
  }
  if (startSequenceIndex <= startSequence.length())
  {
    if (startSequenceIndex > 0 && w->frameCount == (bootframes[machineDetailedType] + (startSequenceIndex-1)*20+10))
    {
      if((unsigned char)startSequence.at(startSequenceIndex-1) == 254)
      {
        update_keyboard(false,RETROK_LSHIFT,0,0);
        update_keyboard(false,RETROK_2,0,0);
      }
      else
      {
        update_keyboard(false,startSequence.at(startSequenceIndex-1),0,0);
      }
      if (startSequenceIndex == startSequence.length())
      {
        startSequenceIndex++;
        // Start tape for ZX at the end of startup sequence
        if(machineDetailedType == ZX128_TAPE)
        {
          vm->tapePlay();
        }
      }
    }
  }
}
void LibretroCore::render(retro_video_refresh_t video_cb, retro_environment_t environ_cb)
{
  bool resolutionChanged = false;
  unsigned stride  = EP128EMU_LIBRETRO_SCREEN_WIDTH;
  if (useHalfFrame && isHalfFrame && w->interlacedFrameCount > 0)
  {
    isHalfFrame = false;
    change_resolution(currWidth, fullHeight, environ_cb);
    resolutionChanged = true;
  }
  else if (useHalfFrame && ((!isHalfFrame && w->interlacedFrameCount == 0) || (currHeight != halfHeight)))
  {
    isHalfFrame = true;
    change_resolution(currWidth, halfHeight, environ_cb);
    resolutionChanged = true;
  }
  if (canSkipFrames && prevFrameCount == w->frameCount)
  {
    //printf("frame dupe %d \n",prevFrameCount);
    video_cb(NULL, 0, 0, 0);
  }
  else
  {
    prevFrameCount = w->frameCount;
    if (!resolutionChanged)
    {
#ifdef EP128EMU_USE_XRGB8888
      video_cb(w->frame_bufReady+lineOffset, currWidth, currHeight, stride << 2);
#else
      video_cb(w->frame_bufReady+lineOffset, currWidth, currHeight, stride << 1);
#endif
    }
    else
    {
#ifdef EP128EMU_USE_XRGB8888
      video_cb(w->frame_bufSpare+lineOffset, currWidth, currHeight, stride << 2);
#else
      video_cb(w->frame_bufSpare+lineOffset, currWidth, currHeight, stride << 1);
#endif
    }
    //w->frame_bufLastReleased = w->frame_bufReady;
  }
}

void LibretroCore::change_resolution(int width, int height, retro_environment_t environ_cb)
{

  float aspect = 4.0f / 3.0f;
  //if(!showOverscan)
  aspect = 4.0f / (3.0f / (float) (isHalfFrame ? EP128EMU_LIBRETRO_SCREEN_HEIGHT/2/(float)height : EP128EMU_LIBRETRO_SCREEN_HEIGHT/(float)height));
  //aspect = 4.0f / (3.0f / (float) (EP128EMU_LIBRETRO_SCREEN_HEIGHT/(float)height));
  retro_game_geometry g =
  {
    .base_width   = (unsigned int) width,
    .base_height  = (unsigned int) height,
    .max_width    = EP128EMU_LIBRETRO_SCREEN_WIDTH,
    .max_height   = EP128EMU_LIBRETRO_SCREEN_HEIGHT,
    .aspect_ratio = aspect,
  };
  environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &g);
  currWidth = width;
  currHeight = height;
  //printf("Set half resolution done\n");
}

void LibretroCore::start(void)
{
  vmThread->setSpeedPercentage(0);
  vmThread->lock(0x7FFFFFFF);
  vmThread->unlock();
  vmThread->pause(false);
}

void LibretroCore::run_for(retro_usec_t frameTime, float waitPeriod, void * fb)
{
  retro_usec_t halfFrame = 0;//frameTime > 10000 ? 5000 : frameTime > 1000 ? 1000 : 0;
//  bool frameDone = false;
  //Ep128Emu::VMThread::VMThreadStatus  vmThreadStatus(*vmThread);
  //printf("frame: %d usec, start %f ",frameTime, vmThread->speedTimer.getRealTime());
  totalTime += frameTime;
  //vmThread->unlock();
  vmThread->allowRunFor(frameTime-halfFrame);
  if(fb)
  {
#ifdef EP128EMU_USE_XRGB8888
    w->frame_bufActive = (uint32_t*)fb;
  }
  else
  {
    w->frame_bufActive = (uint32_t*) w->frame_bufSpare;
#else
    w->frame_bufActive = (uint16_t*)fb;
  }
  else
  {
    w->frame_bufActive = (uint16_t*) w->frame_bufSpare;
#endif // EP128EMU_USE_XRGB8888

  }
  do
  {
    w->wakeDisplay(false);
    /*    frameDone = w->checkEvents() || frameDone;
        if (frameDone) {
        w->draw();
        frameDone = false;
        }*/
    if (vmThread->isReady()) break;
    if (waitPeriod > 0)
      Timer::wait(waitPeriod);
  }
  while(true);
  //w->wakeDisplay(false);
  //vmThread->waitUntilReady();

  //printf(" end %f \n",vmThread->speedTimer.getRealTime());
  //vmThread->lock(0x7FFFFFFF);
  //vmThread->allowRunFor(halfFrame);
}

void LibretroCore::sync_display(void)
{
  w->wakeDisplay(true);
}

void LibretroCore::errorCallback(void *userData, const char *msg)
{
  (void) userData;
  std::fprintf(stderr, "WARNING: %s\n", msg);
}


}       // namespace Ep128Emu

