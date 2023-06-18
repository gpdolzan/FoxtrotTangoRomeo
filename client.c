#include "rawsocket.h"
#include "packethandler.h"

int main(int argc, char const *argv[])
{
    int socket = ConexaoRawSocket("lo");
    // get input from user
    char input[63];
    printf("Enter a filename: ");
    scanf("%63s", input);
    sendFile(socket, input);
}