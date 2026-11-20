# Makefile — 逆源码重写（src_rs）构建
#   make desk   → dist/jq_run（x86_64 桌面 headless，渲染走 render_stub.c）
#   make so     → dist/libjunqi_rs.so（NDK arm64，14 On* JNI + 真渲染 render.c）
# ★ 包名 com.cnvcs.junqirs（区别于原版 com.cnvcs.junqi / 逆向 com.cnvcs.junqi32，同设备独立共存）
# ★ 版本号 1.6.8/168（正向开发标识；打包源 /tmp/opencode/jq_apk/rs_a64/dec_rs/）
CC ?= gcc
CFLAGS ?= -O2 -Wall -I.

SRCS_CORE := board.c layout.c rules.c save.c bt_protocol.c replay.c

all: desk

# 桌面可执行（headless，无 EGL/GL；RS_NO_GL 桩掉 GL 类型）
desk: dist/jq_run
dist/jq_run: $(SRCS_CORE) main.c junqi.h bt_protocol.h render.h render_stub.c
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o $@ $(SRCS_CORE) main.c render_stub.c ui_state_stub.c ui_menu_stub.c
	@echo "构建完成: $@"

# NDK arm64 共享库（真渲染：EGL + GL 状态机 + SPAK 纹理管线）
# ★ 2026-11-12 官方 Android 15 标准（targetSdk 35）：16KB 内存页对齐。
#   Android 15 起 16KB 页大小设备（Pixel 等）上，未做 16KB 对齐的 .so 无法
#   mmap 加载（dlopen failed）。官方方案 = 链接器 -Wl,-z,max-page-size=16384
#   （让 LOAD 段对齐 16KB，同时 4KB 页设备兼容——ELF p_align=0x4000 在 4KB
#   内核下自动按 4KB 页对齐，无副作用）。NDK r25b clang 支持（无需 r28）。
#   验证判据：readelf -lW 的 LOAD 段 Align=0x4000（旧 0x1000）。
so: dist/libjunqi_rs.so

dist/libjunqi_rs.so: $(SRCS_CORE) jni_bridge.c bt_session.c jq_event_names_73.c evt55_dispatch.c evt55_business.c evt55_sfx.c junqi.h bt_protocol.h bt_session.h render.h gl2_probe_gl.h render.c render_obj.c containers.c render_scene.c glview_gl.c touch.c touch_state.c png_decode.c img_decode.c jq_event_names_73.h
	@mkdir -p dist
	$(NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang \
		$(CFLAGS) -shared -fPIC -o $@ $(SRCS_CORE) jni_bridge.c bt_session.c jq_event_names_73.c evt55_dispatch.c evt55_business.c evt55_sfx.c render.c render_obj.c containers.c render_scene.c glview_gl.c touch.c touch_state.c ui_menu.c ui_state.c png_decode.c img_decode.c gl2_probe_gl.c -lm \
		-llog -lEGL -lGLESv1_CM -lGLESv2 -landroid -ldl \
		-Wl,-z,max-page-size=16384
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

# ★ AUD-1000 B08-037/B08-039（P1 已修，基准新鲜度门禁）：benchcheck 持续门禁——
#   jq_consistency test_move_seq.txt 实际输出 vs test_expected.txt 文件基准 diff。
#   旧版无此门禁：consistency_desktop selftest 只比内置 BENCH_EXPECT（双源，
#   改 test_expected.txt 忘改内置 BENCH_EXPECT 或反之 → selftest PASS 但文件
#   基准过期假绿，B07-041/042 连锁）。现纳入 src_rs_regress（[8.5/12] 位置，
#   rules.c 任何改动后强制基准新鲜度校验，过期即 FAIL 非静默）。
.PHONY: benchcheck
benchcheck: consistency
	@mkdir -p dist
	@./dist/jq_consistency test_move_seq.txt > .bench_actual.txt
	@if cmp -s .bench_actual.txt test_expected.txt; then \
	    echo "  ✓ benchcheck：jq_consistency test_move_seq.txt 输出 == test_expected.txt（基准新鲜）"; \
	else \
	    echo "  ★ benchcheck：test_expected.txt 基准过期（jq_consistency 输出已变）"; \
	    echo "    修法：先确认 rules.c 改动行为是否符合预期，符合则 "; \
	    echo "    ./dist/jq_consistency test_move_seq.txt > test_expected.txt（人工确认后更新基准）"; \
	    diff .bench_actual.txt test_expected.txt | head -20; \
	    rm -f .bench_actual.txt; exit 1; \
	fi
	# ★ R7-D-18（P1 双源同步断言）：test_expected.txt 文件基准 vs consistency_desktop.c
	#   内嵌 BENCH_EXPECT 双源——旧实现只比"实际输出 vs 文件基准"，内嵌 BENCH_EXPECT
	#   过期（改 test_expected.txt 忘改内置表）selftest 恒 PASS 假绿。修：benchcheck
	#   加第三步 = 用 jq_consistency 内置自测序列（无参跑 BENCH_EXPECT 比对）+ 显式
	#   抽取 C 表与文件基准 cmp（grep -A 抽取 BENCH_EXPECT 数组体到临时文件）。
	@python3 $(CURDIR)/bench_c_extract.py
	@if cmp -s .bench_c_expect.txt test_expected.txt; then \
	    echo "  ✓ benchcheck 双源：BENCH_EXPECT（C 内嵌表）== test_expected.txt（文件基准）"; \
	    rm -f .bench_c_expect.txt; \
	else \
	    echo "  ★ benchcheck 双源漂移：BENCH_EXPECT（consistency_desktop.c）≠ test_expected.txt"; \
	    echo "    修法：两侧同步更新（jq_consistency 输出变化 → 同时刷新文件基准与 C 内嵌表）"; \
	    diff .bench_c_expect.txt test_expected.txt | head -20; \
	    rm -f .bench_c_expect.txt; exit 1; \
	fi
	@rm -f .bench_actual.txt

.PHONY: bttest
bttest:
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_bttest \
	    $(SRCS_CORE) bt_session.c bt_selftest.c ui_state_stub.c -lm
	@echo "构建完成: dist/jq_bttest"
	@./dist/jq_bttest

# ★ 规则行为探针（32/64 位对比用，§124y R-138 实证专项）：打印 RULE-PROBE 行
.PHONY: ruleprobe
ruleprobe:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_ruleprobe \
	    $(SRCS_CORE) rule_probe.c rule_probe_main.c ui_state_stub.c -lm
	@echo "构建完成: dist/jq_ruleprobe"
	@./dist/jq_ruleprobe

.PHONY: laytest
LAYDIR ?= /disk/workspaces/jq/rev64/dist/layouts
laytest:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DLAY_DIR='"$(LAYDIR)"' \
	    -o dist/jq_laytest board.c layout.c rules.c save.c lay_selftest.c ui_state_stub.c -lm
	@echo "构建完成: dist/jq_laytest"
	@dist/jq_laytest

.PHONY: aittest
aittest:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_aitest \
	    ai_selftest.c rules.c board.c layout.c ui_state_stub.c -lm
	@echo "构建完成: dist/jq_aitest"
	@./dist/jq_aitest

.PHONY: touchtest
touchtest:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_touchtest \
	    board.c layout.c rules.c save.c touch.c touch_state.c touch_selftest.c ui_state_stub.c -lm
	@echo "构建完成: dist/jq_touchtest"
	@dist/jq_touchtest

# 桌面预验证走子序列（批量，免真机盲目试探）：
#   make walkverify SEQ="25,30 27,32"
.PHONY: walkverify
walkverify:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_walkverify \
	    board.c layout.c rules.c save.c walkverify.c ui_state_stub.c -lm
	@echo "构建完成: dist/jq_walkverify"
	@if [ -n "$(LONG)" ]; then dist/jq_walkverify --longrun $(LONG); else dist/jq_walkverify "$(SEQ)"; fi

# 桌面事件验证（EAT/BOMB/MOVE/FLAG/MINE + 布局打印，替代手写 /tmp/*.c）：
#   make evverify [MODE=--all|--flag|--mine|--eat|--bomb|--move|--lay]
# ★ P1（datah_012）：rules.c 依赖 ui_state_stub 的 4 规则开关桩（默认全开，行为不变）
# ★ P1（datah_016/017/020）：布局存档 3 弹窗桌面验证（run_layout_save 10/10）
# ★ P1（datah_038）：登录弹窗 6 控件桌面验证（run_login，ui_menu_stub 几何表桩）
.PHONY: rulestest
# ★ AUD-R-135（P3 落地）：规则引擎全量用例（表驱动 13 族，对齐 evverify 基准 + 暗棋族
#   + 回合门 + 困毙 + 铁路方向）——make rulestest 一键构建 + 跑（dist/jq_rulestest）
rulestest:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_rulestest \
	    board.c layout.c rules.c save.c rulestest.c ui_state_stub.c -lm
	@echo "构建完成: dist/jq_rulestest"
	@./dist/jq_rulestest

.PHONY: bt_semtest
bt_semtest:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_bt_semtest \
	    $(SRCS_CORE) bt_session.c bt_semtest.c ui_state_stub.c -lm
	@echo "构建完成: dist/jq_bt_semtest"
	@./dist/jq_bt_semtest

# ★ R12 混跑矩阵双实例对射（v3/v2 能力混跑 + seq 去重 + UNDO 全链，桌面纯 bt_session/bt_protocol）
.PHONY: bt_semtest_dual
bt_semtest_dual:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_bt_semtest_dual \
	    $(SRCS_CORE) bt_session.c bt_semtest_dual.c ui_state_stub.c -lm
	@echo "构建完成: dist/jq_bt_semtest_dual"
	@./dist/jq_bt_semtest_dual

.PHONY: evverify
MODE ?= --all
evverify:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_evverify \
	    board.c layout.c rules.c save.c replay.c evverify.c ui_state_stub.c ui_menu_stub.c \
	    evt55_business.c evt55_dispatch.c jq_event_names_73.c evt55_stub.c -lm
	# ★ R27-Q67/Q68：evt55 全链入桌面链接（73 名逐名分发/计数断言可调用，含计数桩）
	@echo "构建完成: dist/jq_evverify"
	@dist/jq_evverify $(MODE)


gametest:
	@mkdir -p dist
	$(CC) $(CFLAGS) -DRS_NO_GL -o dist/jq_gametest \
	    board.c layout.c rules.c save.c touch.c touch_state.c game_selftest.c ui_state_stub.c -lm
	@echo "构建完成: dist/jq_gametest"
	@dist/jq_gametest

# ★ 渗透轮 F1-F4 fuzz 探针（ASan 默认开防假绿：崩溃/越界内存必报非 0）
# 用法：make fuzzprobe SEED=123 GAMES=500   或   dist/jq_fuzzprobe [seed games touchN eventN]
fuzzprobe:
	@mkdir -p dist
	$(CC) $(CFLAGS) -O0 -g -fsanitize=address -DRS_NO_GL -o dist/jq_fuzzprobe \
	    board.c layout.c rules.c save.c touch.c touch_state.c fuzz_probe.c ui_state_stub.c \
	    jq_event_names_73.c evt55_stub.c -lm
	@echo "构建完成: dist/jq_fuzzprobe（ASan）"
	@dist/jq_fuzzprobe $(SEED) $(GAMES)

# ★ P1 渗透面：PNG 解码器畸形输入 fuzz（ASan；链接 png_decode.c + zlib）
#   变异：位翻转 / 截断 / 定向畸形（clen 巨值、IHDR 超长、w/h 巨值、bitdepth/colortype 非法、
#   interlace=1、重复 IHDR、无 IEND、签名错、空文件、NULL、IDAT 巨量）/ 结构错乱（拼接/前置垃圾）
#   判据：0 崩溃 + 拒收语义 + 输出不变量（w*h*4）+ 无泄漏（ASan LSAN）
# 用法：make fuzzpng SEED=7 ROUNDS=3000
fuzzpng:
	@mkdir -p dist
	$(CC) $(CFLAGS) -O0 -g -fsanitize=address -DRS_NO_GL -Iandroid_stub \
	    -o dist/jq_fuzzpng png_decode.c fuzz_png.c -lz -lm
	@echo "构建完成: dist/jq_fuzzpng（ASan）"
	@dist/jq_fuzzpng $(SEED) $(ROUNDS)

# ★ P2 渗透面：UI 状态机随机点击序列 fuzz（ASan；ui_state + ui_menu + 依赖）
#   变异：随机点击序列 / 边界点击（面板四角、0,0、719,1279）/ 全 169 屏态自锁扫描
#   判据：0 崩溃 + 屏态合法域 [0,UI_SCREEN_MAX] + 弹窗逃逸 + 棋盘☰回菜单无死锁
# 用法：make fuzzui SEED=7 STEPS=2000
fuzzui:
	@mkdir -p dist
	$(CC) $(CFLAGS) -O0 -g -fsanitize=address -DRS_NO_GL -Iandroid_stub \
	    -o dist/jq_fuzzui fuzz_ui.c ui_state.c ui_menu.c replay.c rules.c board.c layout.c save.c -lm
	@echo "构建完成: dist/jq_fuzzui（ASan）"
	@dist/jq_fuzzui $(SEED) $(STEPS)

# ★ P5' 渗透面：JNI 入口参数 fuzz（ASan 桌面 + mock JNIEnv + fork 隔离）
#   14 入口 × 4 毒化档（正常/全NULL/垃圾/异常）× 6 变体（env NULL/clazz NULL/参数NULL/
#   env 垃圾/超长串+极值int）= 336 调用组合
#   判据：0 整体崩溃（fork 隔离捕崩溃/hang）+ 防御缺口清单定位
# ★ 依赖 android_stub/（桌面 jni.h / EGL / android/*.h 最小桩）+ platform_stub.c（GL/平台符号）
# 用法：make fuzzjni SEED=7 ROUNDS=1
FUZZJNI_SRCS = fuzz_jni.c jni_bridge.c platform_stub.c board.c layout.c rules.c save.c \
	touch.c touch_state.c replay.c ui_state.c ui_menu.c containers.c \
	jq_event_names_73.c evt55_dispatch.c evt55_business.c evt55_sfx.c \
	bt_protocol.c bt_session.c audio.c
fuzzjni:
	@mkdir -p dist
	$(CC) $(CFLAGS) -O0 -g -fsanitize=address -DRS_NO_GL -Iandroid_stub \
	    -o dist/jq_fuzzjni $(FUZZJNI_SRCS) -lm
	@echo "构建完成: dist/jq_fuzzjni（ASan + mock JNIEnv + fork 隔离）"
	@dist/jq_fuzzjni $(SEED) $(ROUNDS)
