#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netdb.h>

int send_file(int sockfd, char *filename) {
    int file_fd = -1, bytes_read, total_bytes_read = 0;
    unsigned int num_print_chars;
    unsigned int len;
    char send_buff[1024] = {0};
    struct stat st;

    // open filename and retrieve its length
    file_fd = open(filename, O_RDONLY);

    stat(filename, &st);
    len = st.st_size;
    len = htonl(len);
    
    write(sockfd, &len, sizeof(len));
    write(sockfd, "\n", sizeof(char));

    while ((bytes_read = read(file_fd, send_buff, 1024)) > 0) {
        total_bytes_read += bytes_read;
        write(sockfd, send_buff, bytes_read);
    }

    if (total_bytes_read != st.st_size) {
        print("Problem with file len\n");
        return -1;
    }

    read(sockfd, send_buff, 4);
    num_print_chars = ntohl((*(int* )send_buff));

    return num_print_chars;
}

int main(int argc, char* argv[]) {
    int sockfd = -1;
    char *filename;
    int port = 0, ip;
    unsigned int num_print_chars;
    struct sockaddr_in serv_addr;

    if (argc != 4) {
        printf("Usage: %s <ip_addr> <port> <file>\n", argv[0]);
        return 1;
    }
    
    ip = inet_pton(argv[1]);
    port = atoi(argv[2]);
    filename = argv[3];
    
    // connect to server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Error: socket creation failed\n");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; // IPv4
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = ip; 

    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error: connection failed\n");
        return 1;
    }
    
    num_print_chars = send_file(sockfd, filename);

    if (num_print_chars == -1)
        return 1;

    printf("# of printable characters: %u\n", num_print_chars);
    return 0;
}