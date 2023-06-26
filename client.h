#ifndef CLIENT_H
#define CLIENT_H

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <glob.h>

#define clear() printf("\033[H\033[J")

// define rec_arq function
int rec_arquivo(int socket, char *filename, int filesize);

#endif // CLIENT_H