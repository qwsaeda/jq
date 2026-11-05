#!/bin/bash
# gen_c_videos.sh — 生成课程 C.4/C.5 的 4 段方法论配图视频（meta-loop I4）
# 用法：bash /tmp/opencode/gen_c_videos.sh
# 说明：4 段 text 模式串行生成（防并发限流 + 防 timeout），每段 ~10s 视频
#       prompt 用单引号独立成段，避免 heredoc/嵌套引号报 unexpected EOF（N38b）
set -u
export AGNES_API_KEY=$(grep -o 'AGNES_API_KEY=.*' /disk/workspaces/treasureIsl/.env | cut -d= -f2)
V=/disk/workspaces/treasureIsl/scripts/agnes_video.py
O=/disk/workspaces/jq/course_unidbg/videos
log() { echo "[gen_c_videos] $*"; }

VIDFILE=/disk/workspaces/jq/rev64/dist/video_ids.txt
retry429() {
  local name=$1; shift
  # 从调用日志抓已创建的 video_id（防限流后任务悬空）
  local vid=$(grep -o "video_id=[a-zA-Z0-9_-]*" /disk/workspaces/jq/rev64/dist/videos_run.log 2>/dev/null | tail -1 | cut -d= -f2)
  if [ -n "$vid" ]; then echo "$name $vid" >> $VIDFILE; log "  ★ video_id 已记录: $vid（下轮 --video-id $vid 续下载）"; fi
  log "  ★ 429 限流，sleep 180s 后重试 1 次"
  sleep 180
  if python3 "$V" "$@"; then
    log "  ✓ $name 重试成功"
  else
    log "  ★ $name 重试仍失败（429 持续/网络），video_id 已记录可下轮 --video-id 续"
  fi
}

run() {
  local name=$1; shift
  log "=== $name ==="
  if python3 "$V" "$@"; then
    log "  ✓ $name 完成"
    return 0
  fi
  # 检测是否 429
  if tail -5 /disk/workspaces/jq/rev64/dist/videos_run.log | grep -q "429"; then
    retry429 "$name" "$@"
  else
    log "  ★ $name 失败（非 429，网络/参数，下轮重试）"
  fi
}

run "video1 C.4 合并边界调用链统计" \
  --prompt '信息图动画，深色背景扁平技术风格。标题"调用链对齐统计：合并边界4层"。金字塔4层依次高亮：①全库定义1630（完整1334+存根296）②已翻译函数内调用边3698条 ③目标命中97.5%完整/2.4%存根/0.2%缺失=5个函数 ④渲染关键路径缺口=0。左侧"边界=seed∪完整∪存根"右侧"存根=匿名参数return 0"。底部"调用链缺口极小≠能跑（对象图数据未填充）"。中文，平滑高亮过渡，矢量风格。' \
  --out $O/c4_merged_boundary_stats.mp4 --seconds 10 --ratio 16:9

run "video2 八件套三级验证链" \
  --prompt '信息图动画，深色背景扁平技术风格。标题"军棋八件套三级验证链"。横向三级卡片：①桌面预验证（walkverify走子+evverify 8断言，免真机flaky）箭头②真机单点（mumu_walktest触摸+mumu_evtest setprop夹具，多轮重试兜底）箭头③真机全套批量（mumu_fulltest 7/7 走子EAT+FLAG+MINE+重开+和棋+认输+困毙一次跑完）。卡片间箭头连接，顶部"先桌面→再真机单点→后真机全套（防盲目试探）"。中文，平滑过渡，矢量风格。' \
  --out $O/c5_eight_tools_verify_chain.mp4 --seconds 10 --ratio 16:9

run "video3 7类事件闭环+游戏内重开" \
  --prompt '信息图动画，深色背景扁平技术风格。标题"军棋事件闭环+游戏内重开"。环形7个事件徽章：①MOVE空移②EAT吃子③BOMB炸弹④连续多步⑤重进不崩⑥FLAG吃旗胜⑦MINE工兵挖雷，每徽章标触发方式（触摸走子/setprop夹具）。底部闭环链"对局结束FLAG→OVER→游戏内重开jq.restartgame property→OnUpdate每60帧读→default.lay新开局stage=PLAY蓝先手"，循环箭头。中文，事件徽章对应颜色，平滑过渡。' \
  --out $O/c5_seven_events_restart.mp4 --seconds 10 --ratio 16:9

run "video4 对局结束UI（win/lose/draw三色）" \
  --prompt '信息图动画，深色背景扁平技术风格。标题"军棋对局结束UI闭环"。左右两栏：左"音效闭环（已真机）"喇叭图标，sfx_game_over统一入口3处调用→stage==OVER自动播win/lose/draw，标注"真机15120/18636出声闭环"；右"渲染闭环（调用链已触发）"横幅图标，rs_draw_overlay画中心胜负横幅（蓝胜BLUE/红胜RED/和棋灰），标注"渲染调用链日志实证"和"像素出屏需真机GPU（MuMu GLES1受限）"。中间箭头从"FLAG ev=3 stage=OVER winner=BLUE"触发。底部"音效win/lose/draw真机闭环≠横幅画到屏幕"。中文，蓝红灰配色，平滑过渡。' \
  --out $O/c5_game_over_ui.mp4 --seconds 10 --ratio 16:9

log "=== 4 段视频批次结束（成功段已落 $O，失败段下轮 --video-id 续或重试）==="
exit 0
