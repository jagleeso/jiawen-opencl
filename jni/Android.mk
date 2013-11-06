LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := aopencl.c opencl_aes.c
LOCAL_MODULE := opencl_aes
LOCAL_C_INCLUDES := $(LOCAL_PATH)/openssl
LOCAL_LDLIBS := -ldl -L$(LOCAL_PATH)/lib -lcrypto
LOCAL_CFLAGS := -g

LOCAL_EXPORT_CFLAGS += -g

LOCAL_CXXFLAGS += -fno-exceptions
#include $(BUILD_SHARED_LIBRARY)  
include $(BUILD_EXECUTABLE)
