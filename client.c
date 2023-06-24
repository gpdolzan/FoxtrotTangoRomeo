#include "rawsocket.h"
#include "packethandler.h"
#include "client.h"

#define BUFFER_SIZE 1024

int sendFileWrapper(int socket, char *filename)
{
    // Check if file exists
    if(checkFileExists(filename) == 1)
    {
        printf("File does not exist\n");
        return 1;
    }

    if (sendFile(socket, filename, strlen(filename), CLIENT) == 0)
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
    int tries = 5;

    createPacket(&packet, strlen(filename), 0, REC_1_ARQ, filename);
    printPacket(&packet);
    sendPacket(socket, &packet);
    // Aguardar resposta (talvez timeout)
    if (readPacket(socket, &serverPacket, 1) == 1)
    {
        if(tries <= 0)
        {
            printf("Timeout received file -> client-side\n");
            return 1;
        }
        tries--;
    }
    else
    {
        tries = 5;
    }

    if (receiveFile(socket, filename, filesize, CLIENT) == 0)
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

/*int main(int argc, char const *argv[])
{
    //int socket = ConexaoRawSocket("enp5s0");
    int socket = ConexaoRawSocket("lo");
    // get input from user
    char input[31];
    // Set everything in buffer to 0
    memset(input, 0, sizeof(input));
    printf("Enter a filename: ");
    scanf("%31s", input);
    
}*/

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
        printf("[CLIENT-CLI] See ya!\n");
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
        if(wordCount == 2)
        {
            if(sendFileWrapper(socket, args[1]) == 0)
            {
                printf("[CLIENT-CLI] File sent successfully\n");
            }
            else
            {
                printf("[CLIENT-CLI] Error sending file\n");
            }
        }
    }
    else if (strcmp(args[0], "get") == 0)
    {
        if(wordCount == 2)
        {
            int count = strlen(args[1]) + 1;
            if(receiveFileWrapper(socket, args[1], count) == 0)
            {
                printf("[CLIENT-CLI] File received successfully\n");
            }
            else
            {
                printf("[CLIENT-CLI] Error receiving file\n");
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
                // Mostrar diretorio atual
                char cwd[1024];
                getcwd(cwd, sizeof(cwd));
                printf("[CLIENT-CLI] Diretorio Atual: %s\n", cwd);
            }
            else
            {
                printf("[CLIENT-CLI] Diretorio %s nao encontrado\n", args[1]);
            }
        }
        else if(wordCount < 2)
        {
            char cwd[1024];
            getcwd(cwd, sizeof(cwd));
            printf("[CLIENT-CLI] Diretorio Atual: %s\n", cwd);
        }
    }
    else if (strcmp(args[0], "scd") == 0)
    {

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
    else if (strcmp(args[0], "help") == 0)
    {
        printf("[CLIENT-CLI] Comandos:\n");
        printf("\texit - sair do programa\n");
        printf("\tsend <file or files> - enviar arquivos para o servidor\n");
        printf("\tget <file or files> - baixar arquivos do servidor\n");
        printf("\tcd <directory> - mudar diretorio local\n");
        printf("\tscd <directory> - mudar diretorio no servidor\n");
        printf("\tls - listar diretorio corrente\n");
        printf("\tcls ou clear - limpar tela\n");
    }
    else
    {
        printf("[CLIENT-CLI] comando nao reconhecido\n");
    }
}

int main(int argc, char const *argv[])
{
    int socket = ConexaoRawSocket("enp5s0");
    //int socket = ConexaoRawSocket("lo");
    int wordCount;
    char buffer[1024];
    char **args;
    printf("\x1B[0m");

    // Clear screen
    clear();
    while(1)
    {
        printf("[CLIENT-CLI] > ");
        // Clean buffer
        memset(buffer, 0, sizeof(buffer));
        // fgets buffer
        fgets(buffer, BUFFER_SIZE, stdin);
        // Remove newline from buffer
        buffer[strcspn(buffer, "\n")] = 0;

        // Check for CTRL+D
        if(feof(stdin))
        {
            printf("\n[CLIENT-CLI] See ya!\n");
            // Free args using wordCount
            for(int i = 0; i < wordCount; i++)
            {
                free(args[i]);
            }
            free(args);
            exit(0);
        }

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
