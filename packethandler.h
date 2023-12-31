#ifndef PACKETHANDLER_H
#define PACKETHANDLER_H

#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#define MAX_PACKET_SIZE 63

#define BACK_1_FILE 0 // 0000
#define BACK_PLUS_1_FILE 1 // 0001
#define REC_1_ARQ // 0010
#define REC_GROUP_ARQ// 0011
#define CD_LOCAL // 0100
#define VERIFICA_BACK // 0101
#define DATA 8 // 1000
#define FIM_ARQ 9 // 1001
#define FIM_GRUPO_ARQ 10 // 1010
#define ERRO 12 // 1100
#define OK 13 // 1101
#define ACK 14 // 1110
#define NACK 15 // 1111

// Packet struct
struct t_packet
{
    unsigned char marcadorInicio; // 8 bits
    unsigned int tamanho : 6; // 6 bits -> maior tamanho de dados = 63 bytes
    unsigned int sequencia : 6; // 6 bits
    unsigned int tipo : 4; // 4 bits
    unsigned char dados[63]; // 0 a 63 bytes
    unsigned char paridade; // 8 bits
}__attribute__((packed));

int createPacket(struct t_packet *packet, unsigned int tamanho, unsigned int sequencia, unsigned int tipo, unsigned char *dados);
void printPacket(struct t_packet *packet);
unsigned int calculateParity(struct t_packet *packet);
int checkParity(struct t_packet *packet);
int sendPacket(int socket, struct t_packet *packet);
int readPacket(int socket, struct t_packet *packet, unsigned int timeout);
int checkFileExists(char *filename);
int sendFile(int socket, char *filename); // PROJECT CRITICAL
int receiveFile(int socket, struct t_packet *packet); // PROJECT CRITICAL


#endif // PACKETHANDLER_H