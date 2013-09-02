#ifndef _GPRS_C_
#define _GPRS_C_

#include "../Include/gprs.h"
#include "../Include/Util.h"

#include <string.h>
#include <svc.h>
#include <ceif.h>
#include <ceifConst.h>
#include <errno.h>
#include <svc_net.h>	// TCP/IP library
#include <eoslog.h>

#define LogSerial dbprintf 

#define	_TOKEN_START_	"/*"
#define	_TOKEN_END_		"*/"

#define	MAX(a,b)	(((a)>(b)) ? (a):(b))
#define	MIN(a,b)	(((a)<(b)) ? (a):(b))

#define HIBYTE(x)	(((x) >> 8)	& 0x00ff)
#define LOBYTE(x)	( (x)		& 0x00ff)

/* --- Constants ------	*/
/* --------------------	*/
#define DESLIGADO "GPRS DESLIGADO"
#define REGISTRANDO "GPRS REGISTRANDO"
#define REGISTRADO "GPRS REGISTRADO"
#define ATTACHANDO "GPRS ATTACHANDO"
#define ATTACHADO "GPRS ATTACHADO"
#define CONECTADO "GPRS CONECTADO"

#define	MAX_START_LENGTH	128
#define	 MAX_INIT_LENGTH	128
#define	  MAX_APN_LENGTH	64
#define	 IP_ADDRESS_SIZE	17		// nnn.nnn.nnn.nnn\0
//	- PPP -------------------------
char *PPP_PARMS[] = { "PPP-USR", "PPP-PWD" };
enum {
	PPP_USR = 0, PPP_PWD
};
//	- TCP/IP ----------------------
char *TCP_PARMS[] = { "TCP-IP", "TCP-MASK", "TCP-GWAY", "TCP-DNS1", "TCP-DNS2" };
enum {
	TCP_IP = 0, TCP_MASK, TCP_GWAY, TCP_DNS1, TCP_DNS2
};
//	- GPRS ------------------------
char *GPRS_PARMS[] = { "GPRS-INIT", "GPRS-START", "GPRS-PHONE", "GPRS-APN" };
enum {
	GPRS_INIT_ = 0, GPRS_START, GPRS_PHONE, GPRS_APN
};

//	------------------------------------------------------------
//	Local variables
//	------------------------------------------------------------
static int iHnd = -1;
int currSts;

static int iDevCount = 0; // Device count
static stNIInfo* pDevInfo = NULL; // Device list
static int iDevIndex = -1; // Device index

volatile int SimcardSts = GPRS_STS_SIMERROR;

static ret_code ceSetDDParameters(void);

/****************************************************************************/
/*	Function:		ceGetNWIFHandle 										*/
/*	Description:	Retrieve device handle									*/
/*	Data:			03/Jun/2011												*/
/*	Author:			Marcelo_R1												*/
/*	----------------------------------------------------------------------- */
/*	Revisions:																*/
/*	Date		Name		What											*/
/*	03/Jun/2011	Marcelo_R1	Creation										*/
/****************************************************************************/
static int ceGetNWIFHandle(char* commTech) {
	int ret = ECE_SUCCESS;
	unsigned int iNumDev = 0;

	// Get the total number of network interface from this terminal (NWIF)

	iDevCount = ceGetNWIFCount();
	#ifdef DBG_CONN
	LogSerial("DevCount: %d", iDevCount);
	#endif
	if (iDevCount > 0) {
		if (pDevInfo)
			free(pDevInfo);
		pDevInfo = (stNIInfo*) malloc((iDevCount + 1) * sizeof(stNIInfo));

		ret = ceGetNWIFInfo(pDevInfo, iDevCount, &iNumDev);
		#ifdef DBG_CONN
		LogSerial("ceGetNWIFInfo()= %d | %d", ret, iNumDev);
		#endif
		if (!ret) {
			int i;
			for (ret = ECE_NOT_SUPPORTED, i = 0; i < iNumDev; i++) {
				#ifdef DBG_CONN
				LogSerial("Device Name       : %s", pDevInfo[i].niDeviceName);
				LogSerial("Comm Tech         : %s", pDevInfo[i].niCommTech);
				LogSerial("Device Driver Name: %s",
						pDevInfo[i].niDeviceDriverName);
				LogSerial("Handle            : %d", pDevInfo[i].niHandle);
				LogSerial("Run State         : %d", pDevInfo[i].niRunState);
				LogSerial("Startup Mode      : %d", pDevInfo[i].niStartUpMode);
				LogSerial("Error Code        : %d", pDevInfo[i].niErrorCode);
				LogSerial("-------------------");
				#endif

				if (!strcmp(pDevInfo[i].niCommTech, commTech)) {
					ret = pDevInfo[iDevIndex = i].niHandle;
					#ifdef DBG_CONN
					LogSerial("FOUND!!");
					#endif
					break;
				}
			}
		}
	} else
		ret = iDevCount;
	iDevIndex = iDevIndex;

	LogSerial("ceGetNWIFHandle(): %d", ret);
	return ret;
}

/****************************************************************************/
/*	Function:		ceSetNWParameters										*/
/*	Description:	Set network parameters									*/
/*	Data:			03/Jun/2011												*/
/*	Author:			Marcelo_R1												*/
/*	----------------------------------------------------------------------- */
/*	Revisions:																*/
/*	Date		Name		What											*/
/*	03/Jun/2011	Marcelo_R1	Creation										*/
/****************************************************************************/
static int ceSetNWParameters(void) {
	int ret = 1;
	char szIP[5][IP_ADDRESS_SIZE];
	in_addr_t ulIP;
	stNI_IPConfig stIP;

	stIP.ncDHCP = 1;
	stIP.ncIPAddr = 0x0000000000;
	stIP.ncGateway = 0x00000000;
	stIP.ncSubnet = 0x00000000;
	stIP.ncDNS1 = 0x00000000; // 8.8.8.8 dns do google
	stIP.ncDNS2 = 0x00000000; // 8.8.4.4 dns do google
	#ifdef DBG_CONN
	LogSerial("DHCP   : %d\n\r", stIP.ncDHCP);
	LogSerial("IP     : %x\n\r", stIP.ncIPAddr);
	LogSerial("Mask   : %x\n\r", stIP.ncGateway);
	LogSerial("Gateway: %x\n\r", stIP.ncSubnet);
	LogSerial("DNS Pri: %x\n\r", stIP.ncDNS1);
	LogSerial("DNS Sec: %x\n\r", stIP.ncDNS2);
	#endif

	if (ret)
		ret = ceSetNWParamValue(iHnd, IP_CONFIG, (const void *) &stIP,
				sizeof(stIP));
	else
		ret = ECE_PARAM_INVALID;
	#ifdef DBG_CONN
	LogSerial("ceSetNWParameters(): %d", ret);
	#endif
	return ret;
}

int GPRS_connect()
{
	int errp;
	GPRS_init(&errp);
	if (errp >= 0 || errp == -1037){
		while (currSts != 5){
			GPRS_yield(&errp);
			switch(currSts){
				case 0:
					WRITE(DESLIGADO, 8);
				break;
				case 1:
					WRITE(REGISTRANDO, 8);
				break;
				case 2:
					WRITE(REGISTRADO, 8);
				break;
				case 3:
					WRITE(ATTACHANDO, 8);
				break;
				case 4:
					WRITE(ATTACHADO, 8);
				break;
				case 5:
					WRITE(CONECTADO, 8);
					errp = 0;
				break;
			}
		}
	}
	return errp;
}

/****************************************************************************/
/*	Function:		GPRS_init 												*/
/*	Description:	Initialize communication device							*/
/*	Data:			03/Jun/2011												*/
/*	Author:			Marcelo_R1												*/
/*	----------------------------------------------------------------------- */
/*	Revisions:																*/
/*	Date		Name		What											*/
/*	03/Jun/2011	Marcelo_R1	Creation										*/
/****************************************************************************/
int GPRS_init(int *errp) {
	int ret = 0;
	char tech[5];
	memset(tech, 0, 5);
	#ifdef DBG_CONN
	LogSerial("GPRS_init()");
	#endif

	get_env("METHOD", tech, 5);

	ret = ceRegister(); // Register this application with CommEngine
	if (ret < 0) {
		switch (ret) {
		case ECE_REGAGAIN:
			ceUnregister(); // Unregister first then ...
			ret = ceRegister(); // ... register this application with CommEngine
		default:
			break;
		}
	}
	if (ret >= 0) {
		ceEnableEventNotification(); // Enable receiving CommEngine events
		iHnd = ceGetNWIFHandle(CE_COMM_TECH_GPRS);
		#ifdef DBG_CONN
		LogSerial("Colocando GPRS no modo manual");
		#endif
		ceSetNWIFStartMode(iHnd, CE_SM_MANUAL);
		#ifdef DBG_CONN
		LogSerial("Modo escolhido no config: %s", tech);
		#endif
		if (!strcmp("G", tech)) {
			#ifdef DBG_CONN
			LogSerial("Iniciando o equipamento no modo GPRS");
			#endif
			iHnd = ceGetNWIFHandle(CE_COMM_TECH_GPRS);
		} else if (!strcmp("E", tech)) {
			#ifdef DBG_CONN
			LogSerial("Iniciando o equipamento no modo ETH");
			#endif
			iHnd = ceGetNWIFHandle(CE_COMM_TECH_ETH);
		}
		if (iHnd >= 0) {
			ceSetNWIFStartMode(iHnd, CE_SM_MANUAL); // Network interface must be explicitly started
			//ceSetSignalNotification(CE_SF_OFF); // Disable signal strength notification --------------------------------------
			CHECK(SUCCESS==ceSetDDParameters());
			#ifdef DBG_CONN
			LogSerial("REGISTERING...");
			#endif
			ret = ceStartNWIF(iHnd, CE_OPEN); // Incremental start
		} else
			ret = iHnd;
	}
	*errp = ret;
	currSts = GPRS_STS_OFF;

	#ifdef DBG_CONN
	LogSerial("GPRS_init(): currSts: %d, errp: %d, errno: %d", currSts, ret,
			errno);
	#endif

	return currSts;
}

#endif	/* _GPRS_C_	*/

/****************************************************************************/
/*	Function:		process_CEEvent 										*/
/*	Description:	Event handler											*/
/*	Data:			03/Jun/2011												*/
/*	Author:			Marcelo_R1												*/
/*	----------------------------------------------------------------------- */
/*	Revisions:																*/
/*	Date		Name		What											*/
/*	03/Jun/2011	Marcelo_R1	Creation										*/
/****************************************************************************/

static int process_CEEvent(int *errp) {
	int isConnected = (int) FALSE;
	stceNWEvt ceEvt;
	*errp = 0;

	while (ceGetEventCount() > 0) {
		ceGetEvent(&ceEvt, 0, NULL, NULL);
		#ifdef DBG_CONN
		LogSerial("CEEvent: %x", ceEvt.neEvt);
		#endif
		switch (ceEvt.neEvt) {
		case CE_EVT_START_OPEN:
			currSts = GPRS_STS_REGISTERED;
			SimcardSts = GPRS_STS_SUCCESS;
			#ifdef DBG_CONN
			LogSerial("ATTACHING : %d", ceEvt.neEvt);
			#endif
			//ceSetPPPParameters( );
			*errp = ceStartNWIF(iHnd, CE_LINK);
			break;

		case CE_EVT_NET_DN:
		case CE_EVT_NET_OUT:
		case CE_EVT_NET_FAILED:
		case CE_EVT_START_LINK:
			currSts = GPRS_STS_ATTACHED;
			#ifdef DBG_CONN
			LogSerial("CONNECTING: %d", ceEvt.neEvt);
			#endif

			ceSetNWParameters();
			*errp = ceStartNWIF(iHnd, CE_NETWORK);
			break;

		case CE_EVT_START_NW:
			break;
		case CE_EVT_NET_UP:
		case CE_EVT_NET_RES:
			currSts = GPRS_STS_CONNECTED;
			#ifdef DBG_CONN
			LogSerial("CONNECTED : %d", ceEvt.neEvt);
			#endif
			break;

		case CE_EVT_STOP_NW:
			currSts = GPRS_STS_ATTACHED;
			break;
		case CE_EVT_STOP_LINK:
			currSts = GPRS_STS_REGISTERED;
			break;

		case CE_EVT_START_FAIL:
			SimcardSts = GPRS_STS_NOSIM;
		case CE_EVT_STOP_CLOSE:
		case CE_EVT_STOP_FAIL:
			currSts = GPRS_STS_OFF;
			break;
		}
		//LogSerial("process_CEEven: %d | connected: %d", *errp, isConnected);
	}
	/*	{	stNI_NWIFState	stState;
	 int	ret;
	 unsigned  int	uLen   ;

	 ret = ceGetNWParamValue(iHnd, NWIF_STATE, (void *)&stState, sizeof(stState), &uLen);

	 LogSerial("ceGetNWParamValue: %d | State: %d/%d", ret, stState.nsCurrentState, stState.nsTargetState);
	 if(!ret )
	 {	isConnected = (	(stState.nsCurrentState == NWIF_CONN_STATE_LINK) ||
	 (stState.nsCurrentState == NWIF_CONN_STATE_NET ) );
	 }
	 else
	 */
	{
		isConnected = (currSts == GPRS_STS_CONNECTED);
	}
	/*	}
	 */
#ifdef DBG_CONN
	LogSerial("process_CEEven: %d | connected: %d | Status: %d", *errp, isConnected, currSts);
#endif
	return isConnected;
}

/****************************************************************************/
/*	Function:		GPRS_yield 												*/
/*	Description:	Non blocking NW operation control						*/
/*	Data:			03/Jun/2011												*/
/*	Author:			Marcelo_R1												*/
/*	----------------------------------------------------------------------- */
/*	Revisions:																*/
/*	Date		Name		What											*/
/*	03/Jun/2011	Marcelo_R1	Creation										*/
/****************************************************************************/
int GPRS_yield(int *errp) { // Non blocking NW operation is controlled by the EOS
	return process_CEEvent(errp);
}

/*
 * Set GPRS device driver parameters
 */
static ret_code ceSetDDParameters(void) 
{
	ret_code ret = (0==ceSetDDParamValue(iHnd, "GP_APN", g_AppConfig.szAPN,
			1+strlen(g_AppConfig.szAPN)))? SUCCESS:ERROR;	
	return ret;	
}

