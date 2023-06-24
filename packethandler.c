#include "packethandler.h"

int createPacket(struct t_packet *packet, unsigned int tamanho, unsigned int sequencia, unsigned int tipo, unsigned char *dados)
{
    packet->marcadorInicio = 0x7E;
    packet->tamanho = tamanho;
    packet->sequencia = sequencia;
    packet->tipo = tipo;
    if(dados != NULL)
    {
        for(unsigned int i = 0; i < tamanho; i++)
        {
            packet->dados[i] = dados[i];
        }
    }
    packet->paridade = calculateParity(packet);
    maskPacket(packet);
    return 0;
}

// Mask packet data
int maskPacket(struct t_packet *packet)
{
    for(int i = 0; i < packet->tamanho; i++)
    {
        if(packet->dados[i] == 0x0081 ||packet->dados[i] == 0x0088)
            packet->dados[i] += 0xFF00;
    }
    return 0;
}

// Unmask packet data
int unmaskPacket(struct t_packet *packet)
{
    for(int i = 0; i < packet->tamanho; i++)
    {
        if(packet->dados[i] == 0xFF81 ||packet->dados[i] == 0xFF88)
            packet->dados[i] -= 0xFF00;
    }
    return 0;
}

void printPacket(struct t_packet *packet)
{
    printf("Marcador de inicio: %x\n", packet->marcadorInicio);
    printf("Tamanho: %d\n", packet->tamanho);
    printf("Sequencia: %d\n", packet->sequencia);
    printf("Tipo: %d\n", packet->tipo);
    // Loop through data printing each byte
    printf("Dados: ");
    for (int i = 0; i < packet->tamanho; i++)
    {
        printf("%x ", packet->dados[i]);
    }
    printf("\n");
    printf("Paridade: %d\n", packet->paridade);
}

unsigned int calculateParity(struct t_packet *packet)
{
    unsigned int parity = 0;
    parity ^= packet->tamanho;
    parity ^= packet->sequencia;
    parity ^= packet->tipo;
    if(packet->tamanho > 0)
    {
        for (int i = 0; i < packet->tamanho; i++)
        {
            parity ^= packet->dados[i];
        }
    }
    return parity;
}

int checkParity(struct t_packet *packet)
{
    int calculatedParity = calculateParity(packet);

    if(packet->paridade == calculatedParity)
        return 0;
    else
        return 1;
}

int sendPacket(int socket, struct t_packet *packet)
{
    int bytesSent = 0;
    bytesSent = write(socket, packet, sizeof(struct t_packet));
    if(bytesSent == 130)
        return 0;
    else
        return 1;
}

int readPacket(int socket, struct t_packet *packet, unsigned int timeout)
{
    // Create buffer size of packet_t
    struct t_packet *buffer = malloc(sizeof(struct t_packet));

    if(timeout > 0)
    {
        time_t start = time(NULL);
        time_t current = time(NULL);

        while(current <= start + timeout)
        {
            read(socket, buffer, sizeof(struct t_packet));
            if(buffer->marcadorInicio == 0x7E)
            {
                memcpy(packet, buffer, sizeof(struct t_packet));
                unmaskPacket(packet);
                free(buffer);
                return 0;
            }
            current = time(NULL);
        }
        free(buffer);
        return 1;
    }
    
    while(1)
    {
        read(socket, buffer, sizeof(struct t_packet));
        if(buffer->marcadorInicio == 0x7E)
        {
            memcpy(packet, buffer, sizeof(struct t_packet));
            unmaskPacket(packet);
            free(buffer);
            return 0;
        }
    }
}

int checkFileExists(char *filename)
{
    FILE *file = fopen(filename, "r");
    if(file != NULL)
    {
        fclose(file);
        return 0;
    }
    else
        return 1;
}

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

    // Loop de confirmacao de inicio
    printf("confirmation loop!\n");
    while(1)
    {
        // Send packet
        createPacket(&myPacket, filesize, 0, BACK_1_FILE, filename);
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
        
        // Check parity
        if(checkParity(&serverPacket) == 1)
        {
            // Send NACK
            createPacket(&myPacket, 0, 0, NACK, NULL);
            sendPacket(socket, &myPacket);
            continue;
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
            printf("reenviando pacote!\n");
            continue;
        }
    }

    printf("data loop!\n");
    // Loop de envio de dados
    while(1)
    {
        // Read file
        char *buffer = malloc(63 * sizeof(char));
        int readBytes = fread(buffer, sizeof(char), 63, file);

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
                sendPacket(socket, &myPacket);
                tries--;
            }
            else
            {
                tries = 8;
            }

            // Check parity
            if(checkParity(&serverPacket) == 1)
            {   
                // Send packet again
                continue;
            }
            
            if (serverPacket.tipo == OK && serverPacket.sequencia == seq)
            {
                if(seq < 63)
                    seq++;
                else
                    seq = 0;
                free(buffer);
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

    printf("end loop!\n");
    // Loop de confirmacao de fim
    while(1)
    {

        // Send packet
        createPacket(&myPacket, 0, seq, FIM_ARQ, NULL);
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

        // Check parity
        if(checkParity(&serverPacket) == 1)
        {
            // Send NACK
            createPacket(&myPacket, 0, 0, NACK, NULL);
            sendPacket(socket, &myPacket);
            continue;
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
    return 0;
}

int receiveFile(int socket, char* filename, int filesize)
{
    int expectedSequence = 0;
    struct t_packet clientPacket;
    struct t_packet serverPacket;
    int tries = 8;

    // Create file
    FILE *file = fopen(filename, "wb");

    // Send OK
    createPacket(&serverPacket, 0, expectedSequence, OK, NULL);

    // Loop de recebimento de bytes do arquivo
    printf("Loop de bytes de arquivo\n");
    sendPacket(socket, &serverPacket);
    expectedSequence++;
    while(1)
    {
        // Aguardar resposta (talvez timeout)
        if (readPacket(socket, &clientPacket, 1) == 1)
        {
            printPacket(&clientPacket);
            printf("expected seq: %d\n", expectedSequence);
            if(tries <= 0)
            {
                printf("Timeout dados do arquivo\n");
                fclose(file);
                return 1;
            }
            tries--;
            sendPacket(socket, &serverPacket);
            continue;
        }
        else
        {
            tries = 5;
        }
        // Recebeu mensagem, verifica OK ou NACK
        if(clientPacket.sequencia == expectedSequence && checkParity(&clientPacket) == 0)
        {
            if(clientPacket.tipo == DATA)
            {
                // Create a buffer
                char *buffer = malloc(clientPacket.tamanho);
                // Do a for loop to copy the data
                for(int i = 0; i < clientPacket.tamanho; i++)
                {
                    buffer[i] = clientPacket.dados[i];
                }
                // Write buffer to file
                fwrite(buffer, 1, clientPacket.tamanho, file);
                free(buffer);

                // Send OK
                createPacket(&serverPacket, 0, expectedSequence, OK, NULL);
                sendPacket(socket, &serverPacket);
                if(expectedSequence < 63)
                    expectedSequence++;
                else
                    expectedSequence = 0;
            }
            else if(clientPacket.tipo == FIM_ARQ)
            {
                printf("Recebi FIM_ARQ\n");
                fclose(file);
                // Send OK
                createPacket(&serverPacket, 0, expectedSequence, OK, NULL);
                sendPacket(socket, &serverPacket);
                break;
            }
            else if(clientPacket.tipo == NACK)
            {
                printf("Recebi NACK\n");
                // Enviar novamente
                createPacket(&serverPacket, 0, expectedSequence, NACK, NULL);
                sendPacket(socket, &serverPacket);
            }
        }
        else if(checkParity(&clientPacket) == 1)
        {
            printf("Paridade recebida: %d\n", clientPacket.paridade);
            // Enviar novamente
            createPacket(&serverPacket, 0, expectedSequence, NACK, NULL);
            sendPacket(socket, &serverPacket);
        }
    }
    return 0;
}