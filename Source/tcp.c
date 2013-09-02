#ifndef _TCP_C_
#define _TCP_C_

#include "../Include/tcp.h"
#include <string.h>
#include <svc.h>
#include <ceif.h>
#include <ceifConst.h>
#include <errno.h>
#include <svc_net.h>	// TCP/IP library
#include <eoslog.h>

#define LogSerial dbprintf 

static int iSok = -1;

/****************************************************************************/
/*	Function:		TCP_connect												*/
/*	Description:	TCP connection											*/
/*	Data:			03/Jun/2011												*/
/*	Author:			Marcelo_R1												*/
/*	----------------------------------------------------------------------- */
/*	Revisions:																*/
/*	Date		Name		What											*/
/*	03/Jun/2011	Marcelo_R1	Creation										*/
/****************************************************************************/
int *TCP_connect(const char *host, int port, byte timeout, int *errp) {
	int *ptSok = (int*) 0;

	LogSerial("TCP_connect(): %s : %d : T=%ds", host, port, (int) timeout);

	iSok = socket(AF_INET, SOCK_STREAM, 0);

	LogSerial("socket(): %d", iSok);

	if (iSok >= 0) {
		int iSts;
		struct sockaddr_in socket_host;
		socket_host.sin_family = AF_INET;
		socket_host.sin_addr.s_addr = htonl(inet_addr((char *)host));
		socket_host.sin_port = htons(port);
		memset(socket_host.sin_zero, 0, 8);

		iSts = connect(iSok, (struct sockaddr *) &socket_host,
				sizeof(struct sockaddr_in));
		#ifdef DBG_CONN
		LogSerial("connect(): %d", iSts);
		#endif
		if (!iSts) // Connection established
		{
			ptSok = &iSok;
			*errp = 0;
		} else {
			int iErr = errno;
			socketclose(iSok);
			#ifdef DBG_CONN
			LogSerial("socketclose()");
			#endif
			iSok = -1;
			errno = iErr;
		}
	}
	if (iSok < 0) {
		*errp = errno;
	}

	#ifdef DBG_CONN
	LogSerial("TCP_connect(): errp: %d, errno: %d", *errp, errno);
	#endif

	return ptSok;
}

/****************************************************************************/
/*	Function:		TCP_send 												*/
/*	Description:	Send data bufer											*/
/*	Data:			03/Jun/2011												*/
/*	Author:			Marcelo_R1												*/
/*	----------------------------------------------------------------------- */
/*	Revisions:																*/
/*	Date		Name		What											*/
/*	03/Jun/2011	Marcelo_R1	Creation										*/
/****************************************************************************/
int TCP_send(int *sock, const char *data, int *errp) {
	int iSts = 0;

	struct timeval sendTime;
	sendTime.tv_sec = 0;
	sendTime.tv_usec = 8192;
	setsockopt(*sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &sendTime,
			sizeof(sendTime));
#ifdef DBG_CONN
	LogSerial("TCP_send(sock=%d): %d bytes", *sock, strlen(data));
#endif
	iSts = send(*sock, (char*) data, strlen(data), 0);
	*errp = errno;

	#ifdef DBG_CONN
	LogSerial("TCP_send(): errno: %d", *errp);
	#endif
	return iSts;
}

/****************************************************************************/
/*	Function:		TCP_recv 												*/
/*	Description:	Receive data bufer										*/
/*	Data:			03/Jun/2011												*/
/*	Author:			Marcelo_R1												*/
/*	----------------------------------------------------------------------- */
/*	Revisions:																*/
/*	Date		Name		What											*/
/*	03/Jun/2011	Marcelo_R1	Creation										*/
/****************************************************************************/
/*
int TCP_recv(int *sock, char *data, int *size, int tmFirst, int tmMax,
		int *errp) {
	int bytesRead = -1;
	errno = EINVAL;
	*errp = GPRS_STS_INVCALL;

	if (currSts == GPRS_STS_CONNECTED) {
		int iLen;
		struct timeval recvTime;
		recvTime.tv_sec = tmMax;
		recvTime.tv_usec = tmMax ? 0 : 10;

		iLen = setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &recvTime,
				sizeof(recvTime));
		#ifdef DBG_CONN
		LogSerial("TCP_recv(sock=%d): T = %lds Len = %d", *sock, tmMax, *size);
		#endif
		{
			char isFirst = TRUE;
			unsigned long ticks = read_ticks();
			*errp = 0;
			bytesRead = 0;
			do {
				iLen = recv(*sock, (char*) data + bytesRead, *size, 0);
				if (iLen < 0) {
					if (!bytesRead) {
						bytesRead = -1;
					}errno = GPRS_STS_ERROR;
					break;
				}
				if (!iLen) {
					if (!isFirst)
						break;
				} else {
					isFirst = FALSE;
					bytesRead += iLen;
					#ifdef DBG_CONN
					LogSerial("Received ... %d", bytesRead);
					#endif
					break;
				}
				if ((read_ticks() - ticks) >= (tmMax * TICKS_PER_SEC)) {
					#ifdef DBG_CONN
					LogSerial("Recv timeout (%lds)", tmMax);
					#endif

					*errp = GPRS_STS_TIMEOUT;
					errno = ETIMEDOUT;
					break;
				}
				SVC_WAIT(10);
			} while (!bytesRead);
//			while( bytesRead < *size);
		}
		#ifdef DBG_CONN
		LogSerial("recv(): %d", iLen);
		#endif
	}
	if (bytesRead >= 0) {
		*size = bytesRead;

		if (gbAddPacketLen) {
			memmove((char*) data + 0, (char*) data + 2, bytesRead -= 2);
		}
		//	GPRS_yield( errp)	 ;
		*errp = 0;
	} else {
		int auxp;
		*errp = errno;
		GPRS_updateStatus(&auxp);
		*size = 0;
	}
	#ifdef DBG_CONN
	LogSerial("TCP_recv(): currSts: %d, errp: %d, errno: %d", currSts, *errp,
			errno);
	#endif

	return currSts;
}
*/

/****************************************************************************/
/*	Function:		TCP_disconnect											*/
/*	Description:	TCP connection											*/
/*	Data:			03/Jun/2011												*/
/*	Author:			Marcelo_R1												*/
/*	----------------------------------------------------------------------- */
/*	Revisions:																*/
/*	Date		Name		What											*/
/*	03/Jun/2011	Marcelo_R1	Creation										*/
/****************************************************************************/
/*
int TCP_disconnect(int *sock, int *errp) {
	int iSts = 0;
	errno = EINVAL;
	*errp = GPRS_STS_INVCALL;
	if (*sock >= 0) {
		int iSts = socketclose(*sock);
		if (iSts >= 0) {
			*sock = -1;
			errno = 0;
			currSts = GPRS_STS_ATTACHED;
		}
	}
	*errp = errno;
	iSts = iSts;

	#ifdef DBG_CONN
	LogSerial("TCP_disconnect(): currSts: %d, errp: %d, errno: %d", currSts,
			iSts, errno);
	#endif

	return currSts;
}
*/

#endif	/* _TCP_C_	*/