# ep128emu-core
> Use ep128emu as a libretro core

Leverage the convenience of libretro/retroarch to emulate the Z80 based home 
computers that the original ep128emu supports - that is, Enterprise 64/128, 
Videoton TVC, Amstrad CPC and ZX Spectrum. Focus is on Enterprise and TVC.

## Features

### Content types supported:
* Enterprise disk images: `img`, `dsk`
* Enterprise tape images: `tap` (either ep128emu or epte format)
* Enterprise compressed files: `dtf` (via "ZozoTools" ROM)
* Enterprise direct files: `com`, `trn`, `128`, `bas` or `.` (no extension)
* TVC disk images: `img`, `dsk` (TVC type guess from disk layout)
* TVC direct files: `cas`
* CPC disk images: `img`, `dsk` (CPCEMU and extended formats)
* CPC tape images: `cdt`
* Spectrum tape images: `tzx`
* Spectrum direct files: `tap`

### Input mapping and configuration
| Emulated machine | User 1 default | User 2 default | User 3 default |
| ---------------- | ------ | ------ | ------ |
| Enterprise | Internal | External 1 | External 2 |
| TVC | Internal | External 1 |  External 2 |
| CPC | External 1 | External 2 | |
| ZX | Kempston | Sinclair 1 | Sinclair 2 |

* Fire is **X** button. CPC Fire2 is **A** button.
* Keyboard input is enabled
  * Be aware that retroarch uses several hotkeys by default, use "Game focus" (Scroll Lock)
* Other buttons for User 1:
  * **L**-**R**-**L2**-**R2** buttons are 0-1-2-3 (several games use these in menu)
  * Use **L3** button to display current layout
* Configurable core options:
  * wait period (set to 0 if there are performance problems, set higher to reduce CPU load)
  * use software framebuffer (experimental, may crash)
  * use high quality sound (disable if there are performance problems)
  * enable resolution changes
  * amount of border to keep when zooming in
  * use original or enhanced ROM for Enterprise (faster memory test)
  * override joystick map for each user
  * zoom and info keys for player 1

### Other features
* Save/load state, rewind
* Content autostart except for disk images
* Customizable configuration (per-content or system-wide)
  * See [sample.ep128cfg](core/sample.ep128cfg) for details
  * User 1 button map can be finetuned
  * Joypad setup for other users can be changed
  * Emulated machine type can be changed
  * Autostart can be enabled also for disk images
  * Following files are recognized in `ep128emu/config` inside retroarch system directory: `enterprise.ep128cfg`, `tvc.ep128cfg`, `cpc.ep128cfg`, `zx.ep128cfg`
  * Most options from ep128emu configuration are also recognized in .ep128cfg files,
* Fit to content
  * Pressing **R3** will zoom in to the actual content, cropping black/single-colored borders
  * Borders can be cropped completely (default) or using a margin configurable among core options

For the emulation features, see the [original README](README). Since GUI is replaced by retroarch, features that would require own window (debugger, keyboard layout setting, etc) are not available. Some extra features not required for original games are also excluded (SD card, SID, MIDI, Spectrum emulation card for EP, mouse).

## Using the binaries

### Requirements

* A system with retroarch installed. Linux 64-bit, ARM 32-bit, and Windows 32 and 64 bit versions are currently available.
* ROM files for the systems to be emulated.
  * Enterprise: `exos21.rom`, `basic21.rom`, `epfileio.rom`, `exdos13.rom`
    * [epfileio.rom](roms/epfileio.rom) is a special image for ep128emu, needed only for direct file loading
    * For extra functions: `exos24uk.rom` (fast memory test), `zt19uk.rom` (DTF support)
    * TOSEC ROMs can also be used, exact file names can be checked at [core.hpp](core/core.hpp)
  * TVC: `tvc22_sys.rom`, `tvc22_ext.rom`, `tvcfileio.rom`, `tvc_dos12d.rom`
    * [tvcfileio.rom](roms/tvcfileio.rom) is a special image for ep128emu, needed only for direct file loading
    * ROMs from tvc.homeserver.hu can also be used, exact file names can be checked at [core.hpp](core/core.hpp)
  * CPC: `cpc6128.rom`, `cpc_amsdos.rom`
    * TOSEC ROMs can also be used, exact file names can be checked at [core.hpp](core/core.hpp)
  * ZX: `zx128.rom`
    * TOSEC ROMs or [retroarch-system](https://github.com/Abdess/retroarch_system/tree/libretro/Sinclair%20-%20ZX%20Spectrum) ROMs can also be used, exact file names can be checked at [core.hpp](core/core.hpp)
* Put the files to `ep128emu/roms` inside retroarch system directory. Example: `~/.config/retroarch/system/ep128emu/roms/`

### Running the core
```shell
# Linux
retroarch -L ep128emu_core_libretro.so -v <content file>
# Windows
retroarch -L ep128emu_core_libretro.dll -v <content file>
```
For most content types, there is a startup sequence that will do the program loading, if possible. Use fast-forward if loading is slow (such as tape input).


## Contributing

Pull requests welcome. Especially for currently unsupported builds (like MacOS).

## Links


## Licensing

Staying with GPL2 as ep128emu.
