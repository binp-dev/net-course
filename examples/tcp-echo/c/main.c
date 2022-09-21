#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
    int listenfd;
    struct sockaddr_in servaddr;
    char buffer[256];

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        perror("Listen socket error");
        abort();
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8080);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        perror("Bind error");
        abort();
    }
    if (listen(listenfd, 16) != 0)
    {
        perror("Listen error");
        abort();
    }
    printf("Listening\n");

    for (;;)
    {
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);

        int connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);
        if (connfd < 0)
        {
            perror("Bind error");
            abort();
        }

        printf("Accepted\n");
        if (fork() == 0)
        {
            close(listenfd);
            for (;;)
            {
                int read_len = read(connfd, buffer, sizeof(buffer));
                if (read_len == 0)
                {
                    printf("Closed\n");
                    break;
                }
                else if (read_len < 0)
                {
                    perror("Read error");
                    abort();
                }
                for (;;)
                {
                    int offset = 0;
                    int write_len = write(connfd, buffer + offset, read_len - offset);
                    if (write_len == 0)
                    {
                        printf("Client socket unexpectedly closed\n");
                        exit(0);
                    }
                    else if (write_len < 0)
                    {
                        perror("Write error");
                        abort();
                    }

                    offset += write_len;
                    if (offset == read_len)
                    {
                        break;
                    }
                }
            }
            close(connfd);
            exit(0);
        }
        else
        {
            close(connfd);
        }
    }
}
