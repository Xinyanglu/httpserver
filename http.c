#include "http.h"

int create_server(int port){
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0){
        fprintf(stderr, "socket() error");
        return -1;
    }
    printf("Socket created\n");

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if(inet_pton(AF_INET, LISTEN_ADDRESS, &server_addr.sin_addr) <= 0){
        close(server_socket);
        fprintf(stderr, "inet_pton() error\n");
        return -1;
    }

    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        close(server_socket);
        fprintf(stderr, "bind() error\n");
        return -1;
    }

    if(listen(server_socket, 5) < 0){
        close(server_socket);
        fprintf(stderr, "listen() error\n");
        return -1;
    }

    return server_socket;
}

int client_accept(int server_socket){
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    memset(&client_addr, 0, client_addr_len);
    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    if(client_socket < 0){
        fprintf(stderr, "accept() error\n");
        return -1;
    }

    char client_address_string[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_address_string, INET_ADDRSTRLEN);
    printf("Accepted connection from %s:%d\n", client_address_string, ntohs(client_addr.sin_port));
    return client_socket;
}

void parse_http(char* buffer, HttpRequest* request){
    char* token = buffer;
    // method token
    while(*token != ' '){
        token++;
    }
    strncpy(request->method, buffer, token - buffer);

    char* token2 = token+1;
    // url token
    while(*token2 != ' '){
        token2++;
    }
    strncpy(request->url, token+1, token2 - token - 1);
    
}

void http_headers(int client_socket, int code){
    char response[512];
    memset(response, 0 ,512);
    char* status;
    if(code == 200){
        status = "OK";
    }else if(code == 404){
        status = "Not Found";
    }else{
        status = "Internal Server Error";
        code = 500;
    }
    snprintf(response, 511, 
        "HTTP/1.0 %d %s\r\n"
        "Server: http.c\r\n"
        "Cache-Control: no-store, no-cache, max-age=0, private\r\n"
        "Content-Language: en\r\n"
        "Expires: -1\r\n"
        "X-Frame-Options: SAMEORIGIN\r\n",
        code, status
        );
    write(client_socket, response, strlen(response));
}

void http_response(int client_socket, char* content_type, char* content){
    char response[512];
    memset(response, 0, 512);
    snprintf(response, 511,
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n\r\n"
        "%s\r\n",
        content_type, strlen(content), content
        );

    write(client_socket, response, strlen(response));
}

void handle_client_connect(int server_socket, int client_socket){
    char buffer[512];
    memset(buffer, 0, 512);
    HttpRequest* request = malloc(sizeof(HttpRequest));
    memset(request, 0, sizeof(HttpRequest));

    int read_size = read(client_socket, buffer, 512);
    if(read_size < 0){
        fprintf(stderr, "read() error\n");
        close(client_socket);
        free(request);
        return;
    }

    parse_http(buffer, request);

    char* response;
    if(!strcmp(request->method, "GET") && (!strcmp(request->url, "/hello"))){
        response = "<html><h1>Hello, World!</h1></html>";
        http_headers(client_socket, 200);
        http_response(client_socket, "text/html", response);
    }else if (!strcmp(request->method, "GET") && (!strcmp(request->url, "/"))){
        response = "<html><body><h1>Routes: /hello </h1></body></html>";
        http_headers(client_socket, 200);
        http_response(client_socket, "text/html", response);
    }else{
        response = "File not found";
        http_headers(client_socket, 404);
        http_response(client_socket, "text/plain", response);
    }

    close(client_socket);
    free(request);
}

int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return -1;
    }

    char *endptr;
    int port = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number: %s\n", argv[1]);
        return -1;
    }

    int server_socket = create_server(port);
    if (server_socket < 0){
        return -1;
    }

    printf("Listening on %s:%d\n", LISTEN_ADDRESS, port);

    int client_socket;
    while(1){
        client_socket = client_accept(server_socket);

        if(client_socket < 0){
            continue;
        }

        if(!fork()){
            handle_client_connect(server_socket, client_socket);
        }
    }

    return 0;
}