#include "rawsocket.h"
#include "handler.h"

int main(int argc, char const *argv[])
{
    int socket = ConexaoRawSocket("lo");
    // Create buffer size of packet_t
    struct t_packet packet;
    struct t_packet *buffer = malloc(sizeof(struct t_packet));

    while(1)
    {
        read(socket, buffer, sizeof(struct t_packet));
        if(buffer->marcadorInicio == 0x7E)
        {
            memcpy(&packet, buffer, sizeof(struct t_packet));
            unmaskPacket(&packet);
            if(checkParity(&packet) == 0)
            {
                printPacket(&packet);
                // Get data and print as string
                char data[packet.tamanho];
                for(int i = 0; i < packet.tamanho; i++)
                {
                    data[i] = packet.dados[i];
                }
                printf("Data: %s\n", data);
                for(int i = 0; i < packet.tamanho; i++)
                {
                    printf("%d ", data[i]);
                }
            }
            else
            {
                char data[packet.tamanho];
                for(int i = 0; i < packet.tamanho; i++)
                {
                    data[i] = packet.dados[i];
                }
                for(int i = 0; i < packet.tamanho; i++)
                {
                    printf("%d ", data[i]);
                }
                printf("PARIDADE ERRADA\n");
            }
            return 0;
        }
    }

    return 0;
}
