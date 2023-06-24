#include "rawsocket.h"
#include "handler.h"

#include <unistd.h>
#include <stdio.h>


int main(int argc, char const *argv[])
{
    int socket = ConexaoRawSocket("lo");
    struct t_packet packet;
    createPacket(&packet, 21, 0, BACK_1_FILE, "STARS WHEN YOU SHINE!");
    write(socket, &packet, sizeof(packet));
    return 0;
}
