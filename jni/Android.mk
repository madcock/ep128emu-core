LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

include $(CORE_DIR)/Makefile.common

COREFLAGS := -D__LIBRETRO__ -DEP128EMU_LIBRETRO_CORE -DEXCLUDE_SOUND_LIBS $(INCFLAGS)

GIT_VERSION ?= " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C) $(SOURCES_CPP)
LOCAL_CXXFLAGS  := $(COREFLAGS) -fexceptions -frtti 
LOCAL_CFLAGS    := $(COREFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/link.T
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)
