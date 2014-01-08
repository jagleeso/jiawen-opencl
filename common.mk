ifndef $(PROGRAM)
	PROGRAM := opencl_aes
endif
LOCAL_MODULE := $(PROGRAM) 
# LOCAL_MODULE := opencl_aes

COMMON_SRC_FILES := common.c
ifeq ($(PROGRAM),opencl_aes)
	LOCAL_SRC_FILES := aopencl.c opencl_aes.c $(COMMON_SRC_FILES)
else
ifeq ($(PROGRAM),cpu-aes)
	LOCAL_SRC_FILES := cpu-aes.c $(COMMON_SRC_FILES)
else
ifeq ($(PROGRAM),clinfo)
	LOCAL_SRC_FILES := clinfo.c
else
ifeq ($(PROGRAM),example-aes)
	LOCAL_SRC_FILES := example-aes.c $(COMMON_SRC_FILES)
else
ifeq ($(PROGRAM),coalesce)
	LOCAL_SRC_FILES := aopencl.c coalesce.c $(COMMON_SRC_FILES)
else
ifeq ($(PROGRAM),memcopy)
	LOCAL_SRC_FILES := aopencl.c memcopy.c $(COMMON_SRC_FILES)
endif
endif
endif
endif
endif
endif

LOCAL_C_INCLUDES := $(LOCAL_PATH)/openssl
LOCAL_LIBRARIES := -lcrypto 
LOCAL_LDLIBS := -ldl -L$(LOCAL_PATH)/lib $(LOCAL_LIBRARIES)
ifeq ($(PROGRAM),opencl_aes)
	LOCAL_CFLAGS := -g
else
	LOCAL_CFLAGS := -g -Wall -Werror
endif
LOCAL_EXPORT_CFLAGS += -g

LOCAL_CXXFLAGS += -fno-exceptions

