#include "rawsocket.h"
#include "packethandler.h"

int main(int argc, char const *argv[])
{
    int socket = ConexaoRawSocket("enp5s0");
    // get input from user
    char input[63];
    // Set everything in buffer to 0
    memset(input, 0, sizeof(input));
    printf("Enter a filename: ");
    scanf("%63s", input);
    sendFile(socket, input);
}