#ifndef __POTATO_H__
#define __POTATO_H__
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/select.h>
#include <sys/resource.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>


struct potato_tag{
    bool isPotato;
    int players;
    int hops;
    int cnt;
    int player_id[512];
};typedef struct potato_tag potato_t;


void mkpotato(int p, int h, potato_t & potato){
    potato.isPotato = true;
    potato.players = p;
    potato.hops = h;
    potato.cnt = 0;
    for(int i = 0; i < 512; ++i){
        potato.player_id[i] = 0;
    }
}

void transfer_potato(potato_t & potato, int playerid){
    if(potato.hops <= 0 || potato.cnt >= 512){return;}
    potato.player_id[potato.cnt] = playerid;
    potato.hops--;
    potato.cnt++;
}

#endif