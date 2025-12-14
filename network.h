#ifndef NETWORK_H
#define NETWORK_H
#include "globals.h"

// Define si somos Cliente o Servidor
typedef enum { NET_NONE, NET_CLIENT, NET_SERVER } NetworkRole;

void InitNetwork(NetworkRole role, const char* ip);
void SendPlayerState(Player* p);
void ReceiveWorldState(); // Actualiza enemigos y otros jugadores

#endif