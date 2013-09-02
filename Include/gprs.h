#ifndef _GPRS_H_
#define	_GPRS_H_

#define	GPRS_STS_OFF			0
#define	GPRS_STS_REGISTERING	1
#define	GPRS_STS_REGISTERED		2
#define	GPRS_STS_ATTACHING		3
#define	GPRS_STS_ATTACHED		4
#define	GPRS_STS_CONNECTED		5

#define	GPRS_STS_SUCCESS		 0
#define	GPRS_STS_ERROR			-1
#define	GPRS_STS_INVCALL		-2
#define	GPRS_STS_SIMERROR		-3
#define	GPRS_STS_NOSIM			-4
#define	GPRS_STS_TIMEOUT		-5

typedef	unsigned char	byte;

// GPRS functions prototypes
/*
int GPRS_updateStatus		(int *errp);
*/
int GPRS_yield			(int *errp);
int	GPRS_init			(int *errp);
int GPRS_connect();
/*int	GPRS_hangup 		(int *errp);
int	GPRS_close			(int *errp);
int *TCP_connect 			(			const char *host, int  port, byte timeout, int *errp);
int	TCP_send 			(int *sock, const char *data, int *size, int *errp	);
int	TCP_recv 			(int *sock,		  char *data, int *size, int tmFirst , int tmMax , int *errp);
int	TCP_disconnect 		(int *sock, int  *errp);

int	sendPkt	(int* hSock, char* pkt, int len);
int recvPkt	(int* hSock, char* pkt, int len , int timeout);
*/

#endif/*_GPRS_H_*/
