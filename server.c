#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 8080
#define INDEX_FILE "index.html"

void handleCommand(char content[1024], char command[256]) {
    char function[8];
    char argument[256];
    int func_index = 0;
    int arg_index = 0;
    function[0] = '\0';
    argument[0] = '\0';

    int func = 1;
    for (int i = 0; i<strlen(command); i++ ) {
        if (command[i] !=' ') {
            if (func) {
                function[func_index++] = command[i];

            } else {
                argument[arg_index++] = command[i];
            }
        } else {
            func = 0;
        }
    }
    function[func_index] = '\0';
    argument[arg_index] = '\0';

    printf("func: %s, arg: %s\n",function,argument);
    FILE *file;

    if (strstr(argument,".txt")) {
        file = fopen(argument, "r");
        if (file == NULL) {
            printf("Error opening file\n");
            return;
        }
    } else {
        printf("only txt is supported so far\n");
    }

    int bytes_read;
    if (strstr(function,"text")) {
        char buffer[256];
        memset(buffer,0,sizeof(buffer));
        while ((bytes_read = fread(buffer,1, sizeof(buffer)-1, file)) > 0) {
        strcat(content, buffer);

        }
        //fread(content,1, sizeof(content)-1, file);
        fclose(file);
        return;

    } else if (strstr(function,"list")) {
        char buffer[256];
        memset(buffer,0,sizeof(buffer));
        while ((bytes_read = fread(buffer,1, sizeof(buffer)-1, file)) > 0) {
        buffer[bytes_read]='\0';
        strcat(content, "<li>");
        strcat(content, buffer);
        strcat(content, "</li>\n");
        }

        return;

    } else {
        printf("Invalid function %s\n",function);
        return;
    }


}
int isHTML(FILE *file) {
    char buffer[256];
    fread(buffer,1,sizeof(buffer)-1,file);
    buffer[sizeof(buffer)-1] = '\0';
    return strstr(buffer, "<html>") != NULL || strstr(buffer, "<!DOCTYPE html>") != NULL;
}

int sendParsedHTML(FILE *file, int socket) {
    char buffer[1024];
    size_t bytes_read = 0;

    char html[2048];

    char command[256];
    int is_command=0;
    int command_index = 0;
    int html_index = 0;

    while ((bytes_read = fread(buffer,1, sizeof(buffer)-1, file)) > 0) {
        buffer[bytes_read]='\0';

        for (size_t i = 0; i<bytes_read; i++) {
            if (buffer[i] == '{') {

                is_command = 1;
                command_index=0;
            } else if (buffer[i] == '}') {
                is_command = 0;
                command[command_index] = '\0';
                
                char content[1024];
                memset(content,0,sizeof(content));
                handleCommand(content,command);
                for (int i = 0; i<sizeof(content);i++) {
                    html[html_index++] = content[i];

                }

                //printf("execute command:%s\n",command);
            }

            else if (is_command) {
                if (command_index < sizeof(command)-1) {
                    command[command_index++] = buffer[i];
                } else {
                    printf("Command too long, not allowed\n");
                    return 1;
                }
            } else {
                if (html_index<sizeof(html)-1) {
                    html[html_index++] = buffer[i];
                }
                //printf("%c",html[i]);

            }
        }
    }
    if (html_index>0) {
        html[html_index] = '\0';
        send(socket, html, html_index, 0);
    }
    return 0;
}


void sendHttp(int socket, const char *path) {
    FILE *file;
    int htmlFile = 0;
    if (strstr(path,".html")) {
        htmlFile = 1;
        char templates[12] = "templates/";
        char full_path[strlen(templates)+strlen(path)+1];
        full_path[0] = '\0';
        strcat(full_path, templates);
        strcat(full_path, path);
        printf("path is %s\n",full_path);
        file = fopen(full_path, "r");
    }
    else {
        file = fopen(path, "r");
    }

    if (file == NULL){
        char *header = "HTTP/1.1 200 =OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n"
                    "\r\n";
        send(socket,header,strlen(header),0);

        char *content = "<html>"
                        "<head><title>Test</title></head>"
                        "<body>"
                        "<h1>Could not find html file i am afriaid</h1>"
                        "<h3>That means one of us is incompetent</h3>"
                        "</body>"
                        "</html>";
        send(socket,content,strlen(content),0);
        return;
    }
    char *header = "HTTP/1.1 200 =OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n"
                    "\r\n";
    send(socket,header,strlen(header),0);

    if (htmlFile) {
        printf("sending html\n");
        sendParsedHTML(file, socket);
    } else{
        printf("Sending non html\n");
        char buffer[1024];
        size_t bytes_read;

        while ((bytes_read = fread(buffer,1, sizeof(buffer), file)) > 0) {
            send(socket, buffer, bytes_read, 0);
        }
    }
    fclose(file);
}

void get_request(int socket) {
    char buffer[1024];
    read( socket, buffer, 1024);
    buffer[strcspn(buffer, "\r\n")] = 0;

    char method[10], path[100];

    sscanf(buffer, "%s %s", method, path);

    if (path[0] == '/') {
        memmove(path, path+1, strlen(path));
    }
    printf("\nMethod is: %s\n",method);
    printf("User wants: %s\n",path);

    if (strstr(method, "GET")!= NULL) {
        if (strlen(path) == 0) {
            sendHttp(socket, INDEX_FILE);
            return;
        }
        sendHttp(socket, path);
    }


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
        get_request(new_socket);
        close(new_socket);

        
    }
    return 0;


}
