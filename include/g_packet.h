
#ifndef H_G_PACKET
#define H_G_PACKET

void init_packets(void);
int new_packet(int type, int player_index, al_fixed x, al_fixed y);
void run_packets(void);

#endif
