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
                    char *buffer_bk1_file = (char *)malloc(myPacket.tamanho * sizeof(char));
                    // Copy data to buffer using for loop
                    for(int i = 0; i < myPacket.tamanho; i++)
                    {
                        buffer_bk1_file[i] = myPacket.dados[i];
                    }
        
                    if(receiveFile(socket, buffer_bk1_file, strlen(buffer_bk1_file)) == 1)
                    {
                        printf("Erro ao receber arquivo\n");
                    }
                    else
                    {
                        printf("Arquivo recebido com sucesso\n");
                    }
                    free(buffer_bk1_file);
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
                char *buffer_bk1_plus = (char *)malloc((myPacket.tamanho + 1) * sizeof(char));
                // Copy data to buffer using for loop
                for(int i = 0; i < myPacket.tamanho; i++)
                {
                    buffer_bk1_plus[i] = myPacket.dados[i];
                }
                buffer_bk1_plus[myPacket.tamanho] = '\0';
                // Create a temporary buffer that concatenates server directory and file name
                char *tempBuffer_bk1_plus = (char *)malloc((strlen(buffer_bk1_plus) + (strlen(sdirectory) + 1) * sizeof(char)));
                strcpy(tempBuffer_bk1_plus, sdirectory);
                strcat(tempBuffer_bk1_plus, "/");
                strcat(tempBuffer_bk1_plus, buffer_bk1_plus);
        
                printf("Saving file %s on directory %s\n", buffer_bk1_plus, sdirectory);
                printf("Final directory: %s\n", tempBuffer_bk1_plus);
                if(receiveFile(socket, tempBuffer_bk1_plus, strlen(tempBuffer_bk1_plus)) == 1)
                {
                    printf("Erro ao receber arquivo\n");
                }
                else
                {
                    printf("Arquivo recebido com sucesso\n");
                }
                free(buffer_bk1_plus);
                free(tempBuffer_bk1_plus);
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
            }
        }
        else if(myPacket.tipo == VERIFICA_BACK)
        {
            // Calculate hash of file
            char *buffer_md5 = (char *)malloc(myPacket.tamanho * sizeof(char));
            // Copy data to buffer using for loop
            for(int i = 0; i < myPacket.tamanho; i++)
            {
                buffer_md5[i] = myPacket.dados[i];
            }
            char *tempBuffer_md5 = (char *)malloc((strlen(buffer_md5) + (strlen(sdirectory) + 1) * sizeof(char)));
            strcpy(tempBuffer_md5, sdirectory);
            strcat(tempBuffer_md5, "/");
            strcat(tempBuffer_md5, buffer_md5);
            char *hash = (uint8_t *)malloc(16 * sizeof(uint8_t));
            // Try to open file
            FILE *fp = fopen(tempBuffer_md5, "rb");
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

                // For loop printing hash
                printf("Hash: ");
                for(int i = 0; i < 16; i++)
                {
                    printf("%02x", hash[i]);
                }

                // Send MD5
                createPacket(&sPacket, 16, 0, MD5, hash);
                sendPacket(socket, &sPacket);
            }
            free(buffer_md5);
            free(tempBuffer_md5);
        }
        else if(myPacket.tipo == CH_DIR_SERVER)
        {
            
        }
    }
}