#include "rawsocket.h"
#include "packethandler.h"

// Global variables
char sdirectory[1024]; // Server directory

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

    // Set server directory as /received
    if(chdir("received") == 1)
    {
        printf("[SERVER-CLI] Erro ao mudar padrao do servidor\n");
        exit(1);
    }

    printf("[SERVER-CLI] Servidor Funcionando!\n");
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
                    // Create a temporary buffer that concatenates server directory and file name
                    char *tempBuffer = (char *)malloc((strlen(buffer) + (strlen(sdirectory) + 1) * sizeof(char)));
                    
        
                    printf("RECEIVING\n");
                    if(receiveFile(socket, buffer, myPacket.tamanho) == 1)
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