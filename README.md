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
* Default d-pad input mapping for player 1:
  * Enterprise: internal joystick
  * TVC: internal joystick
  * CPC: external joystick
  * ZX: Kempston
  * Fire is **X** button, **L**-**R**-**L2**-**R2** buttons are 0-1-2-3 (several games use these in menu)
  * Keyboard input is enabled, be aware that retroarch uses several hotkeys by default
  * Use **A** button to display current layout
* Default d-pad input mappings for further players
  * Enterprise: external 1 (also mapped to numpad), external 2
  * TVC: external 1 (also mapped to numpad), external 2
  * CPC: external 2
  * ZX: Sinclair 1, Sinclair 2, Protek
* Configurable core options:
  * wait period (set to 0 if there are performance problems, set higher to reduce CPU load)
  * use software framebuffer (experimental, may crash)
  * use high quality sound (disable if there are performance problems)
  * enable resolution changes
  * amount of border to keep when zooming in
  * use original or enhanced ROM for Enterprise (faster memory test)
  * override joystick map for each user

### Other features
* Save/load state, rewind
* Content autostart except for disk images
* Customizable input mapping for player 1
  * See [sample.ep128cfg](core/sample.ep128cfg) to see how retropad buttons can be assigned for a specific game.
  * Autostart file name can also be specified to enable autostart for disk.
* System-wide configuration file
  * Following files are recognized in `ep128emu/config` inside retroarch system directory: `enterprise.ep128cfg`, `tvc.ep128cfg`, `pc.ep128cfg`, `zx.ep128cfg`
  * Most options from ep128emu configuration are also recognized in .ep128cfg files,
* Fit to content
  * Pressing **R3** will zoom in to the actual content, cropping black/single-colored borders
  * Borders can be cropped completely (default) or using a margin configurable among core options

For the emulation features, see the [original README](README). SD card, SID, MIDI emulation is not available in ep128emu-core.

## Using the binaries

### Requirements

* A system with retroarch installed. Currently Linux x86_64 and arm builds are
available.
* ROM files for the systems to be emulated.
  * Enterprise: `exos21.rom`, `basic21.rom`, `epfileio.rom`, `exdos13.rom`
    * [epfileio.rom](roms/epfileio.rom) is a special image for ep128emu, needed only for direct file loading
    * For extra functions: `exos24uk.rom` (fast memory test), `zt19uk.rom` (DTF support)
    * TOSEC ROMs can also be used, exact file names can be checked at [core.hpp](core/core.hpp)
  * TVC: `tvc22_sys.rom`, `tvc22_ext.rom`, `tvcfileio.rom`, `tvc_dos12d.rom`
    * [tvcfileio.rom](roms/tvcfileio.rom) is a special image for ep128emu, needed only for direct file loading
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

Pull requests welcome. Especially for currently unsupported builds (like Windows or MacOS).

## Links


## Licensing

To be specified, probably staying with GPL2 as ep128emu.
