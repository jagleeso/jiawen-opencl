LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifndef $(PROGRAM)
	PROGRAM := opencl_aes
endif
LOCAL_MODULE := $(PROGRAM) 
# LOCAL_MODULE := opencl_aes

ifeq ($(PROGRAM),opencl_aes)
	LOCAL_SRC_FILES := aopencl.c opencl_aes.c
else
ifeq ($(PROGRAM),cpu-aes)
	LOCAL_SRC_FILES := cpu-aes.c
else
ifeq ($(PROGRAM),clinfo)
	LOCAL_SRC_FILES := clinfo.c
endif
endif
endif

LOCAL_C_INCLUDES := $(LOCAL_PATH)/openssl
LOCAL_LDLIBS := -ldl -L$(LOCAL_PATH)/lib -lcrypto
LOCAL_CFLAGS := -g

LOCAL_EXPORT_CFLAGS += -g

LOCAL_CXXFLAGS += -fno-exceptions
#include $(BUILD_SHARED_LIBRARY)  
include $(BUILD_EXECUTABLE)
