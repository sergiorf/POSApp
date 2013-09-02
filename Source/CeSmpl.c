#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// for tcpip library
#include <svc_net.h>

// for EOS log
#include <eoslog.h>

// CEIF
#include <ceif.h>

// OpenSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include "../Include/CeSmpl.h"
#include "../Include/http_lib.h"
#include "../Include/Util.h"
#include "../Include/tcp.h"
#include "../Include/gprs.h"
#include "../Include/startup_screen.h"
#include "../Include/db.h"
#include "../Include/main_screen.h"

// Globals
int g_NICount;						// Global for number of NWIF supported
int g_EthStartMode;

unsigned char* g_NIInfo;
appConfig_t g_AppConfig;

char g_szMsgBuff [64] = {'\0'};		// Screen message buffer

// Device Handles
int g_conHandle = -1;				// Console Handle
int g_gprsHandle = -1;				// GPRS Handle
int g_gsmHandle = -1;

int g_LinkState = LINK_DISCONNECTED;

char g_szVXCEVersion [CE_VERSION_STR_LEN] = {'\0'};
char g_szCEIFVersion [CEIF_VERSION_STR_LEN] = {'\0'};
char g_rtClock [RT_CLOCK_TIME_LEN] = {'\0'};

BIO*		g_Conn;
SSL*		g_SSL;
SSL_CTX*	g_CTX;
int			g_rc;
long		g_err;

extern int currSts;

int main(int argc, char** argv)
{
	CHECK(ERROR!=(g_conHandle=open (DEV_CONSOLE, 0)));
	LoadConfig();			
	CHECK(SUCCESS==startupScreenInit());
	return 0;
}

int InitComEngine (void)
{
	// Register this appication with CommEngine

	int retVal = 0;

	LOG_PRINTFF (LOG_APPFILTER, "**** InitComEngine Start ****");

	clrscr ();
	sprintf (g_szMsgBuff, "\fREGISTER TO\nCOMM ENGINE");
	write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 1);
	SVC_WAIT (SCREEN_DELAY);

	retVal = ceRegister ();

	LOG_PRINTFF (LOG_APPFILTER, "ceRegister retVal: %d", retVal);

	if (retVal < 0) {
		switch (retVal) {
			case ECE_REGAGAIN:
				ceUnregister ();	// Unregister first before
				break;
		}
		return retVal;
	}

	return 0;
}

int GetConnStatus (int iNWIFHandle)
{	
	stNI_IPConfig ipConfig;
	unsigned int pLen;

	LOG_PRINTFF (LOG_APPFILTER, "---> GetConnStatus Start");

	ceGetNWParamValue (iNWIFHandle, IP_CONFIG, &ipConfig, sizeof (stNI_IPConfig), &pLen);

	LOG_PRINTFF (LOG_APPFILTER, "ipConfig.ncIsConnected: %d", ipConfig.ncIsConnected);

	LOG_PRINTFF (LOG_APPFILTER, "GetConnStatus End <---");

	return ipConfig.ncIsConnected;
}
/*
int InitCEEvents (void)
{	
	int retVal = 0;

	LOG_PRINTFF (LOG_APPFILTER, "**** InitCEEvents Start ****");

	// Subscribe notification events from CommEngine
	retVal = ceEnableEventNotification ();
	LOG_PRINTFF (LOG_APPFILTER, "[%s] ceEnableEventNotification retVal: %d", __FUNCTION__, retVal);

	if (retVal < 0) {
		LOG_PRINTFF (LOG_APPFILTER, "[%s] ceEnableEventNotification FAILED.  retVal: %d", __FUNCTION__, retVal);
		return retVal;
	}

	// Handle Signal Events
	retVal = ceSetSignalNotificationFreq (CE_SF_HIGH);
	LOG_PRINTFF (LOG_APPFILTER, "[%s] ceSetSignalNotificationFreq retVal: %d", __FUNCTION__, retVal);

	return 0;
}
*/

// Checks how many network interfaces (NWIF) are available
// and will be stored in the stNIInfo structure array
// Returns the number of supported NWIF

int InitNWIF (void)
{
	int retVal = 0;
	unsigned int nwInfoCount = 0;

	LOG_PRINTFF (LOG_APPFILTER, "**** InitNWIF Start ****");

	// Get the total number of network interface from this terminal (NWIF)
	g_NICount = ceGetNWIFCount ();

	LOG_PRINTFF (LOG_APPFILTER, "ceGetNWIFCount retVal: %d", g_NICount);

	if (g_NICount < 0) {
		LOG_PRINTFF (LOG_APPFILTER, "ceGetNWIFCount FAILED.  retVal: %d", retVal);
		return -1;
	}

	g_NIInfo = (unsigned char*) malloc (g_NICount * sizeof (stNIInfo));

	retVal = ceGetNWIFInfo ( (stNIInfo*) g_NIInfo, g_NICount, &nwInfoCount);

	LOG_PRINTFF (LOG_APPFILTER, "ceGetNWIFInfo retVal: %d", retVal);

	if (retVal < 0) {
		LOG_PRINTFF (LOG_APPFILTER, "ceGetNWIFInfo FAILED.  retVal: %d", retVal);
		return -1;
	}

	ListNWIF ((stNIInfo*)g_NIInfo, g_NICount);

	return retVal;
}

void GetIPAddress (void)
{	
	stNI_IPConfig ipConfig;
	unsigned int retLen;
	char szIPAddress [MAX_IP_LENGTH] = {'\0'};
	unsigned char* szIPBuff;

	clrscr ();

	ceGetNWParamValue(0, IP_CONFIG, (void *)&ipConfig, sizeof(ipConfig), &retLen);

	szIPBuff = (unsigned char*) &ipConfig.ncIPAddr;
	sprintf (szIPAddress, "IP: %d.%d.%d.%d", szIPBuff [0], szIPBuff [1], szIPBuff [2], szIPBuff [3]);
	LOG_PRINTFF (LOG_APPFILTER, "%s", szIPAddress);
	write_at (szIPAddress, strlen (szIPAddress), 1, 1);

	szIPBuff = (unsigned char*) &ipConfig.ncSubnet;
	sprintf (szIPAddress, "SNET: %d.%d.%d.%d", szIPBuff [0], szIPBuff [1], szIPBuff [2], szIPBuff [3]);
	LOG_PRINTFF (LOG_APPFILTER, "%s", szIPAddress);
	write_at (szIPAddress, strlen (szIPAddress), 1, 2);

	szIPBuff = (unsigned char*) &ipConfig.ncGateway;
	sprintf (szIPAddress, "GW: %d.%d.%d.%d", szIPBuff [0], szIPBuff [1], szIPBuff [2], szIPBuff [3]);
	LOG_PRINTFF (LOG_APPFILTER, "%s", szIPAddress);
	write_at (szIPAddress, strlen (szIPAddress), 1, 3);

	szIPBuff = (unsigned char*) &ipConfig.ncDNS1;
	sprintf (szIPAddress, "DNS1: %d.%d.%d.%d", szIPBuff [0], szIPBuff [1], szIPBuff [2], szIPBuff [3]);
	LOG_PRINTFF (LOG_APPFILTER, "%s", szIPAddress);
	write_at (szIPAddress, strlen (szIPAddress), 1, 4);

	szIPBuff = (unsigned char*) &ipConfig.ncDNS2;
	sprintf (szIPAddress, "DNS2: %d.%d.%d.%d", szIPBuff [0], szIPBuff [1], szIPBuff [2], szIPBuff [3]);
	LOG_PRINTFF (LOG_APPFILTER, "%s", szIPAddress);
	write_at (szIPAddress, strlen (szIPAddress), 1, 5);

	WaitForKeyPress (8);
}

void GetMACAddress (void)
{	
	char szBuff [64] = {'\0'};
	
	char szMacValue [64] = {'\0'};
	unsigned int macValueLen;

	char szLinkValue [64] = {'\0'};
	unsigned int linkValueLen;

	clrscr ();

	ceGetDDParamValue(0, "GET_MAC", sizeof(szMacValue), szMacValue, &macValueLen);
	ceGetDDParamValue (0, "GET_LINK_SPEED", sizeof (szLinkValue), szLinkValue, &linkValueLen);

	write_at ("MAC ADDRESS", strlen ("MAC ADDRESS"), 1, 2);
	sprintf (szBuff, "%s", szMacValue);
	write_at (szBuff, strlen (szBuff), 1, 3);

	write_at ("LINK SPEED", strlen ("LINK SPEED"), 1, 5);
	sprintf (szBuff, "%s", szLinkValue);
	write_at (szBuff, strlen (szBuff), 1, 6);

	WaitForKeyPress (8);
}

void ShowVersions (void)
{	
	char szBuff [64] = {'\0'};

	clrscr ();

	write_at ("VxCE VERSION", strlen ("VxCE VERSION"), 1, 2);
	sprintf (szBuff, "%s", g_szVXCEVersion);
	write_at (szBuff, strlen (szBuff), 1, 3);

	write_at ("CEIF VERSION", strlen ("CEIF VERSION"), 1, 5);
	sprintf (szBuff, "%s", g_szCEIFVersion);
	write_at (szBuff, strlen (szBuff), 1, 6);

	WaitForKeyPress (8);
}

void ManageCEEvents (ceEvent_t ceEventCB)
{
	stceNWEvt ceEvt;
	int retVal = 0;

	unsigned char ucEventData [32] = {'\0'};
	int evtDataLen;

	memset (&ceEvt, 0, sizeof (stceNWEvt));

	retVal = ceGetEventCount ();

	if (retVal > 0) {
		retVal = ceGetEvent (&ceEvt, 0, (void*) ucEventData, &evtDataLen);

		if (!retVal) {		// 0 - Success

			LOG_PRINTFF (LOG_APPFILTER, "[%s] Received CE Event ID [%d] from NWIF Handle [%d] Event Data Size [%d]", 
				__FUNCTION__, ceEvt.neEvt, ceEvt.niHandle, evtDataLen);

			LOG_HEX_PRINTF ("EVENT DATA", (char*) ucEventData, evtDataLen);

			switch (ceEvt.neEvt) {
				case CE_EVT_NET_UP:

					if (ceEventCB.OnCENetUp)
						ceEventCB.OnCENetUp (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_NET_DN:

					if (ceEventCB.OnCENetDown)
						ceEventCB.OnCENetDown (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_NET_FAILED:

					if (ceEventCB.OnCENetFailed)
						ceEventCB.OnCENetFailed (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_NET_OUT:

					if (ceEventCB.OnCENetOut)
						ceEventCB.OnCENetOut (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_NET_RES:

					if (ceEventCB.OnCENetRestored)
						ceEventCB.OnCENetRestored (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_SIGNAL:

					if (ceEventCB.OnCEEvtSignal)
						ceEventCB.OnCEEvtSignal (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_START_OPEN:

					if (ceEventCB.OnCEStartOpen)
						ceEventCB.OnCEStartOpen (ceEvt.niHandle, ucEventData, evtDataLen);
					
					break;

				case CE_EVT_START_LINK:

					if (ceEventCB.OnCEStartLink)
						ceEventCB.OnCEStartLink (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_START_NW:

					if (ceEventCB.OnCEStartNW)
						ceEventCB.OnCEStartNW (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_START_FAIL:

					if (ceEventCB.OnCEStartFail)
						ceEventCB.OnCEStartFail (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_STOP_NW:

					if (ceEventCB.OnCEStopNW)
						ceEventCB.OnCEStopNW (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_STOP_LINK:

					if (ceEventCB.OnCEStopLink)
						ceEventCB.OnCEStopLink (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_STOP_CLOSE:

					if (ceEventCB.OnCEStopClose)
						ceEventCB.OnCEStopClose (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_STOP_FAIL:

					if (ceEventCB.OnCEStopFail)
						ceEventCB.OnCEStopFail (ceEvt.niHandle, ucEventData, evtDataLen);

					break;

				case CE_EVT_DDI_APPL:

					if (ceEventCB.OnCEEvtDDIAPPL)
						ceEventCB.OnCEEvtDDIAPPL (ceEvt.niHandle, ucEventData, evtDataLen);

				default:
					break;
			}
		} else {
			LOG_PRINTFF (LOG_APPFILTER, "[%s] ceGetEvent FAILED [%d]", __FUNCTION__, retVal);
		}
	}
}

void GetMediaInfo (stNIInfo stArray[], int arrayCount)
{
	int i = 0;	

	for (i=0; i < arrayCount; i++) {
		if (strcmp (stArray [i].niCommTech, COMM_GPRS) == 0) {
			g_gprsHandle = stArray[i].niHandle;
			
		}

		if (strcmp (stArray [i].niCommTech, COMM_GSM) == 0) {
			g_gsmHandle = stArray[i].niHandle;
		}
	}
}

void ListNWIF (stNIInfo stArray[], int arrayCount)
{
	int cntr = 0;

	LOG_PRINTFF (LOG_APPFILTER, "-----------------------------------");
	LOG_PRINTFF (LOG_APPFILTER, "%s", __FUNCTION__);
	LOG_PRINTFF (LOG_APPFILTER, "-----------------------------------");

	for (cntr = 0; cntr < arrayCount; cntr++) {
		LOG_PRINTFF (LOG_APPFILTER, "  Device Name: %s", stArray[cntr].niDeviceName);
		LOG_PRINTFF (LOG_APPFILTER, "  Comm Tech: %s", stArray[cntr].niCommTech);
		LOG_PRINTFF (LOG_APPFILTER, "  Device Driver Name: %s", stArray[cntr].niDeviceDriverName); 
		LOG_PRINTFF (LOG_APPFILTER, "  Handle: %d", stArray[cntr].niHandle);
		LOG_PRINTFF (LOG_APPFILTER, "  Run State: %d", stArray[cntr].niRunState);
		LOG_PRINTFF (LOG_APPFILTER, "  Startup Mode: %d", stArray[cntr].niStartUpMode);
		LOG_PRINTFF (LOG_APPFILTER, "-----------------------------------");
	}
}

void FillGPRSSettings (int gprsHandle)
{
	stNI_PPPConfig pppCon;
	int retVal = 0;

	CHECK(FALSE);

	memset (&pppCon, 0, sizeof (pppCon));

	pppCon.ncAuthType = g_AppConfig.pppAuthType;
	strcpy(pppCon.ncUsername, g_AppConfig.szUser);   
	strcpy(pppCon.ncPassword, g_AppConfig.szPassword);

	LOG_PRINTFF (LOG_APPFILTER, "PPP Auth Used: %d", g_AppConfig.pppAuthType);
	LOG_PRINTFF (LOG_APPFILTER, "User: %s", g_AppConfig.szUser);
	LOG_PRINTFF (LOG_APPFILTER, "Password: %s", g_AppConfig.szPassword);
	LOG_PRINTFF (LOG_APPFILTER, "Phone: %s", g_AppConfig.szPhone);

	// Set user/pass
  	retVal = ceSetNWParamValue(gprsHandle, "PPP_CONFIG", (const void *)&pppCon, sizeof(pppCon));
	LOG_PRINTFF (LOG_APPFILTER, "[%s] ceSetNWParamValue retVal: %d", __FUNCTION__, retVal);
	/*
	// Set APN
	retVal = ceSetDDParamValue(gprsHandle, "gprs_apn", g_AppConfig.szAPN, strlen (g_AppConfig.szAPN) + 1);
	LOG_PRINTFF (LOG_APPFILTER, "[%s] ceSetDDParamValue - GPRS_APN retVal: %d", __FUNCTION__, retVal);

 	// Set Phone
	ceSetDDParamValue(gprsHandle, "gprs_primary", g_AppConfig.szPhone, strlen (g_AppConfig.szPhone) + 1);
	LOG_PRINTFF (LOG_APPFILTER, "[%s] ceSetDDParamValue - GPRS_PRIMARY retVal: %d", __FUNCTION__, retVal);
	*/
}

void DoTransaction (void)
{
	int sockHandle = -1;
	int retVal = -1;

	LOG_PRINTFF (LOG_APPFILTER, "-----------------------------------");
	LOG_PRINTFF (LOG_APPFILTER, " %s", __FUNCTION__);
	LOG_PRINTFF (LOG_APPFILTER, "-----------------------------------");

	if (g_AppConfig.iSSL) {
		LOG_PRINTFF (LOG_APPFILTER, "[%s] SSL ENABLED.  Setting up SSL", __FUNCTION__);

		InitOpenSSL ();
		
		g_CTX = setup_client_ctx ();

		if (!g_CTX) {
			LOG_PRINTFF (LOG_APPFILTER, "[%s] FAILED TO SET CTX", __FUNCTION__);

			clrscr ();
			sprintf (g_szMsgBuff, "\fCTX FAILED");
			write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 1);
			SVC_WAIT (SCREEN_DELAY);
			return;
		}

		SSL_CTX_set_info_callback (g_CTX, InfoCallBack);
	}

	// Connect socket
	SocketConnect (&sockHandle);

	if (g_AppConfig.iSSL) {
		// Do SSL handshake

		// 1 create SSL context
		if ((g_SSL = SSL_new(g_CTX)) == NULL)
			LOG_PRINTFF (LOG_APPFILTER, "[%s] Error creating an SSL context", __FUNCTION__);

		// 2 Assign the socket into SSL
		SSL_set_fd (g_SSL, sockHandle);

		// 3 Do the SSL handshake
		retVal = SSL_connect (g_SSL);

		LOG_PRINTFF (LOG_APPFILTER, "[%s] SSL_connect retVal: %d", __FUNCTION__, retVal);

		if ((g_err = PostConnectionCheck (g_SSL, g_AppConfig.szHostIP)) != X509_V_OK) {
			LOG_PRINTFF (LOG_APPFILTER, "[%s] Error: PEER CERTIFICATE %s", __FUNCTION__, X509_verify_cert_error_string (g_err));
			SocketClose (sockHandle);

			SSL_shutdown (g_SSL);
			SSL_free (g_SSL);
			SSL_CTX_free (g_CTX);

			clrscr ();
			sprintf (g_szMsgBuff, "\fPEER CERT ERROR");
			write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 1);
			return;
		}
	}

	DoSendReceive (sockHandle, g_AppConfig.iDataSize, 0);
	SocketClose (sockHandle);

	if (g_AppConfig.iSSL) {
		SSL_shutdown (g_SSL);
		SSL_free (g_SSL);
		SSL_CTX_free (g_CTX);
	}

	SVC_WAIT (SCREEN_DELAY);
}

int SocketConnect (int* pSocketHandle)
{
	int sockHandle;
	int sockType;
	int retVal;

	struct sockaddr_in	sockHost;
	struct timeval timeout;

	clrscr ();
	memset (&sockHost, 0, sizeof (struct sockaddr_in));
	memset (&timeout, 0, sizeof (struct timeval));

	sockHost.sin_family = AF_INET;
	sockHost.sin_addr.s_addr = htonl (inet_addr (g_AppConfig.szHostIP));
	sockHost.sin_port = htons (g_AppConfig.iPort);

	sockType = SOCK_STREAM;
	sockHandle = socket (AF_INET, sockType, 0);

	if (sockHandle < 0) {
		clrscr ();
		sprintf (g_szMsgBuff, "\fSocket FAILED");
		write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 1);
		LOG_PRINTFF (LOG_APPFILTER, "[%s] Socket creation FAILED: %d. errno: %d", __FUNCTION__, sockHandle, errno);
		return RET_FAILED;
	}

	*pSocketHandle = sockHandle;

	LOG_PRINTFF (LOG_APPFILTER, "[%s] Socket handle is: %d", __FUNCTION__, sockHandle);

	clrscr ();
	sprintf (g_szMsgBuff, "\fConnecting\n...");
	write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 1);
	SVC_WAIT (SCREEN_DELAY);

	retVal = connect (sockHandle, (struct sockaddr*)&sockHost,
		sizeof (struct sockaddr_in));

	if (retVal != 0)  {
		LOG_PRINTFF (LOG_APPFILTER, "[%s] Socket connect failed and retVal: %d errno: %d", 
			__FUNCTION__, retVal, errno);

		return RET_FAILED;
	}else{
		LOG_PRINTFF (LOG_APPFILTER, "[%s] Socket connect is successful returned: %d errno: %d", 
			__FUNCTION__, retVal, errno);
	}

	sprintf (g_szMsgBuff, "\fCONNECTED");
	write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 4);

	return RET_SUCCESS;
}

int SocketClose (int sockHandle)
{
	int retVal = 0;

	LOG_PRINTFF (LOG_APPFILTER, "Socket Shutdown....");

	retVal = socketclose (sockHandle);

	if (retVal < 0) {
		LOG_PRINTFF (LOG_APPFILTER, "Socket Shutdown FAILED: %d.  errno: %d", retVal, errno);
		return RET_FAILED;
	}

	LOG_PRINTFF (LOG_APPFILTER, "[%s] Socket Shutdown SUCCESS", __FUNCTION__);

	return RET_SUCCESS;
}

int DoSendReceive (int iSockHandle, unsigned int uiSendSize, unsigned int uiRecvSize)
{
	int cntr = 0, i;
	char cVal = 'A';
	int retVal, bytesRecvd = 0;

	char szSendBuff [MAX_DATASIZE] = {'\0'};
	char szRecvBuff [MAX_DATASIZE] = {'\0'};

	clrscr ();
	sprintf (g_szMsgBuff, "\fSending %d Bytes...", uiSendSize);
	write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 1);
	SVC_WAIT (SCREEN_DELAY);

	cntr = (uiSendSize >= MAX_DATASIZE) ? (uiSendSize / MAX_DATASIZE) : 1;

	for (i = 0; i < cntr; i++) {
		memset (szSendBuff, cVal, uiSendSize);

		if (!g_AppConfig.iSSL) {
			retVal = send (iSockHandle, szSendBuff, uiSendSize, 0);
		} else {
			retVal = SSL_write (g_SSL, szSendBuff, uiSendSize);
		}

		if (retVal < 0) {
			LOG_PRINTFF (LOG_APPFILTER, "SEND Batch %d Of %d FAILED: %d errno %d", i, cntr, retVal, errno);
			
			clrscr ();
			sprintf (g_szMsgBuff, "\fSending FAILED");
			write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 1);
			SVC_WAIT (SCREEN_DELAY);

			return RET_FAILED;
		}

		cVal++;
	}

	clrscr ();
	sprintf (g_szMsgBuff, "\fSending SUCCESS");
	write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 1);

	LOG_PRINTFF (LOG_APPFILTER, "SEND Finished.  Total Bytes Sent: %d", uiSendSize);

	sprintf (g_szMsgBuff, "\fReceiving...");
	write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 2);
	SVC_WAIT (SCREEN_DELAY);

	for (;;) {

		if (!g_AppConfig.iSSL) {
			retVal = recv (iSockHandle, szRecvBuff, sizeof (szRecvBuff), 0);
		} else {
			retVal = SSL_read (g_SSL, szRecvBuff, sizeof (szRecvBuff));
		}

		if (retVal < 0) {
			LOG_PRINTFF (LOG_APPFILTER, "Receive FAILED: errno %d", errno);

			clrscr ();
			sprintf (g_szMsgBuff, "\fReceive FAILED");
			write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 1);
			return RET_FAILED;
		} 

		bytesRecvd += retVal;

		clrscr ();
		sprintf (g_szMsgBuff, "\fReceive OK\nTotal Bytes Received:\n%d Bytes", bytesRecvd);
		write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 1);
		
		LOG_PRINTFF (LOG_APPFILTER, "Received %d.  Total Bytes Received: %d", retVal, bytesRecvd);

		// Log the received data in hex
		LOG_HEX_PRINTF ("RECV", szRecvBuff, retVal);	// only print the # of bytes received

		if (bytesRecvd >= uiSendSize)
			break;
	}

	clrscr ();
	sprintf (g_szMsgBuff, "\fReceive DONE\nTotal Bytes Received:\n%d Bytes", bytesRecvd);
	write_at (g_szMsgBuff, strlen (g_szMsgBuff), 1, 1);
	SVC_WAIT (SCREEN_DELAY);

	return RET_SUCCESS;
}

void LoadConfig (void)
{
	char szBuff [32] = {'\0'};
	memset (&g_AppConfig, 0, sizeof (g_AppConfig));

	memset (szBuff, '\0', sizeof (szBuff));
	if (get_env(CONFIG_DATASIZE, szBuff, 4) < 0) {
		g_AppConfig.iDataSize = DEFAULT_DATA_LEN;
	} else {
		g_AppConfig.iDataSize = atoi (szBuff);
	}

	memset (szBuff, '\0', sizeof (szBuff));
	if (get_env(CONFIG_HOSTIP, szBuff, MAX_IP_LENGTH) < 0) {
		strcpy (g_AppConfig.szHostIP, DEFAULT_IP);
	} else {
		strcpy (g_AppConfig.szHostIP, szBuff);
	}

	memset (szBuff, '\0', sizeof (szBuff));
	if (get_env(CONFIG_PORT, szBuff, 4) < 0) {
		g_AppConfig.iPort = DEFAULT_PORT;
	} else {
		g_AppConfig.iPort = atoi (szBuff);
	}

	memset (szBuff, '\0', sizeof (szBuff));
	if (get_env(CONFIG_SSL, szBuff, 4) < 0) {
		g_AppConfig.iSSL = 0;
	} else {
		g_AppConfig.iSSL = atoi (szBuff);
	}

	/*
	memset (szBuff, '\0', sizeof (szBuff));
	if (get_env(CONFIG_PPPTYPE, szBuff, 4) < 0) {
		g_AppConfig.pppAuthType = PAP;
	} else {
		g_AppConfig.pppAuthType = atoi (szBuff);
	}
	*/

	memset (szBuff, '\0', sizeof (szBuff));
	if (getEnv(CONFIG_USER, szBuff, MAX_USERNAME_LENGTH) < 0) {
		strcpy (g_AppConfig.szUser, DEFAULT_USERPASS);
	} else {
		strcpy (g_AppConfig.szUser, szBuff);
	}

	memset (szBuff, '\0', sizeof (szBuff));
	if (getEnv(CONFIG_PASSWORD, szBuff, MAX_PASSWORD_LENGTH) < 0) {
		strcpy (g_AppConfig.szPassword, DEFAULT_USERPASS);
	} else {                             
		strcpy (g_AppConfig.szPassword, szBuff);
	}
	
	memset (szBuff, '\0', sizeof (szBuff));
	if (getEnv(CONFIG_APN, szBuff, MAX_APN_LENGTH) < 0) {
		strcpy (g_AppConfig.szAPN, DEFAULT_APN);
	} else {
		strcpy (g_AppConfig.szAPN, szBuff);
	}

	memset (szBuff, '\0', sizeof (szBuff));
	if (get_env(CONFIG_CAFILE, szBuff, MAX_FILE_LENGTH) < 0) {
		strcpy (g_AppConfig.szCAFile, DEFAULT_CACERT);
	} else {
		strcpy (g_AppConfig.szCAFile, szBuff);
	}

	LOG_PRINTFF (LOG_APPFILTER, "CAFILE: %s", g_AppConfig.szCAFile);

	memset (szBuff, '\0', sizeof (szBuff));
	if (get_env(CONFIG_PHONE, szBuff, MAX_PHONE_LENGTH) < 0) {
		strcpy (g_AppConfig.szPhone, DEFAULT_PHONE);
	} else {
		strcpy (g_AppConfig.szPhone, szBuff);
	}

	CHECK(SUCCESS==(ret_code)SVC_INFO_SERLNO(g_AppConfig.szSerialNr));	
}

// SSL Calls

void InitOpenSSL (void)
{
	if (!SSL_library_init ()) {
		LOG_PRINTFF (LOG_APPFILTER, "Init OpenSSL Failed");
		return;
	}

	SSL_load_error_strings ();
}

int VerifyCallBack (int ok, X509_STORE_CTX* store)
{
	char data[256] = {'\0'};
 
    if (!ok)
    {
        X509 *cert = X509_STORE_CTX_get_current_cert(store);
        int  depth = X509_STORE_CTX_get_error_depth(store);
        int  err = X509_STORE_CTX_get_error(store);
 
        LOG_PRINTFF (LOG_APPFILTER, "-Error with certificate at depth: %i\n", depth);
        X509_NAME_oneline(X509_get_issuer_name(cert), data, sizeof (data));

        LOG_PRINTFF (LOG_APPFILTER, "  issuer   = %s\n", data);
        X509_NAME_oneline(X509_get_subject_name(cert), data, sizeof (data));
        LOG_PRINTFF (LOG_APPFILTER, "  subject  = %s\n", data);
        LOG_PRINTFF (LOG_APPFILTER, "  err %i:%s\n", err, X509_verify_cert_error_string(err));
    }
 
    return ok;
}

long PostConnectionCheck (SSL *ssl, char *host)
{
    X509      *cert;
    int       extcount;
    int       ok = 0;
	char*	  svrCertStr;

    /* Checking the return from SSL_get_peer_certificate here is not strictly
     * necessary.  With our example programs, it is not possible for it to return
     * NULL.  However, it is good form to check the return since it can return NULL
     * if the examples are modified to enable anonymous ciphers or for the server
     * to not require a client certificate.
     */

	// Checking PEER CERTIFICATE

	cert = SSL_get_peer_certificate (ssl);
	if (!cert) {
        if (cert)
			X509_free(cert);
		
		return X509_V_ERR_APPLICATION_VERIFICATION;
	}

	if (cert) {
		LOG_PRINTFF (LOG_APPFILTER, "SERVER CERTIFICATE");
		LOG_PRINTFF (LOG_APPFILTER, "=======================================");

		svrCertStr = X509_NAME_oneline (X509_get_subject_name (cert), 0, 0);
		LOG_PRINTFF (LOG_APPFILTER, "Subject: %s", svrCertStr);
		free (svrCertStr);

		svrCertStr = X509_NAME_oneline (X509_get_issuer_name (cert), 0, 0);
		LOG_PRINTFF (LOG_APPFILTER, "Issuer: %s", svrCertStr);
		free (svrCertStr);

		LOG_PRINTFF (LOG_APPFILTER, "=======================================");
	}

    if ((extcount = X509_get_ext_count(cert)) > 0) {
        int i;

        for (i = 0;  i < extcount;  i++) {
            char*              extstr;
            X509_EXTENSION*    ext;

            ext = X509_get_ext(cert, i);
            extstr = (char*) OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));

            if (!strcmp(extstr, "subjectAltName")) {
                int						j;
                unsigned char*			data;
                STACK_OF(CONF_VALUE)*	val;
                CONF_VALUE*				nval;
                X509V3_EXT_METHOD*		meth;
                void*					ext_str = NULL;

				meth = X509V3_EXT_get(ext);

                if (!meth)
                    break;

                data = ext->value->data;

                if (meth->it)
                    ext_str = ASN1_item_d2i(NULL, (const unsigned char**) &data, ext->value->length,
                            ASN1_ITEM_ptr(meth->it));
                else
                    ext_str = meth->d2i(NULL, (const unsigned char**) &data, ext->value->length);

                val = meth->i2v(meth, ext_str, NULL);
                for (j = 0;  j < sk_CONF_VALUE_num(val);  j++) {
                    nval = sk_CONF_VALUE_value(val, j);
                    if (!strcmp(nval->name, "DNS") && !strcmp(nval->value, host)) {
                        ok = 1;
                        break;
                    }
                }
            }

            if (ok)
                break;
        }
    }
    
    X509_free(cert);
    return SSL_get_verify_result(ssl);
}

void InfoCallBack (const SSL* ssl, int type, int val)
{
    LOG_PRINTFF (LOG_APPFILTER, "info: SSL %X %X %d\n", ssl, type, val);
}

SSL_CTX* setup_client_ctx (void)
{
	SSL_CTX* ctx;

#if 0
    file_attr = dir_get_attributes (CAFILE);
    if (file_attr & ATTR_NOT_AUTH) {
        LOG_PRINTFF (LOG_APPFILTER, "setup_client_ctx: CAFILE not authentic");
        return NULL;
    }
#endif

	// NOTE: SSLv23_method problem in linking for now
    //ctx = SSL_CTX_new (TLSv1_client_method());
	ctx = SSL_CTX_new (SSLv23_method ());
    LOG_PRINTFF (LOG_APPFILTER, "SSL_CTX_new %X", ctx);

    /* Auto retry read/write when renegotitation is requested.
     * This only makes sense when the underlying socket is blocking.
     */

#if 0
    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
#endif

	// Load our CA
	if (SSL_CTX_load_verify_locations(ctx, g_AppConfig.szCAFile, CADIR) != 1)
        LOG_PRINTFF (LOG_APPFILTER, "Error loading CA file and/or directory");

    if (SSL_CTX_set_default_verify_paths(ctx) != 1)
        LOG_PRINTFF (LOG_APPFILTER, "Error loading CA default file and/or directory");

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, VerifyCallBack);
    SSL_CTX_set_verify_depth(ctx, 3);

	LOG_PRINTFF (LOG_APPFILTER, "setup_client_ctx SUCCESSFULL");

    return ctx;
}

int memicmp(const void *s1, const void *s2, size_t len)
{
    const char *p1 = (const char *)s1;
    const char *p2 = (const char *)s2;

    if ((s1 == NULL) || (s2 == NULL) || (len == 0)) return -1;
    while( (toupper(*p1) == toupper(*p2)) && --len ) {
        ++p1;
        ++p2;
    }
    return toupper(*(unsigned char*)p1) - toupper(*(unsigned char*)p2);
}

void MainMenu (void)
{
	long lOSEvent = 0;
	int	isFinished = FALSE;
	char szKey;
	ceEvent_t	ceEvent_cb;
	long lConStatTimer = 0;

	clrscr ();
	          
	write_at ("CE SAMPLE - GPRS     ", strlen ("CE SAMPLE - GPRS     "), 1, 1);
	write_at ("1 --> TRANSACT       ", strlen ("1 --> TRANSACT       "), 1, 4);
	write_at ("2 --> ACTIVATE NCP   ", strlen ("2 --> ACTIVATE NCP   "), 1, 6);
	write_at ("3 --> IP INFORMATION ", strlen ("3 --> IP INFORMATION "), 1, 8);
	write_at ("4 --> CE VERSIONS    ", strlen ("4 --> CE VERSIONS    "), 1, 10);


	// Set Event Handler Functions
	ceEvent_cb.OnCENetUp		= MainMenu_OnNetUp;
	ceEvent_cb.OnCENetDown		= MainMenu_OnNetDown;
	ceEvent_cb.OnCEStartFail	= MainMenu_OnStartFail;
	ceEvent_cb.OnCEStartOpen	= MainMenu_OnOpen;
	ceEvent_cb.OnCENetOut		= MainMenu_OnNetOut;
	ceEvent_cb.OnCENetRestored	= MainMenu_OnNetRestored;
	ceEvent_cb.OnCEEvtSignal	= MainMenu_OnSignal;
	ceEvent_cb.OnCENetFailed	= MainMenu_OnNetFailed;

	while (!isFinished) {
		lOSEvent = wait_event ();

		if (lOSEvent & EVT_PIPE) {
			ManageCEEvents (ceEvent_cb);
		}

		if (lOSEvent & EVT_KBD) {
			if (read (g_conHandle, &szKey, 1) > 0) {
				isFinished = TRUE;
				szKey &= 0x7f;

				switch (szKey) {
					case KEY_1:
						if (g_LinkState == LINK_DISCONNECTED || g_LinkState == LINK_INIT) {
							clrscr ();
							write_at ("NOT YET CONNECTED    ", strlen ("NOT YET CONNECTED    "), 1, 4);
							write_at ("TO GPRS NETWORK      ", strlen ("TO GPRS NETWORK      "), 1, 5);
						} else {
							DoTransaction ();
						}

						WaitForKeyPress (7);
						break;
					case KEY_2:
						ceActivateNCP ();
						break;
					case KEY_3:
						GetIPAddress ();
						break;
					case KEY_4:
						ShowVersions ();
						break;
				}
			}
		}

		if (lOSEvent & EVT_ACTIVATE) {
			isFinished = TRUE;
			LOG_PRINTFF (LOG_APPFILTER, "Returning from NCP");
			g_conHandle = open (DEV_CONSOLE, 0);
		}

		if (lOSEvent & EVT_DEACTIVATE) {
			isFinished = TRUE;
			clr_timer (lConStatTimer);
		}
	}

	MainMenu ();
}

////////////////////////////////////////////////////////////////////////////
// Callback functions used for every
// CE Events Received
////////////////////////////////////////////////////////////////////////////

void MainMenu_OnNetUp (unsigned int nwifHandle, unsigned char* szData, int iDataLen)
{
	LOG_PRINTFF (LOG_APPFILTER, "---> %s", __FUNCTION__);
	write_at ("CONNECTED            ", strlen ("CONNECTED            "), 1, 2);

	g_LinkState = LINK_CONNECTED;
}

void MainMenu_OnNetDown (unsigned int nwifHandle, unsigned char* szData, int iDataLen)
{
	LOG_PRINTFF (LOG_APPFILTER, "---> %s", __FUNCTION__);
	write_at ("NOT CONNECTED        ", strlen ("NOT CONNECTED        "), 1, 2);

	g_LinkState = LINK_DISCONNECTED;
}

void MainMenu_OnStartFail (unsigned int nwifHandle, unsigned char* szData, int iDataLen)
{
	LOG_PRINTFF (LOG_APPFILTER, "---> %s", __FUNCTION__);
	write_at ("NWIF START FAILED    ", strlen ("NWIF START FAILED    "), 1, 2);

	g_LinkState = LINK_DISCONNECTED;
}

void MainMenu_OnOpen (unsigned int nwifHandle, unsigned char* szData, int iDataLen)
{
	LOG_PRINTFF (LOG_APPFILTER, "---> %s", __FUNCTION__);
	write_at ("DEVICE OPEN          ", strlen ("DEVICE OPEN          "), 1, 2);

	g_LinkState = LINK_INIT;
}

void MainMenu_OnNetOut (unsigned int nwifHandle, unsigned char* szData, int iDataLen)
{
	LOG_PRINTFF (LOG_APPFILTER, "---> %s", __FUNCTION__);
	write_at ("NET OUT              ", strlen ("NET OUT              "), 1, 2);

	g_LinkState = LINK_DISCONNECTED;
}

void MainMenu_OnNetFailed (unsigned int nwifHandle, unsigned char* szData, int iDataLen)
{
	LOG_PRINTFF (LOG_APPFILTER, "---> %s", __FUNCTION__);
	write_at ("NET FAILED           ", strlen ("NET FAILED           "), 1, 2);

	g_LinkState = LINK_DISCONNECTED;
}

void MainMenu_OnNetRestored (unsigned int nwifHandle, unsigned char* szData, int iDataLen)
{
	LOG_PRINTFF (LOG_APPFILTER, "---> %s", __FUNCTION__);
	write_at ("NET RESTORED         ", strlen ("NET RESTORED         "), 1, 2);

	g_LinkState = LINK_CONNECTED;
}

void MainMenu_OnSignal (unsigned int nwifHandle, unsigned char* szData, int iDataLen)
{
	LOG_PRINTFF (LOG_APPFILTER, "---> %s", __FUNCTION__);

}
