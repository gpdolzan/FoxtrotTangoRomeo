#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BACK_1_FILE 0 // 0000

// Packet struct
struct t_packet
{
    unsigned char marcadorInicio; // 8 bits
    unsigned int tamanho : 6; // 6 bits -> maior tamanho de dados = 63 bytes
    unsigned int sequencia : 6; // 6 bits
    unsigned int tipo : 4; // 4 bits
    unsigned short dados[63]; // 0 a 126 bytes
    unsigned char paridade; // 8 bits
}__attribute__((packed));

int createPacket(struct t_packet *packet, unsigned int tamanho, unsigned int sequencia, unsigned int tipo, unsigned char *dados);
int maskPacket(struct t_packet *packet);
int unmaskPacket(struct t_packet *packet);
void printPacket(struct t_packet *packet);
unsigned int calculateParity(struct t_packet *packet);
int checkParity(struct t_packet *packet);