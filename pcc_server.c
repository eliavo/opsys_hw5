#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

unsigned int pcc_total[95] = {0}; // number of printable characters in ascii. 126 - 32 + 1 = 95

int conn_fd = -1; // we check this in the SIGINT handler

bool end_connection = false; // similar to yawn, we change in the SIGINT handler

void handler(int sig) {
    end_connection = true;
    if (conn_fd != -1)
        return;
    
    print_stats();
    exit(0);
}

// for each connection, this function will return the number of printable character in the stream
void handle_connection(int sockfd) {
    int bytes_read = 0;
    char recv_buff[1024] = {0};
    unsigned int print_char_count = 0;
    unsigned int temp_pcc_total[95] = {0};
    int i, N;

    bytes_read = read(sockfd, recv_buff, 4+1); //read 4 bytes and a newline
    if (bytes_read <= 0 || recv_buff[4] != '\n') {
        print_char_count = 0;
        print_char_count = htonl(print_char_count);
        write(sockfd, &print_char_count, sizeof(print_char_count));
        return;
    }

    N = (*(int*)recv_buff);
    N = ntohl(N); //number of bytes to expect to read, which is exactly N bytes
    
    for (;N>0; N-=bytes_read){
        bytes_read = read(sockfd, recv_buff, sizeof(recv_buff));
        if(bytes_read <= 0) {
            print_char_count = -1;
            break;
        }
        for (i = 0; i<bytes_read; i++) {
            if (recv_buff[i] >= 32 && recv_buff[i] <= 126) {

                temp_pcc_total[recv_buff[i] - 32]++;
                print_char_count++;
            }
        }
    }

    if (print_char_count > 0) {
        for (i = 0; i < 95; i++) {
            pcc_total[i] += temp_pcc_total[i];
        }
        
        // send print_char_count in network byte order to client
        print_char_count = htonl(print_char_count);
        write(sockfd, &print_char_count, sizeof(print_char_count));
    }

    return;
}

void print_stats(void) {
    int i;
    for (i=0; i<95; i++)
        printf("char '%c' : %u times\n", i+32, pcc_total[i]);
    
    return;
}

int main(int argc, char* argv[]) {
    in_port_t port_num;
    struct sockaddr_in serv_addr;
    int listen_fd = -1;
    
    if (argc != 2) {
        printf("Usage: %s <port_num>\n", argv[0]);
        return 1;
    }

    port_num = atoi(argv[1]);
    listen_fd = socket( AF_INET, SOCK_STREAM, 0 );
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    memset( &serv_addr, 0, sizeof(struct sockaddr_in));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(10000);
    
    // define the signal handler
    struct sigaction act;
    act.sa_handler = (void*) handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);


    if(0 != bind(listen_fd,
                 (struct sockaddr*) &serv_addr,
                 sizeof(struct sockaddr_in)))
    {
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        return 1;
    }
    
    if(0 != listen(listen_fd, 10))
    {
        printf("\n Error : Listen Failed. %s \n", strerror(errno));
        return 1;
    }
    
    // now we accept connections
    while (1) {
        // when accept is blocking, conn_fd is -1 and we can end connection
        conn_fd = accept(listen_fd, NULL, NULL);
        // now we are handling a connection and can't accept
        if (conn_fd < 0) {
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            return 1;
        }
        
        // handle the connection
        handle_connection(conn_fd);
        close(conn_fd);

        // finished handling a connection, now we can accept again
        conn_fd = -1;
        
        // if we received SIGINT while accept was blocking, we need to terminate
        if (end_connection) {
            print_stats();
            break;
        }
    }
    
    return 0;
}