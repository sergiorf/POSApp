COMMENGINE SAMPLE APPLICATION - GPRS
====================================================================

This sample application demonstrates a basic client application that uses the 
CommEngine (CE) and TCPIP library for communication

SOFTWARE PREREQUISITES
====================================================================

Below are the library and other software/hardware components used in building the application

	- RVDS 4.0 Compiler
	- Vx680 with VxEOS Evaluation OS QT00000U
	- VRXSDK 3.1.0A
	- VXEOS CD0 Update 1008

BUILDING THE APPLICATION
====================================================================


1.  Prepare the EOS SDK Environment.  Install the VRXSDK 3.1.0A

2.  Extract the VxEOS CD0 update 1008 package (VxEOS-01000008.zip).  Extract the 
EOSSDK.zip file which contains the necessary header files for VxEOS.
Inside the folder where you extracted the EOSSDK zip file, copy the \include and \lib
folders to your VRXSDK 3.1.0A folder.


3.  Be sure to set the correct library path was properly set on the makefile.  Below are sample library paths:

	VRXSDK		= D:\sdk\VxEOS\VRXSDK\3.1.0A


4.  The following header file must be included in your application in order to use the newest TCPIP and VxEOS LOGSYS libraries:

	<svc_net.h>	- For TCPIP API included in the VRXSDK 3.1.0A
	<eoslog.h>	- For the VxEOS LOGSYS
	<ceif.h>	- For CE API (CommEngine API)

	For SSL enabled application, make sure to add the following header files
	
	<openssl/bio.h>
	<openssl/err.h>
	<openssl/rand.h>
	<openssl/ssl.h>
	<openssl/x509v3.h>

5.  Both the EOS LOGSYS and TCPIP libraries are released as shared libraries.  Be sure to link the following object files:

	elog.o  		- Located under VRXSDK 3.1.0A\lib folder
	svc_net.o		- Located under VRXSDK 3.1.0A\lib folder
	ceif.o			- Located under VRXSDK 3.1.0A\lib folder
	ssl.o			- Located under VRXSDK 3.1.0A\lib folder

6.  Run build.bat to compile the application.  This is located on the CESmpl folder under \Projects\rv\Debug folder.


DOWNLOADING AND RUNNING THE APPLICATION
====================================================================


1.  Before downloading any application, make sure that the *DEBUG on GID1 was set to 0 or was cleared otherwise, download won't start.  
After setting it to 0, restart the terminal to take effect.

2.  Download first the Vx680 QT0000U VxEOS Eval OS

3.  For Trident terminals such as Vx680, run the d11.bat file inside the folder where 
you extract the VxEOS package.  This will download all the required EOS libraries
on the terminal

4.  Perform a partial download for the CESmpl application.  On your CESmpl directory under \Output\Rv\Download, run dl.bat to download this application on GID 1.

5.  Use the cert.bat file under the \Output\RV\Download folder to download the bundled CACert.pem certificate file

6.  Set the #SSL to 1 to enable SSL.  Note that this sample application only supports server certificate for now.


CREATING SSL CERTIFICATES
====================================================================

1.  Make sure that OpenSSL is installed
2.  In order to create a ROOT (CA) Certificate, inside the OpenSSL directory, type the following commands:

	openssl req -new -x509 -extensions v3_ca -keyout CACert.key -out CACert.pem -days 365 -config bin\openssl.conf

3.  In order to create a server or client certificate, first you must create a certificate signing request.  Below is the following command:
	
	openssl req -new -nodes -out SvrCert.csr -keyout SvrCert.key -config bin\openssl.conf

    Once a CSR is created, sign the certificate using the following command:

	openssl ca -out SvrCert.pem -config bin\openssl.conf -infiles SvrCert.csr


INSTALLING SSL CERTIFICATES
====================================================================

1.  Download the CA certificate (CACert.pem) on GID 1 of the terminal
2.  Server certificate and key (SvrCert.pem and SvrCert.key) must be installed on the STunnel Root Folder.  
3.  Be sure that on the stunnel.conf file, the following are defined

	cert = C:\openssl\SvrCert.pem
	key = C:\openssl\SvrCert.key