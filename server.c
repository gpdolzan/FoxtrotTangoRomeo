#include "rawsocket.h"
#include "packethandler.h"

int main(int argc, char const *argv[])
{
    //int socket = ConexaoRawSocket("eno1");
    int socket = ConexaoRawSocket("lo");
    struct t_packet packet;
    struct t_packet sPacket;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

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
                    printPacket(&packet);
                    exit(1);
                    printf("Erro de paridade\n");
                    // Send NACK
                    createPacket(&sPacket, 0, 0, NACK, NULL);
                    sendPacket(socket, &sPacket);
                }
                else
                {
                    printf("caraiba\n");
                    // Create buffer
                    char *buffer = (char *)malloc(packet.tamanho * sizeof(char));
                    // Copy data to buffer using for loop
                    for(int i = 0; i < packet.tamanho; i++)
                    {
                        buffer[i] = packet.dados[i];
                    }
        
                    printf("RECEIVING\n");
                    if(receiveFile(socket, buffer, packet.tamanho, SERVER) == 1)
                    {
                        printf("Erro ao receber arquivo\n");
                    }
                    else
                    {
                        printf("Arquivo recebido com sucesso\n");
                    }
                    free(buffer);
                }
            }
        }
        else if(packet.tipo == REC_1_ARQ)
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
                    // Create buffer
                    char *buffer = (char *)malloc(packet.tamanho * sizeof(char));
                    // Copy data to buffer using for loop
                    for(int i = 0; i < packet.tamanho; i++)
                    {
                        buffer[i] = packet.dados[i];
                    }

                    printf("SENDING\n");
                    printPacket(&packet);
                    sendFile(socket, buffer, packet.tamanho, SERVER);
                    printf("I FINISHED\n");
                    free(buffer);
                }
            }
        }
    }
}