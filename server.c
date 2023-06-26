#include "rawsocket.h"
#include "packethandler.h"
#include "md5.h"

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
    
    const char *dirname = "serverdir";
    mode_t target_mode = 0777;
    if(chdir(dirname) == -1)
    {
        if (mkdir(dirname, 0) == 0)
        {
            chmod(dirname, target_mode);
            // Change to dirname
            chdir(dirname);
        }
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
        else if(myPacket.tipo == BACK_PLUS_1_FILE)
        {
            int nFiles = myPacket.sequencia;
            // Send OK
            createPacket(&sPacket, 0, 0, OK, NULL);
            sendPacket(socket, &sPacket);
            for(int i = 0; i < nFiles; i++)
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

                    sendPacket(socket, &myPacket);
                    printf("[SERVER-CLI] Sending file from %s\n", tempBuffer);
                    sendFile(socket, tempBuffer, strlen(tempBuffer));
                    printf("[SERVER-CLI] I Finished sending file!\n");
                    free(buffer);
                }
            }
        }
        else if(myPacket.tipo == VERIFICA_BACK)
        {
            // Calculate hash of file
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
            printf("Verificando arquivo %s\n", tempBuffer);
            char *hash = (uint8_t *)malloc(16 * sizeof(uint8_t));
            // Try to open file
            FILE *fp = fopen(tempBuffer, "rb");
            if(fp == NULL)
            {
                printf("Erro ao abrir arquivo\n");
                // Send ERRO
                char *msg = "ai"; // arquivo inexistente
                createPacket(&sPacket, strlen(msg), 0, ERRO, msg);
                sendPacket(socket, &sPacket);
            }
            else
            {
                md5File(fp, hash);
                // Send MD5
                createPacket(&sPacket, 16, 0, MD5, hash);
                sendPacket(socket, &sPacket);
            }
        }
        else if(myPacket.tipo == CH_DIR_SERVER)
        {
            // Create buffer
            char *buffer = (char *)malloc(myPacket.tamanho * sizeof(char));
            // Copy data to buffer using for loop
            for(int i = 0; i < myPacket.tamanho; i++)
            {
                buffer[i] = myPacket.dados[i];
            }
            mode_t target_mode = 0777;
            if(chdir(buffer) == -1)
            {
                if (mkdir(buffer, 0) == 0)
                {
                    chmod(buffer, target_mode);
                    // Change to buffer and check if it worked
                    if(chdir(buffer) == -1)
                    {
                        printf("Erro ao mudar de diretorio\n");
                        // Send ERRO
                        char *msg = "fmd"; // falha mudanca de diretorio
                        createPacket(&sPacket, strlen(msg), 0, ERRO, msg);
                        sendPacket(socket, &sPacket);
                    }
                    else
                    {
                        // Get current directory
                        getcwd(sdirectory, sizeof(sdirectory));
                        printf("Diretorio atual: %s\n", sdirectory);
                        // Send OK
                        createPacket(&sPacket, 0, 0, OK, NULL);
                        sendPacket(socket, &sPacket);
                    }
                }
            }
            else
            {
                // Get current directory
                getcwd(sdirectory, sizeof(sdirectory));
                printf("Diretorio atual: %s\n", sdirectory);
                // Send OK
                createPacket(&sPacket, 0, 0, OK, NULL);
                sendPacket(socket, &sPacket);
            }
        }
    }
}