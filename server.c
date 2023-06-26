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

    char *buffer;

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
    printf("[%s] > Esperando comandos\n", sdirectory);
    while(1)
    {
        readPacket(socket, &myPacket, 0);

        // Check parity
        if(checkParity(&myPacket) == 1)
        {
            continue;
        }

        if(myPacket.tipo == BACK_1_FILE)
        {
            printf("[%s] > Receber um arquivo\n", sdirectory);
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
                    buffer = (char *)malloc(myPacket.tamanho * sizeof(char));
                    // Copy data to buffer using for loop
                    for(int i = 0; i < myPacket.tamanho; i++)
                    {
                        buffer[i] = myPacket.dados[i];
                    }
        
                    if(receiveFile(socket, buffer, strlen(buffer)) == 1)
                    {
                        printf("[%s] > Erro ao receber arquivo %s\n", sdirectory, buffer);
                        remove(buffer);
                    }
                    else
                    {
                        printf("[%s] > Arquivo %s recebido com sucesso\n", sdirectory, buffer);
                    }
                    free(buffer);
                }
            }
        }
        else if(myPacket.tipo == BACK_PLUS_1_FILE)
        {
            buffer = (char *)malloc(myPacket.tamanho * sizeof(char));
            // Copy data to buffer using for loop
            for(int i = 0; i < myPacket.tamanho; i++)
            {
                buffer[i] = myPacket.dados[i];
            }
            // String to int
            int nFiles = atoi(buffer);
            free(buffer);

            printf("[%s] > Receber %d arquivos\n", sdirectory, myPacket.sequencia);
            int nFiles = myPacket.sequencia;
            // Send OK
            createPacket(&sPacket, 0, 0, OK, NULL);
            sendPacket(socket, &sPacket);
            for(int i = 0; i < nFiles; i++)
            {
                // Using sdirectory to save file
                if(receiveFile(socket, buffer, strlen(buffer)) == 1)
                {
                    printf("[%s] > Erro ao receber arquivo %s\n", sdirectory, buffer);
                    remove(buffer);
                }
                else
                {
                    printf("[%s] > Arquivo %s recebido com sucesso\n", sdirectory, buffer);
                }
                free(buffer);
            }
            // Wait for FIM_GRUPO_ARQ
            while(1)
            {
                if(readPacket(socket, &myPacket, 1) == 0)
                {
                    if(checkParity(&myPacket) == 1)
                    {
                        printf("[%s] > Erro de paridade\n", sdirectory);
                        // Send NACK
                        createPacket(&sPacket, 0, 0, NACK, NULL);
                        sendPacket(socket, &sPacket);
                    }
                    else
                    {
                        if(myPacket.tipo == FIM_GRUPO_ARQ)
                        {
                            printf("[%s] > FIM_GRUPO_ARQ recebido\n", sdirectory);
                            break;
                        }
                        else
                        {
                            continue;
                        }
                    }
                }
            }
        }
        else if(myPacket.tipo == REC_1_ARQ)
        {
            /*if(myPacket.sequencia == 0) // Inicio de uma sequencia de pacotes
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
                    char *buffer_rec1 = (char *)malloc(myPacket.tamanho * sizeof(char));
                    // Copy data to buffer using for loop
                    for(int i = 0; i < myPacket.tamanho; i++)
                    {
                        buffer_rec1[i] = myPacket.dados[i];
                    }
                    // Create a temporary buffer that concatenates server directory and file name
                    char *tempBuffer_rec1 = (char *)malloc((strlen(buffer_rec1) + (strlen(sdirectory) + 1) * sizeof(char)));
                    strcpy(tempBuffer_rec1, sdirectory);
                    strcat(tempBuffer_rec1, "/");
                    strcat(tempBuffer_rec1, buffer_rec1);

                    sendPacket(socket, &myPacket);
                    printf("[SERVER-CLI] Sending file from %s\n", tempBuffer_rec1);
                    sendFile(socket, tempBuffer_rec1, strlen(tempBuffer_rec1));
                    printf("[SERVER-CLI] I Finished sending file!\n");
                    free(buffer_rec1);
                    free(tempBuffer_rec1);
                }
            }*/
        }
        else if(myPacket.tipo == VERIFICA_BACK)
        {
            // Calculate hash of file
            buffer = (char *)malloc(myPacket.tamanho * sizeof(char));
            // Copy data to buffer using for loop
            for(int i = 0; i < myPacket.tamanho; i++)
            {
                buffer[i] = myPacket.dados[i];
            }
            char *hash = (uint8_t *)malloc(16 * sizeof(uint8_t));
            // Try to open file
            FILE *fp = fopen(buffer, "rb");
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
            free(buffer);
        }
        else if(myPacket.tipo == CH_DIR_SERVER)
        {
            
        }
    }
}