#ifndef JQ_RS_WALKBRIDGE_H
#define JQ_RS_WALKBRIDGE_H
int  jq_walk_bridge_run(void);
int  jq_walk_bridge_touch(float px, float py, int view_w, int view_h);
int  jq_walk_bridge_move(int from, int to);
void jq_walk_bridge_tick(void);
int  jq_game_test_finish(void);   /* §124y-26 端到端终局 */
#endif
