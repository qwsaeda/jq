# Makefile — 逆源码重写（src_rs）构建
#   make desk   → dist/jq_run（x86_64 桌面 headless，渲染走 render_stub.c）
#   make so     → dist/libjunqi_rs.so（NDK arm64，14 On* JNI + 真渲染 render.c）
# ★ 包名 com.cnvcs.junqirs（区别于原版 com.cnvcs.junqi / 逆向 com.cnvcs.junqi32，同设备独立共存）
# ★ 版本号 1.6.8/168（正向开发标识；打包源 /tmp/opencode/jq_apk/rs_a64/dec_rs/）
CC ?= gcc
CFLAGS ?= -O2 -Wall -I.

SRCS_CORE := board.c layout.c rules.c save.c bt_protocol.c

all: desk

# 桌面可执行（headless，无 EGL/GL；RS_NO_GL 桩掉 GL 类型）
desk: dist/jq_run
dist/jq_run: $(SRCS_CORE) main.c junqi.h bt_protocol.h render.h render_stub.c
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o $@ $(SRCS_CORE) main.c render_stub.c ui_state_stub.c ui_menu_stub.c
	@echo "构建完成: $@"

# NDK arm64 共享库（真渲染：EGL + GL 状态机 + SPAK 纹理管线）
so: dist/libjunqi_rs.so

dist/libjunqi_rs.so: $(SRCS_CORE) jni_bridge.c bt_session.c junqi.h bt_protocol.h bt_session.h render.h gl2_probe_gl.h render.c render_obj.c containers.c render_scene.c glview_gl.c touch.c touch_state.c png_decode.c
	@mkdir -p dist
	$(NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang \
		$(CFLAGS) -shared -fPIC -o $@ $(SRCS_CORE) jni_bridge.c bt_session.c render.c render_obj.c containers.c render_scene.c glview_gl.c touch.c touch_state.c ui_menu.c ui_state.c png_decode.c gl2_probe_gl.c -lm \
		-llog -lz -lEGL -lGLESv1_CM -lGLESv2 -landroid
	@echo "构建完成: $@"

# 一致测试（重写侧规则引擎基准序列）：./dist/jq_consistency selftest
# ★ P1（datah_012）：rules.c 依赖 ui_state 的 4 规则开关（ui_rule_cfg_get，默认全开），
#   桌面一致性构建链接 ui_state_stub.c（桩默认全开，行为不变）。
consistency: dist/jq_consistency
dist/jq_consistency: $(SRCS_CORE) consistency_desktop.c junqi.h
	$(CC) $(CFLAGS) -DRS_NO_GL -o $@ $(SRCS_CORE) consistency_desktop.c ui_state_stub.c -lm

clean:
	rm -rf dist

.PHONY: all desk so consistency clean

.PHONY: bttest
bttest:
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_bttest \
	    $(SRCS_CORE) bt_session.c bt_selftest.c -lm
	@echo "构建完成: dist/jq_bttest"
	@./dist/jq_bttest

.PHONY: laytest
LAYDIR ?= /disk/workspaces/jq/rev64/dist/layouts
laytest:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DLAY_DIR='"$(LAYDIR)"' \
	    -o dist/jq_laytest board.c layout.c rules.c save.c lay_selftest.c -lm
	@echo "构建完成: dist/jq_laytest"
	@dist/jq_laytest

.PHONY: aittest
aittest:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_aitest \
	    ai_selftest.c rules.c board.c layout.c -lm
	@echo "构建完成: dist/jq_aitest"
	@./dist/jq_aitest

.PHONY: touchtest
touchtest:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_touchtest \
	    board.c layout.c rules.c save.c touch.c touch_state.c touch_selftest.c -lm
	@echo "构建完成: dist/jq_touchtest"
	@dist/jq_touchtest

# 桌面预验证走子序列（批量，免真机盲目试探）：
#   make walkverify SEQ="25,30 27,32"
.PHONY: walkverify
walkverify:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_walkverify \
	    board.c layout.c rules.c save.c walkverify.c -lm
	@echo "构建完成: dist/jq_walkverify"
	@if [ -n "$(LONG)" ]; then dist/jq_walkverify --longrun $(LONG); else dist/jq_walkverify "$(SEQ)"; fi

# 桌面事件验证（EAT/BOMB/MOVE/FLAG/MINE + 布局打印，替代手写 /tmp/*.c）：
#   make evverify [MODE=--all|--flag|--mine|--eat|--bomb|--move|--lay]
# ★ P1（datah_012）：rules.c 依赖 ui_state_stub 的 4 规则开关桩（默认全开，行为不变）
.PHONY: evverify
evverify:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_evverify \
	    board.c layout.c rules.c save.c evverify.c ui_state_stub.c -lm
	@echo "构建完成: dist/jq_evverify"
	@dist/jq_evverify $(MODE)


gametest:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_gametest \
	    board.c layout.c rules.c save.c touch.c touch_state.c game_selftest.c -lm
	@echo "构建完成: dist/jq_gametest"
	@dist/jq_gametest
