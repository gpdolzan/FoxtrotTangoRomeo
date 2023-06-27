#include "rawsocket.h"
#include "packethandler.h"
#include "md5hashing.h"

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
            struct t_packet packet;
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

                    // Send OK
                    FILE* file = fopen(buffer, "wb");
                    createPacket(&packet, 0, 0, OK, NULL);
                    sendPacket(socket, &packet);
        
                    if(receiveFile(socket, file) == 1)
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
            // Data has a special format #[number]#
            // Get number
            struct t_packet serverPacket;
            int tries = 8;
            int nFiles = 0;
            char *buffer = (char *)malloc(myPacket.tamanho * sizeof(char));
            for(int i = 0; i < myPacket.tamanho; i++)
            {
                buffer[i] = myPacket.dados[i];
            }
            // Now ignore the # and get the number
            char *tempBuffer = (char *)malloc((strlen(buffer) + 1) * sizeof(char));
            int j = 0;
            for(size_t i = 1; i < strlen(buffer) - 1; i++)
            {
                tempBuffer[j] = buffer[i];
                j++;
            }
            tempBuffer[j] = '\0';
            nFiles = atoi(tempBuffer);
            free(tempBuffer);
            free(buffer);

            if(nFiles > 0)
            {
                // Send OK
                createPacket(&sPacket, 0, 0, OK, NULL);
                sendPacket(socket, &sPacket);
                // Wait for BACK_1_ARQ
                while(1)
                {
                    if(readPacket(socket, &myPacket, 1) == 0)
                    {
                        if(checkParity(&myPacket) == 0)
                        {
                            if(tries <= 0)
                            {
                                printf("[%s] > Time exceeded BACK_1_ARQ\n", sdirectory);
                                break;
                            }
                            if(myPacket.tipo == BACK_1_FILE)
                            {
                                FILE* file;
                                // Create buffer
                                buffer = (char *)malloc(myPacket.tamanho * sizeof(char));
                                // Copy data to buffer using for loop
                                for(int i = 0; i < myPacket.tamanho; i++)
                                {
                                    buffer[i] = myPacket.dados[i];
                                }

                                file = fopen(buffer, "wb");
                                // Send OK
                                createPacket(&serverPacket, 0, 0, OK, NULL);
                                sendPacket(socket, &serverPacket);

                                // Using sdirectory to save file
                                if(receiveFile(socket, file) == 1)
                                {
                                    printf("[%s] > Erro ao receber arquivo %s\n", sdirectory, buffer);
                                    remove(buffer);
                                }
                                else
                                {
                                    nFiles--;
                                    printf("[%s] > Arquivo %s recebido com sucesso\n", sdirectory, buffer);
                                    if(nFiles <= 0)
                                    {
                                        break;
                                    }
                                }
                                free(buffer);
                            }
                            else
                            {
                                tries--;
                                continue;
                            }
                        }
                    }
                }
                // Wait for FIM_GRUPO_ARQ
                tries = 8;
                while(1)
                {
                    if (tries <= 0)
                    {
                        printf("[%s] > Time exceeded GROUP_FIM_ARQ\n", sdirectory);
                        break;
                    }

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
                                tries--;
                                continue;
                            }
                        }
                    }
                }
            }
        }
        else if(myPacket.tipo == REC_1_ARQ)
        {
            struct t_packet packet;

            // Create buffer
            char *buffer = (char *)malloc((myPacket.tamanho + 1) * sizeof(char));
            for(int i = 0; i < myPacket.tamanho; i++)
            {
                buffer[i] = myPacket.dados[i];
            }
            buffer[myPacket.tamanho] = '\0';
            
            if(checkFileExists(buffer) == 0)
            {
                printf("[%s] > Arquivo %s nao existe!\n", sdirectory, buffer);
                // Send NACK
                createPacket(&packet, 0, 0, NACK, NULL);
                sendPacket(socket, &packet);
            }
            else
            {
                // Try to open file
                FILE* file = fopen(buffer, "rb");
                // Send BACK_1_FILE
                createPacket(&packet, strlen(buffer), 0, BACK_1_FILE, buffer);
                sendPacket(socket, &packet);
                getcwd(sdirectory, sizeof(sdirectory));
                // Send file
                if(sendFile(socket, file) == 0)
                {
                    printf("[%s] > Arquivo %s enviado com sucesso\n", sdirectory, buffer);
                }
                else
                {
                    printf("[%s] > Erro ao enviar arquivo %s\n", sdirectory, buffer);
                }
            }
        }
        else if(myPacket.tipo == VERIFICA_BACK)
        {
            // Create packet
            struct t_packet packet_md5;
            // Create packet
            struct t_packet packet_sendrealmd5;
            // Create buffer
            char *buffer = (char *)malloc((myPacket.tamanho + 1) * sizeof(char));
            for(int i = 0; i < myPacket.tamanho; i++)
            {
                buffer[i] = myPacket.dados[i];
            }
            buffer[myPacket.tamanho] = '\0';

            // Concatenate sdirectory and buffer
            char *tempBuffer = (char *)malloc((strlen(sdirectory) + strlen(buffer) + 1) * sizeof(char));
            strcpy(tempBuffer, sdirectory);
            strcat(tempBuffer, "/");
            strcat(tempBuffer, buffer);

            printf("[TEMP BUFFER] %s\n", tempBuffer);

            // Try open file
            FILE *fp = fopen(buffer, "rb");
            if(fp != NULL)
            {
                printf("aqui\n");
                // Create a uint8_t array with 16 bytes
                uint8_t *hash = malloc(16);
                // Using md5file function to get md5 hash
                md5File(fp, hash);

                // Make hash a string
                char *hashString = (char *)malloc(33 * sizeof(char));
                for(int i = 0; i < 16; i++)
                {
                    sprintf(&hashString[i * 2], "%02x", hash[i]);
                }
                hashString[32] = '\0';
                
                createPacket(&packet_md5, strlen(hashString), 0, MD5, hashString);
                sendPacket(socket, &packet_md5);

                free(hash);
                fclose(fp);
            }
            else
            {
                printf("aqui nao\n");
                createPacket(&packet_md5, 0, 0, ERRO, NULL);
                sendPacket(socket, &packet_md5);
            }

            free(buffer);
            free(tempBuffer);
        }
        else if(myPacket.tipo == CH_DIR_SERVER)
        {
            // Create buffer and use for loop to copy from packet dados
            char *buffer = (char *)malloc((myPacket.tamanho + 1) * sizeof(char));
            for(int i = 0; i < myPacket.tamanho; i++)
            {
                buffer[i] = myPacket.dados[i];
            }
            buffer[myPacket.tamanho] = '\0';

            mode_t target_mode = 0777;
            if(chdir(buffer) == -1)
            {
                if (mkdir(buffer, 0) == 0)
                {
                    chmod(buffer, target_mode);
                    // Change to dirname
                    if(chdir(buffer) == 0)
                    {
                        // Send OK
                        createPacket(&sPacket, 0, 0, OK, NULL);
                        sendPacket(socket, &sPacket);
                        // Print cwd
                        getcwd(sdirectory, sizeof(sdirectory));
                        printf("[%s] > Diretorio atual: %s\n", sdirectory, sdirectory);
                    }
                    else
                    {
                        // Send ERRO
                        createPacket(&sPacket, 0, 0, ERRO, NULL);
                        sendPacket(socket, &sPacket);
                    }
                }
                else
                {
                    // Send OK
                    createPacket(&sPacket, 0, 0, OK, NULL);
                    sendPacket(socket, &sPacket);
                    // Print cwd
                    getcwd(sdirectory, sizeof(sdirectory));
                    printf("[%s] > Diretorio atual: %s\n", sdirectory, sdirectory);
                }
            }
            else
            {
                // Send OK
                createPacket(&sPacket, 0, 0, OK, NULL);
                sendPacket(socket, &sPacket);
                // Print cwd
                getcwd(sdirectory, sizeof(sdirectory));
                printf("[%s] > Diretorio atual: %s\n", sdirectory, sdirectory);
            }
            free(buffer);
        }
    }
}