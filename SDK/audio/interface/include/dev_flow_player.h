#ifndef DEV_FLOW_PLAYER_H
#define DEV_FLOW_PLAYER_H



int dev_flow_player_open(u8 ch_num, u16 source_uuid);

void dev_flow_player_close();

bool dev_flow_player_runing();



#endif
