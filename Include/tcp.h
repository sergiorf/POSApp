#ifndef _TCP_H_
#define	_TCP_H_

typedef	unsigned char	byte;

int *TCP_connect  	(const char *host, int  port, byte timeout, int *errp);
int	TCP_send 		(int *sock, const char *data, int *errp	);
int	TCP_recv 		(int *sock,		  char *data, int *size, int tmFirst , int tmMax , int *errp);
int	TCP_disconnect 	(int *sock, int  *errp);

int	sendPkt	(int* hSock, char* pkt, int len);
int recvPkt	(int* hSock, char* pkt, int len , int timeout);

#endif/*_TCP_H_*/
