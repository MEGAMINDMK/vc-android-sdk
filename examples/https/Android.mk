LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp .cc
LOCAL_CPPFLAGS += -fexceptions -std=c++17
LOCAL_MODULE := https

# Source files
LOCAL_SRC_FILES := \
	main.cpp \
	updater.cpp \
	linkopener.cpp \
	https.cpp \
	mod/logger.cpp \
	mod/config.cpp \
	includes/scripting.cpp \
	includes/armhook.cpp \


# Flags
LOCAL_CFLAGS += -O2 -mfloat-abi=softfp -DNDEBUG -std=c++17

# Include paths
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/openssl \

# Linker flags
LOCAL_LDLIBS += -llog -lz -landroid -lGLESv2 -lEGL -lOpenSLES
LOCAL_STATIC_LIBRARIES := ssl_static crypto_static

include $(BUILD_SHARED_LIBRARY)
# OpenSSL static libraries
include $(CLEAR_VARS)
LOCAL_MODULE := crypto_static
LOCAL_SRC_FILES := openssl/libcrypto.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ssl_static
LOCAL_SRC_FILES := openssl/libssl.a
include $(PREBUILT_STATIC_LIBRARY)