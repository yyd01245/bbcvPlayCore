
# Copyright (C) 2009 The Android Open Source Project
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
SDL_C_INCLUDES := /home/yyd/android_test/bbcvPlayerCore/jni/sdl-lib

LOCAL_MODULE := sdl
LOCAL_SRC_FILES := $(SDL_C_INCLUDES)/arm/libSDL2.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
FFMPEG_C_INCLUDES := /home/yyd/android_test/bbcvPlayerCore/jni/ffmpeg-lib

LOCAL_MODULE := ffmpeg
LOCAL_SRC_FILES := $(FFMPEG_C_INCLUDES)/arm/libffmpeg.so
include $(PREBUTIL_SHARED_LIBRARY)



include $(CLEAR_VARS)
FFMPEG_C_INCLUDES := /home/yyd/android_test/bbcvPlayerCore/jni/ffmpeg-lib
SDL_C_INCLUDES := /home/yyd/android_test/bbcvPlayerCore/jni/sdl-lib

LOCAL_C_INCLUDES += $(LOCAL_PATH) \
                $(FFMPEG_C_INCLUDES)/include \
                $(SDL_C_INCLUDES)/include

SRC_PATH:=$(LOCAL_PATH)/src/bbcvplayercore
#SRC_SDLPATH:=$(LOCAL_PATH)/src/bbcvRender
SRC_SDLPATH:=$(LOCAL_PATH)/src/bbcvplayercore
LOCAL_MODULE    := bbcvPlayCore
LOCAL_SRC_FILES := $(SRC_PATH)/Decoder.cpp \
		$(SRC_PATH)/TSStreamInfo.cpp \
		$(SRC_PATH)/RecvQueue.cpp   \
		$(SRC_SDLPATH)/bbcvrender.cpp \
		$(SRC_PATH)/libcore.cpp  \
		$(SRC_PATH)/test.cpp  \
		$(SRC_PATH)/SDL_android_main.c

#dynamic lib need
#LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
#LOCAL_STATIC_LIBRARIES:=/home/yyd/android_test/JniTest/app/jni_test/testJni//obj/local/armeabi-v7a/libtest.a
#LOCAL_STATIC_LIBRARIES:=test
#LOCAL_LDFLAGS +=$(STATIC_LIB_PATH)/libtest.a
LOCAL_LDLIBS := -lz -lGLESv1_CM -lGLESv2 -landroid  
LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog -lz
LOCAL_SHARED_LIBRARIES :=  sdl

#LOCAL_STATIC_LIBRARIES :=  avformat avcodec avutil swresample swscale sdl2
LOCAL_LDFLAGS +=  $(FFMPEG_C_INCLUDES)/arm/libffmpeg.so 
#			$(SDL_C_INCLUDES)/arm/libSDL2.so

#LOCAL_LDLIBS:=

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
