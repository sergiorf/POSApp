# Microsoft Developer Studio Project File - Name="UCLSmpl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=UCLSmpl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "UCLSmpl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "UCLSmpl.mak" CFG="UCLSmpl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "UCLSmpl - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "UCLSmpl - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Universal Communication Library/UCL 1.0/UnitTest/UnitTestPrograms/LLTest", DOJAAAAA"
# PROP Scc_LocalPath "c:\verix\ucl\samples\csmpl"

!IF  "$(CFG)" == "UCLSmpl - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f UCLSmpl.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "UCLSmpl.exe"
# PROP BASE Bsc_Name "UCLSmpl.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "nmake /f "UCLSmpl.smk""
# PROP Rebuild_Opt "/a"
# PROP Target_File "UCLSmpl.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "UCLSmpl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "UCLSmpl___Win32_Debug"
# PROP BASE Intermediate_Dir "UCLSmpl___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f UCLSmpl.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "UCLSmpl.exe"
# PROP BASE Bsc_Name "UCLSmpl.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "UCLSmpl___Win32_Debug"
# PROP Intermediate_Dir "UCLSmpl___Win32_Debug"
# PROP Cmd_Line "nmake /f "UCLSmpl.smk""
# PROP Rebuild_Opt "/a"
# PROP Target_File "UCLSmpl.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "UCLSmpl - Win32 Release"
# Name "UCLSmpl - Win32 Debug"

!IF  "$(CFG)" == "UCLSmpl - Win32 Release"

!ELSEIF  "$(CFG)" == "UCLSmpl - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\Source\Appobj.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\Sender.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Source\util.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\Include\AppObj.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Include\util.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\UCLSmpl.smk
# End Source File
# End Target
# End Project
