#include "packethandler.h"

int sendFile(int socket, char *filename, int filesize)
{
    // Create myPacket and serverPacket
    struct t_packet myPacket; // Packets I will send
    struct t_packet serverPacket; // Packets I will receive
    FILE *file = fopen(filename, "rb"); // Open file to read
    int tries = 8; // Number of tries to send a packet
    int seq = 0; // Sequence number

    // Check if file exists
    if(file == NULL)
    {
        printf("File does not exist\n");
        // SERVER WILL SEND ERROR
        return 1;
    }

    // Create BACK_1_FILE packet
    createPacket(&myPacket, filesize, 0, BACK_1_FILE, filename);

    // Loop de confirmacao de inicio
    while(1)
    {
        // Send packet
        sendPacket(socket, &myPacket);
        // Aguardar resposta (talvez timeout)
        if (readPacket(socket, &serverPacket, 1) == 1)
        {
            if(tries <= 0)
            {
                printf("Time exceeded, initial confirmation\n");
                return 1;
            }
            printf("timeout\n");
            tries--;
        }
        else
        {
            tries = 8;
        }
        
        if (serverPacket.tipo == OK && serverPacket.sequencia == seq)
        {
            if(seq < 63)
                seq++;
            else
                seq = 0;
            break;
        }
        else if (serverPacket.tipo == NACK && serverPacket.sequencia == seq)
        {
            // Send packet again
            continue;
        }
    }

    // Loop de envio de dados
    while(1)
    {
        // Read file
        char *buffer = malloc(63 * sizeof(char));
        int readBytes = fread(buffer, sizeof(char), BUFFER_SIZE, file);

        // Cria pacote com base no buffer
        createPacket(&myPacket, readBytes, seq, DATA, buffer);

        // Aguardar resposta
        while(1)
        {
            // Envia pacote
            sendPacket(socket, &myPacket);
            // Aguardar resposta (talvez timeout)
            if (readPacket(socket, &serverPacket, 1) == 1)
            {
                if(tries <= 0)
                {
                    printf("Time exceeded, file data!\n");
                    return 1;
                }
                tries--;
            }
            else
            {
                tries = 8;
            }
            
            if (serverPacket.tipo == OK && serverPacket.sequencia == seq)
            {
                if(seq < 63)
                    seq++;
                else
                    seq = 0;
                break;
            }
            else if (serverPacket.tipo == NACK && serverPacket.sequencia == seq)
            {
                // Send packet again
                continue;
            }
        }

        // Check if bytes sent are less than 63
        if(readBytes < 63)
        {
            break;
        }
    }

    // Loop de confirmacao de fim
    while(1)
    {
        // Create FIM_ARQ packet
        createPacket(&myPacket, 0, seq, FIM_ARQ, NULL);
        while(1)
        {
            // Send packet
            sendPacket(socket, &myPacket);
            // Aguardar resposta (talvez timeout)
            if (readPacket(socket, &serverPacket, 1) == 1)
            {
                if(tries <= 0)
                {
                    printf("Time exceeded, final confirmation\n");
                    return 1;
                }
                tries--;
            }
            else
            {
                tries = 8;
            }
        
            if (serverPacket.tipo == OK && serverPacket.sequencia == seq)
            {
                if(seq < 63)
                    seq++;
                else
                    seq = 0;
                break;
            }
            else if (serverPacket.tipo == NACK && serverPacket.sequencia == seq)
            {
                // Send packet again
                continue;
            }
        }
    }
    return 0;
}