#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>

unsigned int pcc_total[95]; // number of printable characters in ascii. 126 - 32 + 1 = 95

// for each connection, this function will return the number of printable character in the stream
int handle_connection(int sockfd, char char_count[]) {
    int bytes_read = 0;
    char recv_buff[1024] = {0};
    unsigned int print_char_count = 0;
    int i;

    while(1) {
        bytes_read = read(sockfd, recv_buff, sizeof(recv_buff));
        if(bytes_read <= 0) {
            print_char_count = -1;
            break;
        }
        for (i = 0; i<bytes_read; i++) {
            if (recv_buff[i] >= 32 && recv_buff[i] <= 126) {

                char_count[recv_buff[i] - 32]++;
                print_char_count++;
            }
        }
    }
    return print_char_count;
}

int main(int argc, char* argv[]) {
    in_port_t port_num;
    struct sockaddr_in serv_addr;
    int listen_fd = -1;
    int conn_fd = -1;
    
    port_num = atoi(argv[1]);
    listen_fd = socket( AF_INET, SOCK_STREAM, 0 );
    memset( &serv_addr, 0, sizeof(struct sockaddr_in));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(10000);

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
}