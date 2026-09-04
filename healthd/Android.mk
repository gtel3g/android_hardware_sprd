#
# Copyright (C) 2016 The Android Open Source Project
# Copyright (C) 2016 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := healthd.cpp
LOCAL_MODULE := libhealthd.$(TARGET_BOARD_PLATFORM)
LOCAL_C_INCLUDES := \
	system/core/healthd/include \
	system/core/base/include \
	bootable/recovery/minui/include

include $(BUILD_STATIC_LIBRARY)


#
# SPRD offline charger
#

include $(CLEAR_VARS)

LOCAL_MODULE := sprd_charger
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
    charger/charger.cpp \
    charger/healthd_mode_charger.cpp \
    charger/AnimationParser.cpp \
    charger/healthd_draw.cpp

LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)

LOCAL_C_INCLUDES := \
    bootable/recovery \
    $(LOCAL_PATH)/charger \
    system/core/healthd/include

LOCAL_CFLAGS := -Werror

# healthd_draw runtime layout configuration
ifneq ($(TARGET_HEALTHD_DRAW_SPLIT_SCREEN),)
LOCAL_CFLAGS += -DHEALTHD_DRAW_SPLIT_SCREEN=$(TARGET_HEALTHD_DRAW_SPLIT_SCREEN)
else
LOCAL_CFLAGS += -DHEALTHD_DRAW_SPLIT_SCREEN=0
endif

ifneq ($(TARGET_HEALTHD_DRAW_SPLIT_OFFSET),)
LOCAL_CFLAGS += -DHEALTHD_DRAW_SPLIT_OFFSET=$(TARGET_HEALTHD_DRAW_SPLIT_OFFSET)
else
LOCAL_CFLAGS += -DHEALTHD_DRAW_SPLIT_OFFSET=0
endif

ifeq ($(strip $(BOARD_CHARGER_DISABLE_INIT_BLANK)),true)
LOCAL_CFLAGS += -DCHARGER_DISABLE_INIT_BLANK
endif

ifeq ($(strip $(BOARD_CHARGER_ENABLE_SUSPEND)),true)
LOCAL_CFLAGS += -DCHARGER_ENABLE_SUSPEND
endif

LOCAL_STATIC_LIBRARIES := \
    android.hardware.health@2.0-impl \
    android.hardware.health@2.0 \
    android.hardware.health@1.0 \
    android.hardware.health@1.0-convert \
    libhidltransport \
    libhidlbase \
    libhwbinder_noltopgo \
    libhealthstoragedefault \
    libvndksupport \
    libbatterymonitor \
    libminui \
    libpng \
    libz \
    libbase \
    libutils \
    libcutils \
    liblog \
    libm \
    libc

ifeq ($(strip $(BOARD_CHARGER_ENABLE_SUSPEND)),true)
LOCAL_STATIC_LIBRARIES += libsuspend
endif

LOCAL_HAL_STATIC_LIBRARIES := libhealthd

include $(BUILD_EXECUTABLE)
