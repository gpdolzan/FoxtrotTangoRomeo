#include "packethandler.h"

int createPacket(struct t_packet *packet, unsigned int tamanho, unsigned int sequencia, unsigned int tipo, unsigned char *dados)
{
    packet->marcadorInicio = 0x7E;
    packet->tamanho = tamanho;
    packet->sequencia = sequencia;
    packet->tipo = tipo;
    if(dados != NULL)
        memcpy(packet->dados, dados, tamanho);
    else
    {
        memset(packet->dados, 0, tamanho);
    }
    packet->paridade = calculateParity(packet);
    return 0;
}

void printPacket(struct t_packet *packet)
{
    printf("Marcador de inicio: %x\n", packet->marcadorInicio);
    printf("Tamanho: %d\n", packet->tamanho);
    printf("Sequencia: %d\n", packet->sequencia);
    printf("Tipo: %d\n", packet->tipo);
    printf("Dados: %s\n", packet->dados);
    printf("Paridade: %d\n", packet->paridade);
}

unsigned int calculateParity(struct t_packet *packet)
{
    unsigned int parity = 0;
    parity ^= packet->tamanho;
    parity ^= packet->sequencia;
    parity ^= packet->tipo;
    for (int i = 0; i < packet->tamanho; i++)
    {
        parity ^= packet->dados[i];
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
    if(bytesSent == 67)
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

int sendFile(int socket, char *filename)
{
    FILE *file = fopen(filename, "rb");
    struct t_packet packet;
    struct t_packet serverPacket;
    int sequence = 0;
    // Enviar solicitacao de inicio de envio de arquivo para servidor
    // Send BACK_1_FILE
    createPacket(&packet, strlen(filename), sequence, BACK_1_FILE, filename);
    while(1)
    {
        sendPacket(socket, &packet);
        // Aguardar resposta (talvez timeout)
        if (readPacket(socket, &serverPacket, 5) == 1)
        {
            printf("Timeout Receber confirmacao de inicio\n");
            return 1;
        }
        // Recebeu mensagem, verifica OK ou NACK
        if(serverPacket.sequencia == packet.sequencia)
        {
            if(serverPacket.tipo == OK)
            {
                printf("Recebi OK!\n");
                sequence++;
                break;
            }
            else if(serverPacket.tipo == NACK)
            {
                // Enviar novamente
                sendPacket(socket, &packet);
            }
        }
    }

    printf("Loop de bytes de arquivo\n");
    // Loop de envio de bytes do arquivo
    while(1)
    {
        int bytesRead = fread(packet.dados, 1, 63, file);

        // Print progress of file transfer based on total file size
        printf("Enviando %d\n", bytesRead);

        // Coloca esse buffer dentro do pacote
        createPacket(&packet, bytesRead, sequence, DATA, packet.dados);

        // Aguardar resposta (talvez timeout) (talvez NACK) (talvez OK)
        // Se recebeu NACK algo deu errado com o pacote ao enviar
        // Enviar pacote com dados do arquivo
        sendPacket(socket, &packet);
        while(1)
        {
            // Aguardar resposta (talvez timeout)
            if (readPacket(socket, &serverPacket, 5) == 1)
            {
                printf("Timeout enviar dados do arquivo\n");
                return 1;
            }
            // Recebeu mensagem, verifica OK ou NACK
            if(serverPacket.sequencia == packet.sequencia)
            {
                if(serverPacket.tipo == OK)
                {
                    printf("OK sequencia %d\n", sequence);
                    sequence++;
                    break;
                }
                else if(serverPacket.tipo == NACK)
                {
                    // Enviar novamente
                    sendPacket(socket, &packet);
                }
            }
        }

        if(bytesRead < 63)
        {
            fclose(file);
            break;
        }
    }

    // Enviar solicitacao de fim de envio de arquivo para servidor
    createPacket(&packet, 0, sequence, FIM_ARQ, NULL);
    while(1)
    {
        sendPacket(socket, &packet);
        // Aguardar resposta (talvez timeout)
        if (readPacket(socket, &serverPacket, 5) == 1)
        {
            printf("Timeout receber confirmacao de fim\n");
            return 1;
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
            }
        }
    }
    return 0;
}

int receiveFile(int socket, struct t_packet *packet)
{
    int expectedSequence = 0;
    char path[100];
    struct t_packet clientPacket;
    struct t_packet serverPacket;

    // Everything in path is 0
    memset(path, 0, sizeof(path));

    // Add path and packet->dados to path
    strcpy(path, "received/");
    strncat(path, packet->dados, packet->tamanho);

    // Create file
    FILE *file = fopen(path, "wb");

    // Send OK
    createPacket(&serverPacket, 0, expectedSequence, OK, NULL);

    // Loop de recebimento de bytes do arquivo
    printf("Loop de bytes de arquivo\n");
    sendPacket(socket, &serverPacket);
    expectedSequence++;
    while(1)
    {
        // Aguardar resposta (talvez timeout)
        if (readPacket(socket, &clientPacket, 5) == 1)
        {
            printf("Timeout dados do arquivo\n");
            fclose(file);
            // delete file
            remove(path);
            return 1;
        }
        // Recebeu mensagem, verifica OK ou NACK
        printf("%d e %d\n", clientPacket.sequencia, expectedSequence);
        if(clientPacket.sequencia == expectedSequence)
        {
            if(clientPacket.tipo == DATA)
            {
                printf("Recebi DATA: %d\n", clientPacket.tamanho);
                // Create a buffer
                char *buffer = malloc(clientPacket.tamanho);
                memcpy(buffer, clientPacket.dados, clientPacket.tamanho);
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
                sendPacket(socket, &serverPacket);
            }
        }
    }
    return 0;
}