LOCAL_PATH := jni
include common.mk

# all:
# 	ndk-build
# 
# cpu-aes:
# 	ndk-build PROGRAM=cpu-aes
# example-aes:
# 	ndk-build PROGRAM=example-aes

# INCLUDES := $(addprefix -I,$(LOCAL_C_INCLUDES))
INCLUDES := $(addprefix -I,jni)
LIBS := -ldl $(LOCAL_LIBRARIES) -lrt
CFLAGS := $(LOCAL_CFLAGS) $(LOCAL_CXXFLAGS)
# LOCAL_EXPORT_CFLAGS += -g
# LOCAL_CXXFLAGS += -fno-exceptions

SOURCES := $(addprefix jni/,$(LOCAL_SRC_FILES))
OBJS := $(SOURCES:.c=.o)

# SOURCES := t.cpp
# Objs are all the sources, with .cpp replaced by .o
# OBJS := $(SOURCES:.cpp=.o)

all: $(PROGRAM) 

# Compile the binary 't' by calling the compiler with cflags, lflags, and any libs (if defined) and the list of objects.
$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LFLAGS) $(LIBS)

# Get a .o from a .cpp by calling compiler with cflags and includes (if defined)
# .c.o:
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
