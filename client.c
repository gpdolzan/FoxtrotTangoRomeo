#include "rawsocket.h"
#include "packethandler.h"
#include "client.h"
#include "md5hashing.h"

#define BUFFER_SIZE 1024

char cdirectory[1024]; // Client directory

int sendFileWrapper(int socket, char *filename)
{
    // Check if file exists
    if(checkFileExists(filename) == 1)
    {
        printf("[CLIENT-CLI] Arquivo %s nao existe\n", filename);
        return 1;
    }

    printf("[CLIENT-CLI] Enviando arquivo %s!\n", filename);
    if (sendFile(socket, filename, strlen(filename)) == 0)
    {
        printf("File sent successfully\n");
        return 0;
    }
    else
    {
        printf("Error sending file\n");
        return 1;
    }
}

int receiveFileWrapper(int socket, char *filename, int filesize)
{
    // Create packet to ask for file
    struct t_packet packet;
    struct t_packet serverPacket;
    char *buffer;
    int tries = 8;

    createPacket(&packet, strlen(filename), 0, REC_1_ARQ, filename);
    sendPacket(socket, &packet);

    // Listen to BACK_1_FILE
    while(tries > 0)
    {
        if(readPacket(socket, &serverPacket, 1) == 0)
        {
            if(serverPacket.tipo == BACK_1_FILE)
            {
                printf("RECEBI BACK_1_FILE!\n");
                break;
            }
            else if(serverPacket.tipo == ERRO)
            {
                // Get buffer from packet
                buffer = malloc(sizeof(char) * (serverPacket.tamanho + 1));
                // For loop
                for(int i = 0; i < serverPacket.tamanho; i++)
                {
                    buffer[i] = serverPacket.dados[i];
                }
                // Check if says ne
                if(strcmp(buffer, "ne") == 0)
                {
                    printf("Arquivo nao existe!\n");
                    return 1;
                }
            }
        }
        tries--;
    }

    if (receiveFile(socket, filename, filesize) == 0)
    {
        printf("File received successfully\n");
        return 0;
    }
    else
    {
        printf("Error receiving file\n");
        return 1;
    }
}

// Get wordcount from buffer
int getWordCount(char *buffer)
{
    int wordCount = 0;
    int i = 0;
    int firstLetter = 0;
    while(buffer[i] != '\0')
    {
        if(isspace(buffer[i]) && firstLetter == 1)
        {
            wordCount++;
            firstLetter = 0;
        }
        else if(!isspace(buffer[i]) && firstLetter == 0)
        {
            firstLetter = 1;
        }
        i++;
    }
    if(firstLetter == 1)
        wordCount++;
    return wordCount;
}

// Find and allocate memory for each arg in buffer
char **getArgs(char *buffer, int wordCount)
{
    char **args = malloc(sizeof(char*) * wordCount);
    int i = 0;
    int j = 0;
    int firstLetter = 0;
    int argCount = 0;
    while(buffer[i] != '\0')
    {
        if(isspace(buffer[i]) && firstLetter == 1)
        {
            args[argCount] = malloc(sizeof(char) * (i - j + 1));
            memcpy(args[argCount], &buffer[j], i - j);
            args[argCount][i - j] = '\0';
            argCount++;
            firstLetter = 0;
        }
        else if(!isspace(buffer[i]) && firstLetter == 0)
        {
            firstLetter = 1;
            j = i;
        }
        i++;
    }
    if(firstLetter == 1)
    {
        args[argCount] = malloc(sizeof(char) * (i - j + 1));
        memcpy(args[argCount], &buffer[j], i - j);
        args[argCount][i - j] = '\0';
        argCount++;
    }
    return args;
}

// List files in current directory
void listFiles()
{
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            // Dont list . or ..
            if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                continue;
            // Check if directory
            if(dir->d_type == DT_DIR)
                printf("\033[0;32m\t%s\n", dir->d_name);
            // If is file
            else if (dir->d_type == DT_REG)
            {
                // if is executable file
                if(access(dir->d_name, X_OK) == 0)
                    printf("\033[0;31m\t%s\n", dir->d_name);
                else
                    printf("\033[0;34m\t%s\n", dir->d_name);
            }
            // set color to white
            printf("\x1B[0m");
        }
        // Free and close
        closedir(d);
    }
}

// Client commands function
int clientCommands(int socket, char **args, int wordCount)
{
    if(strcmp(args[0], "exit") == 0)
    {
        // Get username from computer
        char username[32];
        getlogin_r(username, 32);
        printf("[CLIENT-CLI] Ate a proxima %s!\n", username);
        // Free args
        for(int i = 0; i < wordCount; i++)
        {
            free(args[i]);
        }
        free(args);
        exit(0);
    }
    else if (strcmp(args[0], "send") == 0)
    {
        int tries = 8;
        // Create packet
        struct t_packet packet;
        if(wordCount == 2)
        {
            // Check if string has a * in it
            if(strchr(args[1], '*') != NULL)
            {
                // Use glob to get all files
                glob_t globbuf;
                glob(args[1], 0, NULL, &globbuf);
                // Using an integer count how many files were found
                int count = globbuf.gl_pathc;
                printf("[CLIENT-CLI] Sending %d files\n", count);

                // Monta string #[valor]arq
                char countString[10];
                sprintf(countString, "#%d#", count);

                // Create BACK_PLUS_1_FILE packet
                if(count > 0)
                {
                    createPacket(&packet, strlen(countString), 0, BACK_PLUS_1_FILE, countString);
                    sendPacket(socket, &packet);
                }
                
                // Wait for OK
                while(count > 0)
                {
                    if(tries <= 0)
                    {
                        printf("[CLIENT-CLI] > Time exceeded - Server not responding\n");
                        break;
                    }
                    sendPacket(socket, &packet);
                    if(readPacket(socket, &packet, 1) == 0)
                    {
                        // Check parity
                        if(checkParity(&packet) == 0)
                        {
                            // Check if packet is OK
                            if(packet.tipo == OK)
                            {
                                printf("[CLIENT-CLI] OK received\n");
                                break;
                            }
                        }
                    }
                    else
                    {
                        tries--;
                    }
                }

                for(int i = 0; i < count; i++)
                {
                    // Send each file
                    // Using C function to test if string is a directory
                    DIR* directory = opendir(globbuf.gl_pathv[i]);
                    if(directory != NULL)
                    {
                        closedir(directory);
                        continue;
                    }

                    if(sendFileWrapper(socket, globbuf.gl_pathv[i]) == 0)
                    {
                        printf("[CLIENT-CLI] Arquivo %s enviado com sucesso!\n", globbuf.gl_pathv[i]);
                    }
                    else
                    {
                        printf("[CLIENT-CLI] Erro ao enviar arquivo %s\n", globbuf.gl_pathv[i]);
                    }
                }

                // Send FIM_GRUPO_ARQ
                createPacket(&packet, 0, 0, FIM_GRUPO_ARQ, NULL);
                sendPacket(socket, &packet);

                // Free globbuf
                globfree(&globbuf);
            }
            else
            {
                if(sendFileWrapper(socket, args[1]) == 0)
                    printf("[CLIENT-CLI] Arquivo enviado com sucesso!\n");
                else
                    printf("[CLIENT-CLI] Falha ao enviar arquivo\n");
            }
        }
    }
    else if (strcmp(args[0], "get") == 0)
    {
        //TODO
        struct t_packet packet; // I will send
        struct t_packet sPacket; // I will receive
        int tries = 8;
        if(wordCount == 2)
        {
            // Check if string has a * in it
            if(strchr(args[1], '*') != NULL)
            {

            }
            else
            {
                // Create REC_1_FILE
                createPacket(&packet, strlen(args[1]), 0, REC_1_ARQ, args[1]);
                sendPacket(socket, &packet);
        
                if(receiveFile(socket, args[1], strlen(args[1])) == 1)
                {
                    printf("[CLIENT-CLI] > Erro ao receber arquivo %s\n", args[1]);
                }
                else
                {
                    printf("[CLIENT-CLI] > Arquivo %s recebido com sucesso\n", args[1]);
                }
            }
        }
    }
    else if (strcmp(args[0], "ls") == 0)
    {
        listFiles();
    }
    else if (strcmp(args[0], "cd") == 0)
    {
        if(wordCount >= 2)
        {
            if(chdir(args[1]) == 0)
            {
                printf("[CLIENT-CLI] Diretorio modificado com sucesso\n");
            }
            else
            {
                printf("[CLIENT-CLI] Diretorio %s nao encontrado\n", args[1]);
            }
        }
        else if(wordCount < 2)
        {
            printf("[CLIENT-CLI] voce precisa especificar o algum diretorio\n");
        }
    }
    else if (strcmp(args[0], "sdir") == 0)
    {
        int tries = 8;
        if(wordCount >= 2)
        {
            printf("[CLIENT-CLI] Enviando pedido para servidor mudar diretorio para: %s\n", args[1]);
            // Create packet
            struct t_packet packet;
            createPacket(&packet, strlen(args[1]), 0, CH_DIR_SERVER, args[1]);
            sendPacket(socket, &packet);

            // Wait for OK
            while(1)
            {
                if(tries <= 0)
                {
                    printf("[CLIENT-CLI] > Time exceeded - Server not responding\n");
                    break;
                }
                if(readPacket(socket, &packet, 1) == 0)
                {
                    // Check parity
                    if(checkParity(&packet) == 0)
                    {
                        // Check if packet is OK
                        if(packet.tipo == OK)
                        {
                            printf("[CLIENT-CLI] OK received\n");
                            break;
                        }
                        else
                        {
                            printf("[CLIENT-CLI] > Server could not change to %s\n", args[1]);
                            break;
                        }
                    }
                }
                else
                {
                    tries--;
                }
            }
        }
        else if(wordCount < 2)
        {
            printf("[CLIENT-CLI] voce precisa especificar o algum diretorio\n");
        }
    }
    else if (strcmp(args[0], "ls") == 0)
    {
        printf("[CLIENT-CLI] deseja listar o diretorio local\n");
        listFiles();
    }
    else if (strcmp(args[0], "cls") == 0 || strcmp(args[0], "clear") == 0)
    {
        clear();
    }
    else if (strcmp(args[0], "md5") == 0)
    {
        if(wordCount < 2)
        {
            printf("[CLIENT-CLI] voce precisa especificar o algum arquivo\n");
        }
        else
        {
            // Create file with args[1] name
            FILE *file = fopen(args[1], "rb");
            if(file == NULL)
            {
                printf("[CLIENT-CLI] Arquivo local %s nao encontrado\n", args[1]);
            }
            else
            {
                // Create a uint8_t array with 16 bytes
                uint8_t *hash = malloc(16);
                // Using md5file function to get md5 hash
                md5File(file, hash);
                printf("[CLIENT-CLI] Hash md5 do arquivo LOCAL %s: ", args[1]);
                // For loop to print hash
                for(int i = 0; i < 16; i++)
                {
                    printf("%02x", hash[i]);
                }
                printf("\n");
                free(hash);
                fclose(file);
            }
            // Send VERIFICA_BACK
            struct t_packet packet;
            createPacket(&packet, strlen(args[1]), 0, VERIFICA_BACK, args[1]);
            sendPacket(socket, &packet);
            // Wait for OK
            int tries = 8;
            while(1)
            {
                if(tries <= 0)
                {
                    printf("[CLIENT-CLI] > Time exceeded - Server not responding\n");
                    break;
                }
                if(readPacket(socket, &packet, 1) == 0)
                {
                    // Check parity
                    if(checkParity(&packet) == 0)
                    {
                        // Check if packet is OK
                        if(packet.tipo == MD5)
                        {
                            // Get string from packet
                            char *string = malloc(packet.tamanho + 1);
                            // FOR LOOP
                            for(int i = 0; i < packet.tamanho; i++)
                            {
                                string[i] = packet.dados[i];
                            }
                            string[packet.tamanho] = '\0';
                            printf("[CLIENT-CLI] > Hash md5 do arquivo REMOTO %s: %s\n", args[1], string);
                            break;
                        }
                        else if(packet.tipo == ERRO)
                        {
                            printf("[CLIENT-CLI] > Server could find %s\n", args[1]);
                            break;
                        }
                    }
                }
                else
                {
                    tries--;
                }
            }
        }
    }
    else if (strcmp(args[0], "help") == 0)
    {
        printf("[CLIENT-CLI] Comandos:\n");
        printf("[CLIENT-CLI]\texit - sair do programa\n");
        printf("[CLIENT-CLI]\tsend <file or files> - enviar arquivos para o servidor\n");
        printf("[CLIENT-CLI]\tget <file or files> - baixar arquivos do servidor\n");
        printf("[CLIENT-CLI]\tcd <directory> - mudar diretorio local\n");
        printf("[CLIENT-CLI]\tsdir <directory> - mudar diretorio no servidor\n");
        printf("[CLIENT-CLI]\tls - listar diretorio corrente\n");
        printf("[CLIENT-CLI]\tcls ou clear - limpar tela\n");
    }
    else
    {
        printf("[CLIENT-CLI] comando nao reconhecido\n");
    }
}

int main(int argc, char const *argv[])
{
    const char *dirname = "clientdir";
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

    int socket = ConexaoRawSocket("enp5s0");
    //int socket = ConexaoRawSocket("lo");
    int wordCount;
    char buffer[1024];
    char **args;
    printf("\x1B[0m");

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    // Clear screen
    clear();
    while(1)
    {
        // Get path
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("[%s] > ", cwd);
        // Clean buffer
        memset(buffer, 0, sizeof(buffer));
        // fgets buffer
        fgets(buffer, BUFFER_SIZE, stdin);
        // Remove newline from buffer
        buffer[strcspn(buffer, "\n")] = 0;

        // Get word count
        wordCount = getWordCount(buffer);
        args = getArgs(buffer, wordCount);
        
        clientCommands(socket, args, wordCount);
        for(int i = 0; i < wordCount; i++)
        {
            free(args[i]);
        }
        free(args);
    }
    return 0;
}

