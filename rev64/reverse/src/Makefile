# Makefile — 手写路线 arm64 native 构建（R1-R4）
# 用法: make NDK=/tmp/opencode/android-ndk-r25b API=24
NDK  ?= /tmp/opencode/android-ndk-r25b
API  ?= 24
CC   := $(NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$(API)-clang
CFLAGS := -shared -fPIC -O2 -Wall -fvisibility=hidden -I src
LIBS := -llog -lGLESv1_CM -landroid -lm

SRCS := src/game_state.c src/game_logic.c src/junqi_jni.c src/render_gles1.c src/bt_spak.c
OBJS := $(SRCS:.c=.o)
OUT  := dist/libjunqi_arm64_hand.so

all: $(OUT)

$(OUT): $(SRCS) src/junqi_core.h
	$(CC) $(CFLAGS) $(SRCS) -o $(OUT) $(LIBS)
	@echo "构建完成: $(OUT)"
	@readelf --dyn-syms --wide $(OUT) | grep -c "Java_com" | xargs echo "JNI 导出数:"

clean:
	rm -f $(SRCS:.c=.o) $(OUT)

.PHONY: all clean
