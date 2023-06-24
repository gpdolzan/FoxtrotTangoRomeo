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
    
    if(chdir("server") != 0)
    {
        mkdir("server", 0777);
        chdir("server");
    }

    printf("[SERVER-CLI] Servidor Funcionando!\n");
    getcwd(sdirectory, sizeof(sdirectory));
    printf("[SERVER-CLI] Diretorio do servidor: %s\n", sdirectory);
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
                    strcpy(tempBuffer, sdirectory);
                    strcat(tempBuffer, "/");
                    strcat(tempBuffer, buffer);
        
                    printf("Saving file %s on directory %s\n", buffer, sdirectory);
                    printf("Final directory: %s\n", tempBuffer);
                    if(receiveFile(socket, tempBuffer, strlen(tempBuffer)) == 1)
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
                    // Create a temporary buffer that concatenates server directory and file name
                    char *tempBuffer = (char *)malloc((strlen(buffer) + (strlen(sdirectory) + 1) * sizeof(char)));
                    strcpy(tempBuffer, sdirectory);
                    strcat(tempBuffer, "/");
                    strcat(tempBuffer, buffer);

                    printf("[SERVER-CLI] Sending file from %s\n", tempBuffer);
                    sendFile(socket, tempBuffer, strlen(tempBuffer));
                    printf("[SERVER-CLI] I Finished sending file!\n");
                    free(buffer);
                }
            }
        }
    }
}