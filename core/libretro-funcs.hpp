
// ep128emu-core -- libretro core version of the ep128emu emulator
// Copyright (C) 2022 Zoltan Balogh
// https://github.com/zoltanvb/ep128emu-core
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

#ifndef EP128EMU_LIBRETRO_FUNCS_HPP
#define EP128EMU_LIBRETRO_FUNCS_HPP

#include "libretro.h"

#define EP128EMU_SAMPLE_RATE 44100
#define EP128EMU_SAMPLE_RATE_FLOAT 44100.0
//#define EP128EMU_USE_XRGB8888 1
#define EP128EMU_SNAPSHOT_SIZE 213823

#define EP128EMU_LIBRETRO_SCREEN_WIDTH 768
#define EP128EMU_LIBRETRO_SCREEN_HEIGHT 576

#define EP128EMU_MAX_USERS 6

#endif
