#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define LISTEN_ADDRESS "0.0.0.0"

typedef struct{
    char method[8];
    char url[128];
}HttpRequest;

int main(int, char**);
int create_server(int);
int client_accept(int);
void handle_client_connect(int, int);
void parse_http(char*, HttpRequest*);