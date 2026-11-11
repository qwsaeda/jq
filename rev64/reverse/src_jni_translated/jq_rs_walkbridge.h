#ifndef JQ_RS_WALKBRIDGE_H
#define JQ_RS_WALKBRIDGE_H
int  jq_walk_bridge_run(void);
int  jq_walk_bridge_reset(void);  /* §124y-26 强制重置开局（game_test 独立开局用） */
int  jq_walk_bridge_flip(int cell);  /* §124y-26 走子桥翻子入口（game_test 用） */
int  jq_walk_bridge_touch(float px, float py, int view_w, int view_h);
int  jq_walk_bridge_move(int from, int to);
void jq_walk_bridge_tick(void);
int  jq_game_test_finish(void);   /* §124y-26 端到端终局 */
#endif
