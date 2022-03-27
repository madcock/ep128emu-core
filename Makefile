STATIC_LINKING := 0
AR             := ar

ifeq ($(platform),)
platform = unix
endif

# system platform
system_platform = unix

TARGET_NAME := ep128emu_core
#LIBM		= -lm -lpthread -lsndfile -lportaudio
LIBM		= -lm -lpthread

ifeq ($(ARCHFLAGS),)
   ARCHFLAGS = -arch i386 -arch x86_64
endif

ifeq ($(STATIC_LINKING), 1)
EXT := a
endif

ifeq ($(platform), unix)
	EXT ?= so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
   CC := g++
endif

LDFLAGS := -Wl,--as-needed
LDFLAGS += $(LIBM)
LDFLAGS += -s

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g
else
   CFLAGS += -O3 -mtune=generic -fno-inline-functions -fomit-frame-pointer -ffast-math
endif
# CFLAGS += -march=native -mtune=generic-armv7-a -mhard-float
# CFLAGS += -DEP128EMU_USE_XRGB8888
CFLAGS += -DEXCLUDE_SOUND_LIBS -DEP128EMU_LIBRETRO_CORE

include Makefile.common

INCLUDES := $(INCFLAGS)
OBJECTS := $(SOURCES_CPP:.cpp=.o)
OBJECTS += $(SOURCES_C:.c=.o)
CFLAGS += -Wall -pedantic $(fpic)

all Release: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CC) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS)
endif

%.o: %.cpp
	$(CC) $(CFLAGS) -std=c++2a $(INCLUDES) $(fpic) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -fpermissive $(INCLUDES) $(fpic) -c -o $@ $<

clean cleanRelease:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

