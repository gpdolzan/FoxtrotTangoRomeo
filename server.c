#include "rawsocket.h"
#include "packethandler.h"

int main(int argc, char const *argv[])
{
    int socket = ConexaoRawSocket("eno1");
    struct t_packet packet;
    struct t_packet sPacket;

    while(1)
    {
        readPacket(socket, &packet, 0);
        if(packet.tipo == BACK_1_FILE)
        {
            if(packet.sequencia == 0) // Inicio de uma sequencia de pacotes
            {
                // Check parity
                if(checkParity(&packet) == 1)
                {
                    printf("Erro de paridade\n");
                    // Send NACK
                    createPacket(&sPacket, 0, 0, NACK, NULL);
                    sendPacket(socket, &sPacket);
                }
                else
                {
                    printf("RECEIVING\n");
                    receiveFile(socket, &packet);
                    printf("I FINISHED\n");
                }
            }
        }
    }
}