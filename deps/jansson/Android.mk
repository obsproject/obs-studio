LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
    src/dump.c \
    src/error.c \
    src/hashtable.c \
    src/hashtable_seed.c \
    src/load.c \
    src/memory.c \
    src/pack_unpack.c \
    src/strbuffer.c \
    src/strconv.c \
    src/utf.c \
    src/value.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/android \
        $(LOCAL_PATH)/src

LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libc
LOCAL_CFLAGS += -O3 -DHAVE_STDINT_H=1

LOCAL_MODULE:= libjansson

include $(BUILD_SHARED_LIBRARY)
