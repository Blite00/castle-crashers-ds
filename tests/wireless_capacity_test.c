#include <stdio.h>
#include "protocol.h"

/* Verified against this project's installed BlocksDS DSWiFi library:
   Wifi_MultiplayerHostMode rejects host_data + 44 > 320, and
   client_data + 29 > 256. These limits exclude application headers only
   insofar as they are already part of our InputPacket/StatePacket sizes. */
_Static_assert(sizeof(StatePacket) <= 276, "State exceeds native DS CMD capacity");
_Static_assert(sizeof(InputPacket) <= 227, "Input exceeds native DS REPLY capacity");
int main(void){
    printf("PASS: native DS payload sizes: CMD %zu/276 bytes; REPLY %zu/227 bytes\n",
           sizeof(StatePacket),sizeof(InputPacket));
    return 0;
}
