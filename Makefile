TARGET_NAME := ep128emu_core
STATIC_LINKING := 0
AR             := ar
DEBUG   = 0
LIBS    :=

ifeq ($(platform),)
	platform = unix
	ifeq ($(shell uname -a),)
		platform = win
	else ifneq ($(findstring MINGW,$(shell uname -a)),)
		platform = win
	else ifneq ($(findstring Darwin,$(shell uname -a)),)
		platform = osx
	else ifneq ($(findstring win,$(shell uname -a)),)
		platform = win
	else ifneq ($(findstring armv,$(shell uname -a)),)
		platform = armv
	endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
	ifeq ($(shell uname -p),powerpc)
		arch = ppc
	endif
	ifeq ($(shell uname -p),arm)
		arch = arm
	endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

# Unix
ifneq (,$(findstring unix,$(platform)))
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,-version-script=link.T -Wl,-no-undefined
	CC = gcc
	CC_AS = gcc
	CXX = g++
	PLATFORM_DEFINES += -mtune=generic
else ifneq (,$(findstring linux-portable,$(platform)))
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC -nostdlib
	LIBM :=
	SHARED := -shared -Wl,-version-script=link.T
# ARM
else ifneq (,$(findstring armv,$(platform)))
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,-version-script=link.T
	CC = gcc
	CC_AS = gcc
	CXX = g++
	ifneq (,$(findstring cortexa8,$(platform)))
		PLATFORM_DEFINES += -marm -mcpu=cortex-a8
	else ifneq (,$(findstring cortexa9,$(platform)))
		PLATFORM_DEFINES += -marm -mcpu=cortex-a9
	endif
	PLATFORM_DEFINES += -marm
	PLATFORM_DEFINES += -mtune=generic-armv7-a -mhard-float

# cross Windows
else ifeq ($(platform), wincross64)
	TARGET := $(TARGET_NAME)_libretro.dll
	AR = x86_64-w64-mingw32-ar
	CC = x86_64-w64-mingw32-gcc
	CXX = x86_64-w64-mingw32-g++
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -static-libgcc -static-libstdc++
	EXTRA_LDF := -lwinmm -Wl,--export-all-symbols
endif


ifeq ($(STATIC_LINKING), 1)
    EXT := a
endif

LIBM		= -lm -lpthread
CC_AS ?= $(CC)

LDFLAGS := -Wl,--as-needed
LDFLAGS += $(LIBM)

ifeq ($(DEBUG), 1)
	CFLAGS += -O0 -g
	CXXFLAGS += -O0 -g
else
    CFLAGS += -O3 -fno-inline-functions -fomit-frame-pointer -ffast-math
	CXXFLAGS += -O3 -fno-inline-functions -fomit-frame-pointer -ffast-math
    LDFLAGS += -s
endif

DEFINES := $(PLATFORM_DEFINES) -DEXCLUDE_SOUND_LIBS -DEP128EMU_LIBRETRO_CORE

# DEFINES += -DEP128EMU_USE_XRGB8888

CFLAGS += $(DEFINES)
CXXFLAGS += $(DEFINES) -std=c++2a

include Makefile.common

INCLUDES := $(INCFLAGS)
OBJECTS := $(SOURCES_CPP:.cpp=.o)
OBJECTS += $(SOURCES_C:.c=.o)
CFLAGS += -Wall -pedantic $(fpic)
CXXFLAGS += -Wall -pedantic $(fpic)

all Release: $(TARGET)

$(TARGET): $(OBJECTS)

ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CXX) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS)
	#@$(CC) -o $@ $(SHARED) $(OBJS) $(LDFLAGS) $(LIBS)
endif

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(fpic) -c -o $@ $<
	#$(CXX) -c -o $@ $< $(CXXFLAGS) $(INCDIRS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(fpic) -c -o $@ $<
	#@echo $@
	#@$(CC) -c -o $@ $< $(CFLAGS) $(INCDIRS)

clean cleanRelease:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

