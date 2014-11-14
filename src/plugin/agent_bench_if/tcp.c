#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include "benchapi.h"

int safe_tcp_recv (int sockfd, void *buf, int bufsize)
{
	int cur_len;
recv_again:
	cur_len = recv (sockfd, buf, bufsize, 0);
	//closed by client
	if (cur_len ==	0) {
		TRACE_LOG ("connection closed by peer, fd=%d", sockfd);
		return 0;
	} else if (cur_len == -1) {
		if (errno == EINTR)
			goto recv_again;
		else 
			ERROR_LOG ("recv error, fd=%d, %m", sockfd);
	}

	return cur_len;
}


int safe_tcp_recv_n (int sockfd, void *buf, int total)
{
	int recv_bytes, cur_len;

	for (recv_bytes = 0; recv_bytes < total; recv_bytes += cur_len) {
		cur_len = recv (sockfd, buf + recv_bytes, total - recv_bytes, 0);
		//closed by client
		if (cur_len ==	0) {
			TRACE_LOG ("connection closed by peer, fd=%d", sockfd);
			return -1;
		} else if (cur_len == -1) {
			if (errno == EINTR)
				cur_len = 0;
			else if (errno == EAGAIN) {
				TRACE_LOG ("recv %d bytes from fd=%d", recv_bytes, sockfd);
				return recv_bytes;
			}
			else {
				ERROR_RETURN (("recv tcp packet error, fd=%d, %m", sockfd), -1);
			}
		}
	}

	return recv_bytes;		
}

/*
 * safe_tcp_send: send wrapper for non-block tcp connection
 * @return :	
 */
int safe_tcp_send_n (int sockfd, void *buf, int total)
{
    int send_bytes = 0;

    send_bytes = send (sockfd, buf + send_bytes, total - send_bytes, 0);

    //closed by client
    if (send_bytes == 0)
    {
        ERROR_LOG ("send tcp packet error, fd=%d, %s", sockfd, strerror(errno));
        return -1;
    }

    if (send_bytes == -1)
    {
        switch (errno)
        {
        case EINTR:
        case EAGAIN:
        case EINPROGRESS:
            break;

        default:
            TRACE_LOG ("send tcp packet error, fd=%d, %m", sockfd);
            return -1;
        }
    }

    TRACE_LOG ("send %d bytes, fd=%d", send_bytes, sockfd);
    return send_bytes;		
}
/*
 * safe_tcp_listen: create tcp socket to listen
 * @return :	listen socket description
 *	> 0  ok
 * 	= -1 error
 */
int safe_socket_listen (struct sockaddr_in *servaddr, int type)
{
	int listenfd;
	int reuse_addr = 1;
	
	if ((listenfd = socket (AF_INET, type, 0)) == -1) 
		ERROR_RETURN (("socket error, %s", strerror (errno)), -1);

	setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof reuse_addr);
	if (bind (listenfd, (struct sockaddr *)servaddr, sizeof(struct sockaddr)) == -1) {
		ERROR_LOG ("bind error, %s", strerror (errno));
		close (listenfd);
		return -1;
	} 
	
	if (type == SOCK_STREAM && listen (listenfd, 256) == -1) {
		ERROR_LOG ("listen error, %s", strerror (errno));
		close (listenfd);
		return -1;
	} 
	
	return listenfd;
}

/*
 * safe_tcp_accept:	accept a client tcp connection
 * @return :	new socket description
 *	> 0  ok
 * 	= -1 error
 */
int safe_tcp_accept (int sockfd, struct sockaddr_in *peer)
{
	socklen_t peer_size;
	int newfd           = -1;
    int accept_count    = 256;
    int curr_count      = 0;

	for (curr_count = 0; curr_count < accept_count; ++curr_count)
    {
		peer_size = sizeof(struct sockaddr_in); 

		if ((newfd = accept(sockfd, (struct sockaddr *)peer, &peer_size)) < 0)
        {
			if (errno == EINTR)
                continue;/* back to for () */

            if (errno == EAGAIN)
				break;

			ERROR_RETURN (("accept failed, listenfd=%d", sockfd), -1);
		}

		break;
	}

	TRACE_LOG ("accept connection from %s:%u, listen fd=%u, accepted fd=%u", 
			inet_ntoa(peer->sin_addr), ntohs(peer->sin_port), sockfd, newfd); 

	return newfd;
}

int safe_tcp_connect (const char *ipaddr, u_short port)
{
	int sockfd;
	struct sockaddr_in peer;

	bzero (&peer, sizeof (peer));
	peer.sin_family = AF_INET;
	peer.sin_port = htons(port);
	if (inet_pton (AF_INET, ipaddr, &peer.sin_addr) <= 0) 
		ERROR_RETURN (("inet_pton %s failed, %m", ipaddr), -1);

	if ((sockfd = socket (PF_INET, SOCK_STREAM, 0)) < 0) 
		ERROR_RETURN (("socket failed, %s", strerror (errno)), -1);

	if (connect (sockfd, (const struct sockaddr *)&peer, sizeof (peer))) {
		close (sockfd);
		ERROR_RETURN (("connect %s:%u failed, %m", ipaddr, port), -1);
	}

	return sockfd;
}
