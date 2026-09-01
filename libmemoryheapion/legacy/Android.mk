LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_USES_SPRD_LEGACY_CAMERA_WRAPPER),true)

include $(CLEAR_VARS)

LOCAL_MODULE := libmemoryheapion_sprd_legacy

LOCAL_SRC_FILES := \
    MemoryHeapIon.cpp

LOCAL_ADDITIONAL_DEPENDENCIES += \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_C_INCLUDES += \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
    $(TOP)/hardware/sprd/kernel_headers/$(TARGET_BOARD_PLATFORM)

LOCAL_SHARED_LIBRARIES := \
    libbinder \
    liblog \
    libcutils \
    libutils

LOCAL_CPPFLAGS += \
    -Wno-error \
    -Wno-unused-parameter \
    -Wno-conversion

LOCAL_MULTILIB := 32
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

endif
