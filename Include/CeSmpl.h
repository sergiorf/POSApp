#ifndef __CECALLS_H_
#define __CECALLS_H_

#include <svc.h>

#include <ceif.h>

#define	APPNAME						"CESMPL"
#define LOG_APPFILTER				0x00000001L

#define CADIR						NULL

#define	RET_SUCCESS					0
#define	RET_FAILED					-1
#define	SCREEN_DELAY				500

#define	MEDIA_ETHERNET				100
#define	MEDIA_LANDLINE				101

#define F0_KEY						0xEE
#define F1_KEY						0xFA
#define F2_KEY						0xFB
#define F3_KEY						0xFC
#define F4_KEY						0xFD
#define UP_KEY						0xE1
#define DOWN_KEY					0xE2
#define BACK_KEY					0xE3
#define NEXT_KEY					0xE4
#define CANCEL_KEY					0x9B
#define ENTER_KEY					0x8D

// ANDing VFI scancodes with 0x7F results to
// ASCII standards
#define	KEY_0						0x30
#define	KEY_1						0x31
#define	KEY_2						0x32
#define	KEY_3						0x33
#define	KEY_4						0x34
#define	KEY_5						0x35
#define	KEY_6						0x36
#define	KEY_7						0x37
#define	KEY_8						0x38
#define	KEY_9						0x39
#define KEY_ESC						0x1B

#define	CONN_STAT_TIMER				10000

#define	COMM_GPRS					"GPRS"
#define COMM_ETH					"Ethernet"
#define	COMM_GSM					"GSM"
#define	COMM_DIAL					"Dial"

#define LINK_CONNECTED				100
#define	LINK_DISCONNECTED			101
#define	LINK_INIT					102

// Callback Function
typedef void (*CEEvent) (unsigned int nwifHandle, unsigned char* szData, int iDataLen);

typedef struct tagCEEvent {
	CEEvent OnCENetUp;
	CEEvent OnCENetDown;
	CEEvent	OnCENetFailed;
	CEEvent	OnCENetOut;
	CEEvent	OnCENetRestored;
	CEEvent	OnCEEvtSignal;
	CEEvent	OnCEStartOpen;
	CEEvent OnCEStartLink;
	CEEvent OnCEStartNW;
	CEEvent OnCEStartFail;
	CEEvent OnCEStopNW;
	CEEvent	OnCEStopLink;
	CEEvent OnCEStopClose;
	CEEvent	OnCEStopFail;
	CEEvent	OnCEEvtDDIAPPL;
} ceEvent_t;


// Function Prototypes
int InitComEngine (void);
int InitCEEvents (void);
int InitNWIF (void);
int NetworkConfig (int iMedia);

void MainMenu (void);

void GetIPAddress (void);
void GetMACAddress (void);
void ManageCEEvents (ceEvent_t ceEventCB);
void ShowVersions (void);

void GetMediaInfo (stNIInfo stArray[], int arrayCount);
void ListNWIF (stNIInfo stArray[], int arrayCount);
int GetConnStatus (int iNWIFHandle);
void FillGPRSSettings (int gprsHandle);

void GetNWIFDeviceName (stNIInfo stArray[], int arrayCount, const char* szMedia, char* szDevName);

int SocketConnect (int* pSocketHandle);
int SocketClose (int sockHandle);
int DoSendReceive (int iSockHandle, unsigned int uiSendSize, unsigned int uiRecvSize);

void DoTransaction (void);
void LoadConfig (void);

void WaitForKeyPress (int row);


// SSL Calls
void InitOpenSSL (void);
int VerifyCallBack (int ok, X509_STORE_CTX* store);
long PostConnectionCheck (SSL* ssl, char* host);
void InfoCallBack (const SSL* ssl, int type, int val);

SSL_CTX* setup_client_ctx (void);

int memicmp(const void *s1, const void *s2, size_t len);

// Callback function prototypes

void MainMenu_OnNetUp (unsigned int nwifHandle, unsigned char* szData, int iDataLen);
void MainMenu_OnNetDown (unsigned int nwifHandle, unsigned char* szData, int iDataLen);
void MainMenu_OnStartFail (unsigned int nwifHandle, unsigned char* szData, int iDataLen);
void MainMenu_OnOpen (unsigned int nwifHandle, unsigned char* szData, int iDataLen);

void MainMenu_OnNetOut (unsigned int nwifHandle, unsigned char* szData, int iDataLen);
void MainMenu_OnNetRestored (unsigned int nwifHandle, unsigned char* szData, int iDataLen);

void MainMenu_OnSignal (unsigned int nwifHandle, unsigned char* szData, int iDataLen);
void MainMenu_OnNetFailed (unsigned int nwifHandle, unsigned char* szData, int iDataLen);

#endif	// __CECALLS_H_
