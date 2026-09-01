LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_USES_SPRD_LEGACY_CAMERA_WRAPPER),true)

include $(CLEAR_VARS)

LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_SRC_FILES := CameraWrapper.cpp

LOCAL_C_INCLUDES := \
    system/media/camera/include

LOCAL_SHARED_LIBRARIES := \
    libhardware \
    liblog \
    libutils

LOCAL_CPPFLAGS := \
    -Wall \
    -Wextra \
    -Wno-unused-parameter

LOCAL_MULTILIB := 32
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

endif
