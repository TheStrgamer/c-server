#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 8080

char* sendHttp(int socket) {
    char *header = "HTTP/1.1 200 =OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n"
                    "\r\n";
    
    char *content = "<html>"
                    "<head><title>Test</title></head>"
                    "<body>"
                    "<h1>Hello there</h1>"
                    "</body>"
                    "</html>";

    send(socket,header,strlen(header),0);
    send(socket,content,strlen(content),0);
    
}

int getInput(int socket, struct sockaddr_in adress) {
    int server_fd, valread;
    char buffer[1024] = {0};
    
    while (1)
    {
        valread = read( socket, buffer, 1024);
        if (valread > 0) {
            printf("%s", buffer);
            char sign[8] = "return: ";
            int tot_len = strlen(sign) + strlen(buffer);
            char send_buffer[tot_len];
            strcpy(send_buffer,sign);
            strcat(send_buffer,buffer);                

            send(socket,send_buffer, strlen(send_buffer),0);
            memset(buffer,0,sizeof(buffer));
            memset(send_buffer,0,sizeof(send_buffer));
        }
        else {
            printf("Disconnection from %s on port %d\n", inet_ntoa((struct in_addr)adress.sin_addr), ntohs(adress.sin_port));
            close(socket);
            break;

        }
    }
    return 0;
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket, valread;
    struct sockaddr_in adress;
    int opt = 1;
    int addrlen = sizeof(adress);
    char buffer[1024] = {0};

    //creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)  {
        perror("socket failed loser");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsocketopt");
        exit(EXIT_FAILURE);
    }
    adress.sin_family = AF_INET;
    adress.sin_addr.s_addr = INADDR_ANY;
    adress.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&adress, sizeof(adress))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd,3) <0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d\n",PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&adress, (socklen_t*)&addrlen))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("Connection form %s on port %d\n", inet_ntoa((struct in_addr)adress.sin_addr), ntohs(adress.sin_port));
        sendHttp(new_socket);
        close(new_socket);

        
    }
    return 0;


}
