#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define countof(array) ((signed int)(sizeof(array) / sizeof(array[0])))


enum
{
    STATE_UNUSED     = 0,
    STATE_CONNECTING = 1,
    STATE_WORKING    = 2,
};
typedef struct
{
    int  state;
    int  c_fd;    // Client FD
    int  s_fd;    // Server FD
} pair_t;


static int     l_sock = -1;  // Listening 
static pair_t  pairs[100];


int set_fd_flags  (int fd, int mask, int onoff)
{
  int  r;
  int  flags;

    /* Get current flags */
    flags = r = fcntl(fd, F_GETFL, 0);
    if (r < 0) return -1;

    /* Modify the value */
    flags &=~ mask;
    if (onoff != 0) flags |= mask;

    /* And try to set it */
    r = fcntl(fd, F_SETFL, flags);
    if (r < 0) return -1;

    return 0;
}

static void free_pair(int n, fd_set *rfds_p, fd_set *wfds_p)
{
  pair_t *pp = pairs + n;

    if (       pp->c_fd > 0)
    {
        FD_CLR(pp->c_fd, rfds_p);
        FD_CLR(pp->c_fd, wfds_p);
        close (pp->c_fd);
               pp->c_fd = -1;
    }
    if (       pp->s_fd > 0)
    {
        FD_CLR(pp->s_fd, rfds_p);
        FD_CLR(pp->s_fd, wfds_p);
        close (pp->s_fd);
               pp->s_fd = -1;
    }
    pp->state = STATE_UNUSED;
}

static int  copy_data(int from_fd, int to_fd)
{
  int            r;
  unsigned char  buff[1000];
  int            nbytes;

    r = read(from_fd, buff, sizeof(buff));
    if (r <= 0)     return -1;  // -1: some error; 0: EOF
    nbytes = r;

    r = write(to_fd,  buff, nbytes);
    if (r < nbytes) return -1;  // -1: some error; <nbytes: send buffer overflow

    return +1;
}

int main(int argc, char *argv[])
{
  int                 r;

  struct sockaddr_in  l_addr;
  int                 on     = 1;       /* "1" for REUSE_ADDR */

  const char         *hostname;
  struct hostent     *hp;
  in_addr_t           spechost;
  struct sockaddr_in  d_addr;  // Device ADDRess, prepared once at the beginning
  struct sockaddr_in  x_addr;  // eXpendable ADDRess struct, for connect(); copied from d_addr

  int                 maxfd;
  fd_set              rfds;
  fd_set              wfds;

  int                 n;

  struct sockaddr     in_addr;
  socklen_t           in_addrlen;

  int                 sock_err;
  socklen_t           errlen;

    if (argc != 4)
    {
        fprintf(stdout, "Usage: %s LOCAL_PORT_TO_LISTEN REMOTE_HOST REPOTE_PORT [MORE_FOR_DIAGNOSTICS]\n", argv[0]);
        exit(1);
    }

    // Create a listening socket
    l_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (l_sock < 0)
    {
        fprintf(stderr, "%s: error creating listening socket, %d, errno=%d:\"%s\"\n", argv[0], l_sock, errno, strerror(errno));
        exit(2);
    }
    setsockopt(l_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    l_addr.sin_family      = AF_INET;
    l_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    l_addr.sin_port        = htons(atoi(argv[1]));
    r = bind(l_sock, (struct sockaddr *)&l_addr, sizeof(l_addr));
    if (r < 0)
    {
        fprintf(stderr, "%s: error binding listening socket, %d, errno=%d:\"%s\"\n", argv[0], r, errno, strerror(errno));
        exit(2);
    }
    r = listen(l_sock, 5);
    if (r < 0)
    {
        fprintf(stderr, "%s: error listen()'ing socket, %d, errno=%d:\"%s\"\n", argv[0], r, errno, strerror(errno));
        exit(2);
    }
    set_fd_flags(l_sock, O_NONBLOCK, 1);

    // Prepare a destination address
    hostname = argv[2];
    /* Find out IP of the specified host */
    /* First, is it in dot notation (aaa.bbb.ccc.ddd)? */
    spechost = inet_addr(hostname);
    /* No, should do a hostname lookup */
    if (spechost == INADDR_NONE)
    {
        hp = gethostbyname(hostname);
        /* No such host?! */
        if (hp == NULL)
        {
            fprintf(stderr, "%s: unable to resolve host \"%s\"\n", argv[0], hostname);
            exit(2);
        }

        memcpy(&spechost, hp->h_addr, hp->h_length);
    }
    d_addr.sin_family      = AF_INET;
    d_addr.sin_port        = htons(atoi(argv[3]));
    memcpy(&d_addr.sin_addr, &spechost, sizeof(spechost));

    // Initialize file descriptors to be -1 (instead of default 0) tp re[resent "invalid" descriptors
    for (n = 0;  n < countof(pairs);  n++)
        pairs[n].c_fd = pairs[n].s_fd = -1;

    while (1)
    {
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_SET(l_sock, &rfds);
        maxfd = l_sock;

        for (n = 0;  n < countof(pairs);  n++)
            if (pairs[n].state != STATE_UNUSED)
            {
                FD_SET(pairs[n].c_fd, &rfds);
                if (maxfd < pairs[n].c_fd)
                    maxfd = pairs[n].c_fd;
                FD_SET(pairs[n].s_fd, pairs[n].state == STATE_CONNECTING? &wfds : &rfds);
                if (maxfd < pairs[n].s_fd)
                    maxfd = pairs[n].s_fd;
            }

        r = select(maxfd + 1, &rfds, &wfds, NULL/*no efds*/, NULL/*no timeout*/);
        if (r < 0)
        {
            fprintf(stderr, "%s: select() error, errno=%d:\"%s\"\n", argv[0], errno, strerror(errno));
            exit(2);
        }

        /* Check listening first... */
        if (FD_ISSET(l_sock, &rfds))
        {
            // Accept the connection...
            in_addrlen = sizeof(in_addr);
            r = accept(l_sock, &in_addr, &in_addrlen);
            // ...and check accept() result
            if      (r < 0)
            {
                fprintf(stderr, "%s: accept()=%d, errno=%d:\"%s\"\n", argv[0], r, errno, strerror(errno));
                goto END_ACCEPT;
            }
            // FD_SETSIZE is size of "fd_set" in bits, thus the maximum fd suitable for putting there is FD_SETSIZE-1
            if (r >= FD_SETSIZE)
            {
                fprintf(stderr, "%s: accept()=%d, which exceeds maximum value of %d\n", argv[0], r, FD_SETSIZE-1);
                close(r);
                goto END_ACCEPT;
            }
            set_fd_flags(r, O_NONBLOCK, 1);

            // Find a free cell
            for (n = 0;  n < countof(pairs);  n++)
                if (pairs[n].state == STATE_UNUSED) break;
            if (n >= countof(pairs))
            {
                fprintf(stderr, "%s: pairs[] is full, no cell for new client, closing...\n", argv[0]);
                close(r);
                goto END_ACCEPT;
            }
            pairs[n].state = STATE_CONNECTING;
            pairs[n].c_fd = r;

            // Create a socket for remote server connection...
            pairs[n].s_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (pairs[n].s_fd < 0)
            {
                fprintf(stderr, "%s: socket()=%d, errno=%d:\"%s\"\n", argv[0], r, errno, strerror(errno));
                free_pair(n, &rfds, &wfds);
                goto END_ACCEPT;
            }
            set_fd_flags(pairs[n].s_fd, O_NONBLOCK, 1);
            // ...and initiate a connection
            x_addr = d_addr;
            r = connect(pairs[n].s_fd, &x_addr, sizeof(x_addr));
            if (r < 0  &&  errno != EINPROGRESS)
            {
                fprintf(stderr, "%s: connect()=%d, errno=%d:\"%s\"\n", argv[0], r, errno, strerror(errno));
                free_pair(n, &rfds, &wfds);
                goto END_ACCEPT;
            }

  END_ACCEPT:;
        }

        /* Than check all the others */
        for (n = 0;  n < countof(pairs);  n++)
        {
            // Any data from client to server?
            if (pairs[n].state != STATE_UNUSED      &&
                FD_ISSET(pairs[n].c_fd, &rfds))
                if (copy_data(pairs[n].c_fd, pairs[n].s_fd) <= 0)
                    free_pair(n, &rfds, &wfds);
            // Any data from server to client?
            if (pairs[n].state == STATE_WORKING     &&
                FD_ISSET(pairs[n].s_fd, &rfds))
                if (copy_data(pairs[n].s_fd, pairs[n].c_fd) <= 0)
                    free_pair(n, &rfds, &wfds);
            // Connection to server had finished?
            if (pairs[n].state == STATE_CONNECTING  &&
                FD_ISSET(pairs[n].s_fd, &wfds))
            {
                // Okay, was the connection successful?
                errlen = sizeof(sock_err);
                getsockopt(pairs[n].s_fd, SOL_SOCKET, SO_ERROR, &sock_err, &errlen);
                errno = sock_err;
                if (errno != 0)
                {
                    fprintf(stderr, "%s: connect() finished, errno=%d:\"%s\"\n", argv[0], errno, strerror(errno));
                    free_pair(n, &rfds, &wfds);
                }
                pairs[n].state = STATE_WORKING;
            }

        }
    }

    return 0;
}

// mkfifo /tmp/fifo; ncat -k -l localhost 8012 -v  <<(cat /tmp/fifo) >>(tee >/dev/null /tmp/fifo)
