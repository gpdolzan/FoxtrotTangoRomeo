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

int sendFileWrapper(int socket, char *filename, int type)
{
    struct t_packet packet;
    struct t_packet serverPacket;
    FILE *file = fopen(filename, "r");
    int tries = 8;
    if(checkFileExists(filename) == 1)
    {
        printf("Arquivo %s nao existe!\n", filename);
        return 1;
    }

    createPacket(&packet, strlen(filename), 0, BACK_1_FILE, filename);
    sendPacket(socket, &packet);
    while(1)
    {
        // Receive OK
        while(1)
        {
            if(tries <= 0)
            {
                printf("Time exceeded!\n");
                fclose(file);
                return 1;
            }
            if(readPacket(socket, &serverPacket, 1) == 0)
            {
                if(checkParity(&serverPacket) == 1)
                {
                    printf("Recebi paridade errada, enviando solicitacao de novo!\n");
                    sendPacket(socket, &packet);
                }
                else
                    break;
            }
            else
                tries--;
        }
        // Recebeu mensagem, verifica OK ou NACK
        if(serverPacket.sequencia == packet.sequencia)
        {
            if(serverPacket.tipo == OK)
            {
                break;
            }
            else if(serverPacket.tipo == NACK)
            {
                // Enviar novamente
                sendPacket(socket, &packet);
                printf("Nao foi possivel estabelecer conexao!\n");
                return 1;
            }
        }
    }

    if (sendFile(socket, file) == 0)
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

int sendFile(int socket, FILE *file)
{
    struct t_packet packet;
    struct t_packet serverPacket;
    int sequence = 0;
    int tries = 5;

    printf("Loop de bytes de arquivo\n");
    // print file path
    printf("Enviando arquivo\n");
    // Loop de envio de bytes do arquivo
    while(1)
    {
        // Create buffer
        char *buffer = malloc(63);
        int bytesRead = fread(buffer, 1, 63, file);

        // Coloca esse buffer dentro do pacote
        createPacket(&packet, bytesRead, sequence, DATA, buffer);

        // Aguardar resposta (talvez timeout) (talvez NACK) (talvez OK)
        // Se recebeu NACK algo deu errado com o pacote ao enviar
        // Enviar pacote com dados do arquivo
        sendPacket(socket, &packet);
        while(1)
        {
            // Aguardar resposta (talvez timeout)
            if (readPacket(socket, &serverPacket, 1) == 1)
            {
                if(tries <= 0)
                {
                    printf("Timeout enviar dados do arquivo\n");
                    fclose(file);
                    return 1;
                }
                tries--;
                sendPacket(socket, &packet);
                continue;
            }
            else
            {
                tries = 5;
            }
            // Recebeu mensagem, verifica OK ou NACK
            if(serverPacket.sequencia == packet.sequencia && checkParity(&serverPacket) == 0)
            {
                if(serverPacket.tipo == OK)
                {
                    if(sequence < 63)
                        sequence++;
                    else
                        sequence = 0;
                    break;
                }
                else if(serverPacket.tipo == NACK)
                {
                    // Enviar novamente
                    printf("paridade: %d\n", packet.paridade);
                    sendPacket(socket, &packet);
                }
            }
            else if(checkParity(&serverPacket) == 1)
            {
                // Enviar novamente
                printf("Deu ruim a paridade!\n");
                sendPacket(socket, &packet);
            }
        }

        if(bytesRead < 63)
            break;
    }

    // Enviar solicitacao de fim de envio de arquivo para servidor
    createPacket(&packet, 0, sequence, FIM_ARQ, NULL);
    while(1)
    {
        sendPacket(socket, &packet);
        // Aguardar resposta (talvez timeout)
        if (readPacket(socket, &serverPacket, 1) == 1)
        {
            printf("Timeout receber confirmacao de fim\n");
            fclose(file);
            return 1;
        }
        // Recebeu mensagem, verifica OK ou NACK
        if(serverPacket.sequencia == packet.sequencia && checkParity(&serverPacket) == 0)
        {
            if(serverPacket.tipo == OK)
            {
                fclose(file);
                break;
            }
            else if(serverPacket.tipo == NACK)
            {
                // Enviar novamente
                sendPacket(socket, &packet);
            }
        }
        else if(checkParity(&serverPacket) == 1)
        {
            // Enviar novamente
            sendPacket(socket, &packet);
        }
    }
    return 0;
}

int receiveFile(int socket, FILE* file)
{
    int expectedSequence = 0;
    struct t_packet clientPacket;
    struct t_packet serverPacket;
    int tries = 8;
    // Create file

    printf("DATA LOOP\n");
    while(1)
    {
        // Aguardar resposta (talvez timeout)
        if (readPacket(socket, &clientPacket, 1) == 1)
        {
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
                // For loop
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