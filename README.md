# ep128emu-core
> Use ep128emu as a libretro core

Leverage the convenience of libretro/retroarch to emulate the Z80 based home 
computers that the original ep128emu supports - that is, Enterprise 64/128, 
Videoton TVC, Amstrad CPC and ZX Spectrum. Focus is on Enterprise and TVC.

## Features

Libretro features supported:
* Load disk images, tape images, or direct files
  * Enterprise disk images: `img`, `dsk`
  * Enterprise tape images: `tap` (either ep128emu or epte format)
  * Enterprise direct files: `com`, `trn`, `128`, `bas` or `.` (no extension)
  * TVC disk images: `img`, `dsk` (TVC type guess from disk layout)
  * TVC direct files: `cas`
  * CPC disk images: `img`, `dsk` (CPCEMU and extended formats)
  * CPC tape images: `cdt`
  * Spectrum tape images: `tzx`
  * Spectrum direct files: `tap`
* Save/load state, rewind
* Crop overscan
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
  * ZX: Sinclair 1, Sinclair 2
* Configurable core options:
  * wait period (set to 0 if there are performance problems, set higher to reduce CPU load)
  * use software framebuffer (experimental, may crash)
  * use high quality sound (disable if there are performance problems)

Features added by ep128emu-core
* Content autostart except for disk images
* Customizable input mapping for player 1
  * See [sample.ep128cfg](core/sample.ep128cfg) to see how retropad buttons can be assigned for a specific game.
  * Autostart file name can also be specified to enable autostart for disk.

For the emulation features, see the [original README](README).

## Using the binaries

### Requirements

* A system with retroarch installed. Currently Linux x86_64 and arm builds are
available.
* ROM files for the systems to be emulated.
  * Enterprise: ""exos21.rom"", ""basic21.rom"", ""epfileio.rom"", ""exdos13.rom""
  * TVC: ""tvc22_sys.rom"", ""tvc22_ext.rom"", ""tvcfileio.rom"", ""tvc_dos12d.rom""
  * CPC: ""cpc6128.rom"", ""cpc_amsdos.rom""
  * ZX: ""zx128.rom""
* Put the files to ""ep128emu/roms"" inside retroarch system directory. Example: ""~/.config/retroarch/system/ep128emu/roms/""

### Running the core
```shell
retroarch -L ep128emu_core_libretro.so -v
```
Load content from in-game retroarch menu (default F1)


## Contributing

Pull requests welcome. Especially for currently unsupported builds (like Windows or MacOS).

## Links


## Licensing

To be specified, probably staying with GPL2 as ep128emu.
