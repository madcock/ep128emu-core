#include "core.hpp"
#include "libretro_keys_reverse.h"
namespace Ep128Emu
{

LibretroCore::LibretroCore(retro_log_printf_t log_cb_, int machineDetailedType_, bool canSkipFrames_, const char* romDirectory_, const char* saveDirectory_,
                           const char* startSequence_, const char* cfgFile, bool useHalfFrame_, bool enhancedRom)
  : log_cb(log_cb_),
    useHalfFrame(useHalfFrame_),
    isHalfFrame(useHalfFrame_),
    canSkipFrames(canSkipFrames_),
    joypadConfigChanged(false),
    prevFrameCount(0),
    startSequenceIndex(0),
    currWidth(EP128EMU_LIBRETRO_SCREEN_WIDTH),
    fullHeight(EP128EMU_LIBRETRO_SCREEN_HEIGHT),
    halfHeight(EP128EMU_LIBRETRO_SCREEN_HEIGHT/2),
    defaultHalfHeight(EP128EMU_LIBRETRO_SCREEN_HEIGHT/2),
    defaultWidth(EP128EMU_LIBRETRO_SCREEN_WIDTH),
    borderSize(0),
    machineType(MACHINE_EP),
    machineDetailedType(machineDetailedType_),
    totalTime(0),
    vmThread(NULL),
    config(NULL)
{
  if(machineDetailedType == TVC64_DISK || machineDetailedType == TVC64_FILE)
  {
    machineType = MACHINE_TVC;
    log_cb(RETRO_LOG_INFO, "Emulated machine: TVC\n");
  }
  else if(machineDetailedType == CPC_DISK || machineDetailedType == CPC_TAPE)
  {
    machineType = MACHINE_CPC;
    log_cb(RETRO_LOG_INFO, "Emulated machine: CPC\n");
  }
  else if(machineDetailedType == ZX16_TAPE || machineDetailedType == ZX16_FILE ||
          machineDetailedType == ZX48_TAPE || machineDetailedType == ZX48_FILE ||
          machineDetailedType == ZX128_TAPE || machineDetailedType == ZX128_FILE )
  {
    machineType = MACHINE_ZX;
    log_cb(RETRO_LOG_INFO, "Emulated machine: ZX\n");
  }
  else {
      log_cb(RETRO_LOG_INFO, "Emulated machine: EP\n");
  }
  if(machineDetailedType == VM_CONFIG_UNKNOWN)
  {
    log_cb(RETRO_LOG_ERROR, "VM_CONFIG_UNKNOWN passed!\n");
    throw Ep128Emu::Exception("Machine configuration not recognized!");
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
  log_cb(RETRO_LOG_DEBUG, "VM created\n");
  config = new Ep128Emu::EmulatorConfiguration(
    *vm, *(dynamic_cast<Ep128Emu::VideoDisplay *>(w)), *audioOutput
#ifdef ENABLE_MIDI_PORT
    , *midiPort
#endif
  );
  //config->setErrorCallback(&LibretroCore::errorCallback, (void *) 0);
  currHeight = useHalfFrame ? EP128EMU_LIBRETRO_SCREEN_HEIGHT / 2 : EP128EMU_LIBRETRO_SCREEN_HEIGHT;
  std::string romBasePath(romDirectory_);
  std::string configBaseFile(romDirectory_);
#ifdef WIN32
  romBasePath.append("\\ep128emu\\roms\\");
  configBaseFile.append("\\ep128emu\\config\\");
#else
  romBasePath.append("/ep128emu/roms/");
  configBaseFile.append("/ep128emu/config/");
#endif

  startSequence = startSequence_;

  if(machineType == MACHINE_EP)
  {
    configBaseFile = configBaseFile + "enterprise.ep128cfg";
    config->memory.ram.size=128;
    if (enhancedRom)
    {
      bootframes[machineDetailedType] = 20*10;
      config->memory.rom[0x00].file=romBasePath+"exos24uk.rom";
      config->memory.rom[0x00].offset=0;
      config->memory.rom[0x01].file=romBasePath+"exos24uk.rom";
      config->memory.rom[0x01].offset=16384;
      config->memory.rom[0x02].file=romBasePath+"exos24uk.rom";
      config->memory.rom[0x02].offset=32768;
      config->memory.rom[0x03].file=romBasePath+"exos24uk.rom";
      config->memory.rom[0x03].offset=49152;
    }
    else
    {
      bootframes[machineDetailedType] = 40*10;
      config->memory.rom[0x00].file=romBasePath+"exos21.rom";
      config->memory.rom[0x00].offset=0;
      config->memory.rom[0x01].file=romBasePath+"exos21.rom";
      config->memory.rom[0x01].offset=16384;
    }
    // HUN ROM: goes to segment 4 and then Basic goes to segment 5
    //config->memory.rom[0x04].file=romBasePath+"hun.rom";
    //config->memory.rom[0x04].offset=0;
    config->memory.rom[0x04].file=romBasePath+"basic21.rom";
    config->memory.rom[0x04].offset=0;

    if(machineDetailedType == EP128_FILE || machineDetailedType == EP128_FILE_DTF)
    {
      config->memory.rom[0x10].file=romBasePath+"epfileio.rom";
      config->memory.rom[0x10].offset=0;
      if(machineDetailedType == EP128_FILE_DTF)
      {
        config->memory.rom[0x40].file=romBasePath+"zt19uk.rom";
        config->memory.rom[0x40].offset=0;
        config->memory.rom[0x41].file=romBasePath+"zt19uk.rom";
        config->memory.rom[0x41].offset=16384;
      }
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
    configBaseFile = configBaseFile + "tvc.ep128cfg";
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
    configBaseFile = configBaseFile + "cpc.ep128cfg";
    bootframes[machineDetailedType] = 20*10;
    config->memory.ram.size=128;
    config->memory.rom[0x10].file=romBasePath+"cpc6128.rom";
    config->memory.rom[0x10].offset=0;
    config->memory.rom[0x00].file=romBasePath+"cpc6128.rom";
    config->memory.rom[0x00].offset=16384;
    if(machineDetailedType == CPC_DISK)
    {
      config->memory.rom[0x07].file=romBasePath+"cpc_amsdos.rom";
      config->memory.rom[0x07].offset=0;
    }
  }
  else if(machineType == MACHINE_ZX)
  {
    configBaseFile = configBaseFile + "zx.ep128cfg";
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
  for(int i=0; i<68; i++)
  {
    if(config->memory.rom[i].file.length()>0)
    {
      if(!Ep128Emu::does_file_exist(config->memory.rom[i].file.c_str()))
      {
        std::map< std::string, std::string >::const_iterator  iter_altrom;
        std::string romShortName;
        int page_index = config->memory.rom[i].offset / 16384;
        std::string romPageName(std::to_string(page_index));
        std::string replacementFullName;
#ifdef WIN32
        size_t idx = config->memory.rom[i].file.rfind('\\');
#else
        size_t idx = config->memory.rom[i].file.rfind('/');
#endif
        if(idx != std::string::npos)
        {
          romShortName = config->memory.rom[i].file.substr(idx+1);
          romPageName = config->memory.rom[i].file.substr(idx+1) + "_p" + romPageName;
        }
        else
        {
          log_cb(RETRO_LOG_ERROR, "Invalid ROM name: %s \n",config->memory.rom[i].file.c_str());
          throw Ep128Emu::Exception("ROM file not found!");
        }

        bool romFound = false;

        for (iter_altrom = rom_names_ep128emu_tosec.begin(); iter_altrom != rom_names_ep128emu_tosec.end(); ++iter_altrom)
        {
          if ((*iter_altrom).first == romShortName || (*iter_altrom).first == romPageName )
          {
            replacementFullName = (*iter_altrom).second.c_str();
            replacementFullName = romBasePath + replacementFullName;
            if(Ep128Emu::does_file_exist(replacementFullName.c_str()))
            {
              log_cb(RETRO_LOG_INFO, "ROM file alternative found: %s => %s\n",romPageName.c_str(), (*iter_altrom).second.c_str());
              config->memory.rom[i].file = replacementFullName;
              romFound = true;
              // If split ROM is used, remove offset.
              if ((*iter_altrom).first == romPageName)
                config->memory.rom[i].offset=0;
              break;
            }
            else
            {
              log_cb(RETRO_LOG_DEBUG, "ROM file alternative not found: %s => %s\n",romShortName.c_str(),replacementFullName.c_str());
            }
          }
        }
        if(!romFound)
        {
          log_cb(RETRO_LOG_ERROR, "ROM file or any alternative not found: %s \n",config->memory.rom[i].file.c_str());
          throw Ep128Emu::Exception("ROM file not found!");
        }

      }
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

  // Priority order of settings:
  // lowest: hardcoded defaults (above)
  // medium: system-wide defaults per machine type (this one)
  if(Ep128Emu::does_file_exist(configBaseFile.c_str()))
  {
    log_cb(RETRO_LOG_INFO, "Loading system wide configuration file: %s\n",configBaseFile.c_str());
    config->loadState(configBaseFile.c_str(),false);
  }
  // highest: content-specific file
  if (cfgFile[0])
  {
    log_cb(RETRO_LOG_INFO, "Loading content specific configuration file: %s\n",configBaseFile.c_str());
    config->loadState(cfgFile,false);
  }
  initialize_keyboard_map();
  initialize_joystick_map(JOY_DEFAULT,JOY_DEFAULT,JOY_DEFAULT,JOY_DEFAULT);
  if (config->contentFileName != "")
  {
    startSequence = startSequence + config->contentFileName + "\xfe\r";
  }
  //config->display.enabled = false;
  //config->displaySettingsChanged = true;
  log_cb(RETRO_LOG_DEBUG, "Applying settings\n");
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
  libretro_to_ep128emu_kbmap[RETROK_COLON]     = 0x35; // :
  libretro_to_ep128emu_kbmap[RETROK_QUOTE]     = 0x35; // probably there is no : key on the keyboard
  libretro_to_ep128emu_kbmap[RETROK_RIGHTBRACKET] = 0x36;
  //libretro_to_ep128emu_kbmap[???]            = 0x37;
  libretro_to_ep128emu_kbmap[RETROK_F10]       = 0x38; // STOP
  libretro_to_ep128emu_kbmap[RETROK_SYSREQ]    = 0x38; // STOP
  libretro_to_ep128emu_kbmap[RETROK_DOWN]      = 0x39;
  libretro_to_ep128emu_kbmap[RETROK_RIGHT]     = 0x3A;
  libretro_to_ep128emu_kbmap[RETROK_UP]        = 0x3B;
  libretro_to_ep128emu_kbmap[RETROK_F9]        = 0x3C; // HOLD
  //libretro_to_ep128emu_kbmap[RETROK_SCROLLOCK] = 0x3C; // HOLD - conflicts with "game focus" default
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
  libretro_to_ep128emu_kbmap[RETROK_AT]        = 0x4B; // @
  libretro_to_ep128emu_kbmap[RETROK_BACKQUOTE] = 0x4B; // probably there is no @ key on the keyboard
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
    libretro_to_ep128emu_kbmap[RETROK_BACKQUOTE]  = 0x4B; // or the key left of 1
    libretro_to_ep128emu_kbmap[RETROK_AT]         = -1;   // there can be only 2 mappings
    libretro_to_ep128emu_kbmap[RETROK_F10]        = 0x1F; // use F10 for í
    libretro_to_ep128emu_kbmap[RETROK_OEM_102]    = 0x1F; // or the extra 102. key
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
}

void LibretroCore::initialize_joystick_map(int user1, int user2, int user3, int user4)
{
  // Lowest priority joystick settings: machine dependent hardcoded defaults.
  if (machineType == MACHINE_CPC)
  {
    // external joystick 1: controller 1 (cursor keys usually not used for game control)
    update_joystick_map(joystickCodesExt1,0,6);
    // external joystick 2: controller 2
    update_joystick_map(joystickCodesExt2,1,6);
  }
  else if (machineType == MACHINE_ZX)
  {
    // Kempston interface mapped to ext 1
    update_joystick_map(joystickCodesExt1,0,5);
    // The 'left' Sinclair joystick maps the joystick directions and the fire button to the 1 (left), 2 (right), 3 (down), 4 (up) and 5 (fire) keys
    update_joystick_map(joystickCodesSinclair1,1,5);
    // The 'right' Sinclair joystick maps to keys 6 (left), 7 (right), 8 (down), 9 (up) and 0 (fire)
    update_joystick_map(joystickCodesSinclair2,2,5);
    // A cursor joystick interfaces maps to keys 5 (left), 6 (down), 7 (up), 8 (right) and 0 (fire). (Protek and AGF)
    update_joystick_map(joystickCodesSinclair3,3,5);
  }
  else
  {
    // EP or TVC: internal joystick: controller 0
    update_joystick_map(joystickCodesInt,0,5);
    // external joystick 1: controller 1
    update_joystick_map(joystickCodesExt1,1,5);
    // external joystick 2: controller 2
    update_joystick_map(joystickCodesExt2,2,5);
  }
  inputJoyMap[0xff][0] = RETRO_DEVICE_ID_JOYPAD_A; // special: info button
  inputJoyMap[0xfe][0] = RETRO_DEVICE_ID_JOYPAD_R3; // special: fit to content
  inputJoyMap[0x3e][0] = RETRO_DEVICE_ID_JOYPAD_Y; // enter
  inputJoyMap[0x2c][0] = RETRO_DEVICE_ID_JOYPAD_L; // 0
  inputJoyMap[0x19][0] = RETRO_DEVICE_ID_JOYPAD_R; // 1
  inputJoyMap[0x1e][0] = RETRO_DEVICE_ID_JOYPAD_L2; // 2
  inputJoyMap[0x1d][0] = RETRO_DEVICE_ID_JOYPAD_R2; // 3
  infoMessage = "Key map: X->space Y->enter L->0 R->1 L2->2 R2->3 R3->zoom ";

  // Second priority: assignment read from .ep128cfg file
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
      iter_epkey = epkey_reverse.find(button);
      iter_joypad = retro_joypad_reverse.find(joypadButton);
      if (iter_epkey != epkey_reverse.end() && iter_joypad != retro_joypad_reverse.end())
      {
        inputJoyMap[((*iter_epkey).second)][0] = (*iter_joypad).second;
        infoMessage = infoMessage + config->joypadButtons[i] + "->" + config->joypad[i] + " ";
      }
    }
  }
  // Highest priority: core option settings (if they are not left on default)
  int mappings[4] = {user1, user2, user3, user4};
  for (int i=0; i<4; i++)
  {
    // TODO: 2nd / 3rd fire button assignment on need (CPC)
    if(mappings[i]>0)
      update_joystick_map(joystickCodeMap[mappings[i]],i,5);
  }
}

void LibretroCore::update_joystick_map(const unsigned char * joystickCodes, int port, int length)
{
  reset_joystick_map(port);
  inputJoyMap[joystickCodes[0]][port] = RETRO_DEVICE_ID_JOYPAD_UP;
  inputJoyMap[joystickCodes[1]][port] = RETRO_DEVICE_ID_JOYPAD_DOWN;
  inputJoyMap[joystickCodes[2]][port] = RETRO_DEVICE_ID_JOYPAD_LEFT;
  inputJoyMap[joystickCodes[3]][port] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
  inputJoyMap[joystickCodes[4]][port] = RETRO_DEVICE_ID_JOYPAD_X;
  if(length>=6)
    inputJoyMap[joystickCodes[5]][port] = RETRO_DEVICE_ID_JOYPAD_B;
  if(length>=7)
    inputJoyMap[joystickCodes[6]][port] = RETRO_DEVICE_ID_JOYPAD_Y;
}

void LibretroCore::reset_joystick_map(int port)
{
  for(int i=1; i<7; i++)
  {
    inputJoyMap[joystickCodeMap[i][0]][port] = -1;
    inputJoyMap[joystickCodeMap[i][1]][port] = -1;
    inputJoyMap[joystickCodeMap[i][2]][port] = -1;
    inputJoyMap[joystickCodeMap[i][3]][port] = -1;
    inputJoyMap[joystickCodeMap[i][4]][port] = -1;
  }
}

void LibretroCore::update_keyboard(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers)
{
  uint32_t convertedKeycode = config->convertKeyCode(keycode);
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
            if(i == 0xFE)
            {
              w->scanBorders = true;
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
  if (startSequenceIndex < startSequence.length())
  {
    if (w->frameCount == (bootframes[machineDetailedType] + startSequenceIndex*20))
    {
      if((unsigned char)startSequence.at(startSequenceIndex) == 254)
      {
        update_keyboard(true,RETROK_LSHIFT,0,0);
        update_keyboard(true,RETROK_2,0,0);
      }
      else if((unsigned char)startSequence.at(startSequenceIndex) == 253)
      {
        update_keyboard(true,RETROK_F1,0,0);
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
      else if((unsigned char)startSequence.at(startSequenceIndex-1) == 253)
      {
        update_keyboard(false,RETROK_F1,0,0);
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
  // Transition from half frame (normal video mode) to interlaced
  if (useHalfFrame && isHalfFrame && w->interlacedFrameCount > 0)
  {
    isHalfFrame = false;
    change_resolution(defaultWidth, fullHeight, environ_cb);
    w->resetViewport();
  }
  // Transition from interlaced back to half frame
  else if (useHalfFrame && (!isHalfFrame && w->interlacedFrameCount == 0))
  {
    isHalfFrame = true;
    change_resolution(defaultWidth, defaultHalfHeight, environ_cb);
    w->resetViewport();
  }
  else if (useHalfFrame && isHalfFrame && w->bordersScanned)
  {
    w->bordersScanned = false;
    // Transition from zoomed-in view back to default
    if(!w->isViewportDefault())
    {
      change_resolution(defaultWidth, defaultHalfHeight, environ_cb);
      w->resetViewport();
    }
    else
    {
      int proposedY1 = w->contentTopEdge - borderSize > 0 ? w->contentTopEdge - borderSize : 0;
      int proposedY2 = w->contentBottomEdge + borderSize < EP128EMU_LIBRETRO_SCREEN_HEIGHT ? w->contentBottomEdge + borderSize : EP128EMU_LIBRETRO_SCREEN_HEIGHT-1;
      int detectedHeight = proposedY2 - proposedY1 + 2;

      int proposedX1 = w->contentLeftEdge - borderSize > 0 ? w->contentLeftEdge - borderSize : 0;
      int proposedX2 = w->contentRightEdge + borderSize < EP128EMU_LIBRETRO_SCREEN_WIDTH ? w->contentRightEdge + borderSize : EP128EMU_LIBRETRO_SCREEN_WIDTH-1;
      int detectedWidth = proposedX2 - proposedX1 + 1;
      // todo: if the detected height is small, adjust it to be centered

      if (detectedHeight >= 150 && detectedWidth >= 200)
      {
        w->setViewport(proposedX1,proposedY1,proposedX2,proposedY2);
        log_cb(RETRO_LOG_INFO, "Viewport reduced: %d,%d / %d,%d\n",proposedX1,proposedY1,proposedX2,proposedY2);
        change_resolution(detectedWidth, detectedHeight/2, environ_cb);
      }
    }
  }
  unsigned stride  = currWidth;

  if (canSkipFrames && prevFrameCount == w->frameCount)
  {
    //printf("frame dupe %d \n",prevFrameCount);
    video_cb(NULL, 0, 0, 0);
  }
  else
  {
    prevFrameCount = w->frameCount;
    {
#ifdef EP128EMU_USE_XRGB8888
      video_cb(w->frame_bufActive, currWidth, currHeight, stride << 2);
#else
      video_cb(w->frame_bufActive, currWidth, currHeight, stride << 1);
#endif
    }
  }
}

void LibretroCore::change_resolution(int width, int height, retro_environment_t environ_cb)
{

  float aspect = 4.0f / 3.0f;
  // For some reason, aspect is wrong if based purely on pixel basis - if half height is used, it needs to be taken into account
  aspect = 4.0f / (3.0f /
                   (float) (isHalfFrame ? EP128EMU_LIBRETRO_SCREEN_HEIGHT/2/(float)height : EP128EMU_LIBRETRO_SCREEN_HEIGHT/(float)height) *
                   (float) (EP128EMU_LIBRETRO_SCREEN_WIDTH/(float)width)
                  );
  retro_game_geometry g =
  {
    .base_width   = (unsigned int) width,
    .base_height  = (unsigned int) height,
    .max_width    = EP128EMU_LIBRETRO_SCREEN_WIDTH,
    .max_height   = EP128EMU_LIBRETRO_SCREEN_HEIGHT,
    .aspect_ratio = aspect,
  };
  log_cb(RETRO_LOG_INFO, "Changing resolution: %d x %d\n",width,height);
  environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &g);
  currWidth = width;
  currHeight = height;
}

void LibretroCore::start(void)
{
  vmThread->setSpeedPercentage(0);
  vmThread->lock(0x7FFFFFFF);
  vmThread->unlock();
  vmThread->pause(false);
  log_cb(RETRO_LOG_DEBUG, "Core started\n");
}

void LibretroCore::run_for(retro_usec_t frameTime, float waitPeriod, void * fb)
{
  //Ep128Emu::VMThread::VMThreadStatus  vmThreadStatus(*vmThread);
  //log_cb(RETRO_LOG_DEBUG, "Running core for %d ms\n",frameTime);
  totalTime += frameTime;
  if(fb)
  {
#ifdef EP128EMU_USE_XRGB8888
    w->frame_bufActive = (uint32_t*)fb;
#else
    w->frame_bufActive = (uint16_t*)fb;
#endif // EP128EMU_USE_XRGB8888
  }
  else
  {
#ifdef EP128EMU_USE_XRGB8888
    w->frame_bufActive = (uint32_t*) w->frame_buf1;
#else
    w->frame_bufActive = (uint16_t*) w->frame_buf1;
#endif // EP128EMU_USE_XRGB8888
  }
  vmThread->allowRunFor(frameTime);

  do
  {
    w->wakeDisplay(false);
    if (vmThread->isReady()) break;
    if (waitPeriod > 0)
      Timer::wait(waitPeriod);
  }
  while(true);
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
