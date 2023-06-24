#include "rawsocket.h"
#include "packethandler.h"

int main(int argc, char const *argv[])
{
    int socket = ConexaoRawSocket("eno1");
    //int socket = ConexaoRawSocket("lo");
    struct t_packet myPacket; // Packets I will send
    struct t_packet sPacket;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    while(1)
    {
        readPacket(socket, &myPacket, 0);
        if(myPacket.tipo == BACK_1_FILE)
        {
            if(myPacket.sequencia == 0) // Inicio de uma sequencia de pacotes
            {
                // Check parity
                if(checkParity(&myPacket) == 1)
                {
                    printPacket(&myPacket);
                    exit(1);
                    // Send NACK
                    createPacket(&sPacket, 0, 0, NACK, NULL);
                    sendPacket(socket, &sPacket);
                }
                else
                {
                    // Create buffer
                    char *buffer = (char *)malloc(myPacket.tamanho * sizeof(char));
                    // Copy data to buffer using for loop
                    for(int i = 0; i < myPacket.tamanho; i++)
                    {
                        buffer[i] = myPacket.dados[i];
                    }
        
                    printf("RECEIVING\n");
                    if(receiveFile(socket, buffer, myPacket.tamanho, SERVER) == 1)
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
        else if(myPacket.tipo == REC_1_ARQ)
        {
            if(myPacket.sequencia == 0) // Inicio de uma sequencia de pacotes
            {
                // Check parity
                if(checkParity(&myPacket) == 1)
                {
                    printf("Erro de paridade\n");
                    // Send NACK
                    createPacket(&sPacket, 0, 0, NACK, NULL);
                    sendPacket(socket, &sPacket);
                }
                else
                {
                    // Create buffer
                    char *buffer = (char *)malloc(myPacket.tamanho * sizeof(char));
                    // Copy data to buffer using for loop
                    for(int i = 0; i < myPacket.tamanho; i++)
                    {
                        buffer[i] = myPacket.dados[i];
                    }

                    printf("SENDING\n");
                    sendFile(socket, buffer, myPacket.tamanho);
                    printf("I FINISHED\n");
                    free(buffer);
                }
            }
        }
    }
}