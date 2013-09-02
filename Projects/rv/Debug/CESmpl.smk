#------------------------------------------------------------ 
#
#       open.SMK - NMAKE file to build  ucl test program
#					 using the SDS tools
#
#-------------------------------------------------------------
RVCTBIN		= C:\Progra~1\ARM\RVCT\Programs\4.0\436\multi1\win_32-pentium
RVCTDIR		= C:\Progra~1\ARM\RVCT\Programs\4.0\436\multi1\win_32-pentium

#VRXSDK		= C:\verixaps\VFSDK\T3.0.0_EOS10008
VRXSDK		= C:\VxAps\VRXSDK
EOSSDK		= C:\VxAps\EOSSDK
VACT2000    = C:\VxAps\ACT2000

#  App Dir Paths
AppDir = ..\..\..

#  App Source Paths
SrcDir = $(AppDir)\Source

#  Lib Include Paths
SDSIncludes		= $(VRXSDK)\include
EOSIncludes		= $(EOSSDK)\include
ACT2000Includes	= $(VACT2000)\include

#  Compiler/Linker/Outhdr Output Paths
ObjDir = $(AppDir)\Output\RV\Obj
OutDir = $(AppDir)\Output\RV\Files\Debug

# Library Paths
SDSLibraries	= $(VRXSDK)\lib
EOSLibraries	= $(EOSSDK)\lib
ACTLibraries	= $(VACT2000)\Output\RV\Files\Static\Release

Includes = -I$(SDSIncludes) -I$(EOSIncludes) -I$(ACT2000Includes)  \
	-I$(AppDir)\libbmp-0.1.3\src

COptions = -b -DLOGSYS_FLAG -DLOGSYS_NEW_API_STYLE -DDBG_CONN

#
# Dependencies
#
AppObjects =			\
	$(ObjDir)\printer.o \
	$(ObjDir)\modelo.o \
	$(ObjDir)\codebar.o \
	$(ObjDir)\bmpfile.o \
	$(ObjDir)\bmputils.o \
	$(ObjDir)\venda.o	\
	$(ObjDir)\ambiente.o	\
	$(ObjDir)\ingresso.o	\
	$(ObjDir)\menus.o	\
	$(ObjDir)\trex.o	\
	$(ObjDir)\operadores.o	\
	$(ObjDir)\dict.o	\
	$(ObjDir)\db.o	\
	$(ObjDir)\venda_screen.o \
	$(ObjDir)\startup_screen.o \
	$(ObjDir)\main_screen.o \
	$(ObjDir)\login_screen.o \
	$(ObjDir)\update_screen.o	\
	$(ObjDir)\eventos.o	\
	$(ObjDir)\relatorio.o	\
	$(ObjDir)\relatorio_screen.o	\
	$(ObjDir)\http_lib.o	\
	$(ObjDir)\tcp.o	\
	$(ObjDir)\gprs.o	\
	$(ObjDir)\Util.o	\
	$(ObjDir)\date.o	\
	$(ObjDir)\write_utils.o	\
	$(ObjDir)\valor_utils.o	\
	$(ObjDir)\str_utils.o	\
	$(ObjDir)\CeSmpl.o
	
Libs = \
	$(ACTLibraries)\act2000.a \
	$(SDSLibraries)\svc_net.o \
	$(SDSLibraries)\ssl.o \
	$(SDSLibraries)\ceif.o \
	$(SDSLibraries)\elog.o
	
#
#  Target Definition
#
#"$(VSFSTOOL)\filesignature" .\CeSmpl.fst -nogui

# Should be using drive N: but for now, use flash (F)
pseudoOut : $(OutDir)\CeSmpl.out
	$(VRXSDK)\bin\vrxhdr  -s 250000 -h 300000 -lceif.lib=N:/ceif.lib -lelog.lib=N:/elog.lib -lssl.lib=N:/ssl.lib $(OutDir)\CeSmpl.out	
	java -jar sign.jar $(OutDir)\CeSmpl.out 0 200.160.80.90 1234 
	copy $(OutDir)\CeSmpl.out.p7s $(OutDir)\signfile\ram\CeSmpl.p7s

$(OutDir)\CeSmpl.out : $(AppObjects)
	$(VRXSDK)\bin\vrxcc -g $(COptions) $(Includes) $(AppObjects) $(Libs) -o CeSmpl.out
	move CeSmpl.out		$(OutDir)
	move CeSmpl.axf     $(OutDir)
 
######  Compile #######

$(ObjDir)\str_utils.o : $(SrcDir)\str_utils.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\str_utils.c
	move str_utils.o $(ObjDir)
$(ObjDir)\valor_utils.o : $(SrcDir)\valor_utils.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\valor_utils.c
	move valor_utils.o $(ObjDir)
$(ObjDir)\write_utils.o : $(SrcDir)\write_utils.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\write_utils.c
	move write_utils.o $(ObjDir)
$(ObjDir)\printer.o : $(SrcDir)\printer.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\printer.c
	move printer.o $(ObjDir)
$(ObjDir)\modelo.o : $(SrcDir)\modelo.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\modelo.c
	move modelo.o $(ObjDir)
$(ObjDir)\codebar.o : $(SrcDir)\codebar.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\codebar.c
	move codebar.o $(ObjDir)
$(ObjDir)\bmpfile.o : $(SrcDir)\..\libbmp-0.1.3\src\bmpfile.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\..\libbmp-0.1.3\src\bmpfile.c
	move bmpfile.o $(ObjDir)
$(ObjDir)\bmputils.o : $(SrcDir)\..\libbmp-0.1.3\src\bmputils.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\..\libbmp-0.1.3\src\bmputils.c
	move bmputils.o $(ObjDir)

$(ObjDir)\date.o : $(SrcDir)\date.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\date.c
	move date.o  $(ObjDir)

$(ObjDir)\venda.o : $(SrcDir)\venda.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\venda.c
	move venda.o  $(ObjDir)

$(ObjDir)\ambiente.o : $(SrcDir)\ambiente.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\ambiente.c
	move ambiente.o  $(ObjDir)

$(ObjDir)\ingresso.o : $(SrcDir)\ingresso.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\ingresso.c
	move ingresso.o  $(ObjDir)

$(ObjDir)\menus.o : $(SrcDir)\menus.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\menus.c
	move menus.o  $(ObjDir)

$(ObjDir)\trex.o : $(SrcDir)\trex.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\trex.c
	move trex.o  $(ObjDir)

$(ObjDir)\operadores.o : $(SrcDir)\operadores.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\operadores.c
	move operadores.o  $(ObjDir)

$(ObjDir)\dict.o : $(SrcDir)\dict.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\dict.c
	move dict.o  $(ObjDir)

$(ObjDir)\db.o : $(SrcDir)\db.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\db.c
	move db.o  $(ObjDir)

$(ObjDir)\venda_screen.o : $(SrcDir)\venda_screen.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\venda_screen.c
	move venda_screen.o  $(ObjDir)

$(ObjDir)\login_screen.o : $(SrcDir)\login_screen.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\login_screen.c
	move login_screen.o  $(ObjDir)

$(ObjDir)\startup_screen.o : $(SrcDir)\startup_screen.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\startup_screen.c
	move startup_screen.o  $(ObjDir)

$(ObjDir)\main_screen.o : $(SrcDir)\main_screen.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\main_screen.c
	move main_screen.o  $(ObjDir)

$(ObjDir)\update_screen.o : $(SrcDir)\update_screen.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\update_screen.c
	move update_screen.o  $(ObjDir)

$(ObjDir)\eventos.o : $(SrcDir)\eventos.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\eventos.c
	move eventos.o  $(ObjDir)

$(ObjDir)\relatorio.o : $(SrcDir)\relatorio.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\relatorio.c
	move relatorio.o  $(ObjDir)

$(ObjDir)\relatorio_screen.o : $(SrcDir)\relatorio_screen.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\relatorio_screen.c
	move relatorio_screen.o  $(ObjDir)

$(ObjDir)\http_lib.o : $(SrcDir)\http_lib.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\http_lib.c
	move http_lib.o  $(ObjDir)

$(ObjDir)\tcp.o : $(SrcDir)\tcp.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\tcp.c
	move tcp.o  $(ObjDir)

$(ObjDir)\gprs.o : $(SrcDir)\gprs.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\gprs.c
	move gprs.o  $(ObjDir)

$(ObjDir)\Util.o : $(SrcDir)\Util.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\Util.c
	move Util.o  $(ObjDir)
	
$(ObjDir)\CeSmpl.o : $(SrcDir)\CeSmpl.c
	$(VRXSDK)\bin\vrxcc -c $(COptions) $(Includes) $(SrcDir)\CeSmpl.c
	move CeSmpl.o  $(ObjDir)
