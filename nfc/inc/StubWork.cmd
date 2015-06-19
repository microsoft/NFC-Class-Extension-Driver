@echo off
setlocal ENABLEDELAYEDEXPANSION

set _PUBLIC_PREFIX=NfcCx
set _PRIVATE_PREFIX=NfcCxPri
set _FRAMEWORK_NAME=%_PUBLIC_PREFIX%

rem
rem tools are built out of the drivers\wdf\kmdf\tools\src tree and 
rem this script runs them from the binaries directory.	
rem

set TOOLS_DIR=%_NTTREE%\wdf\tools


rem
rem Special case WDF framework and setup previous version checking
rem
rem NOTE: do not join these two blocks, as the second used the modified version of
rem       _FRAMEWORK_NAME
rem

set _PREFIX=-prefix:%_PUBLIC_PREFIX%:%_PRIVATE_PREFIX%

if "%1" == "open"   goto :open
if "%1" == "regen"  goto :regen
if "%1" == "revert"  goto :revert
if "%1" == "close"  goto :close
rem if "%1" == "snapshot"  goto :snapshot

:usage
echo ------------------------------------------------
echo usage: stubwork [ open or regen or revert or close ]
echo ------------------------------------------------
goto :done

:open
echo ------------------------------------------------
echo    Opening archived files for edit.
echo ------------------------------------------------
sd edit *.h
sd edit *.w
sd edit *.x
sd edit *.y

sd edit %_PUBLIC_PREFIX%stubs.xml
sd edit %_PUBLIC_PREFIX%stubs.xsd

sd edit stubs\publictypes.h
sd edit stubs\%_FRAMEWORK_NAME%.version.data.xml
sd edit stubs\%_FRAMEWORK_NAME%.version.data.xsd

sd edit private\%_PRIVATE_PREFIX%Dynamics.h
rem sd edit private\Vf%_PRIVATE_PREFIX%Dynamics.h

rem sd edit private\VfEventHooks.hpp
sd edit ..\nfccx\vercheck\cur\types.h
sd edit ..\nfccx\vercheck\cur\%_PRIVATE_PREFIX%checks.h
rem sd edit ..\nfccx\verifier\Vf%_PUBLIC_PREFIX%Dynamics.cpp
rem sd edit ..\nfccx\verifier\VfEventHooks.cpp

goto :done

:regen

echo -------------------------------------------------------------
echo    Convert: extract DDIs and enums from *.Y files into XML DB
echo -------------------------------------------------------------
echo %TOOLS_DIR%\StubConvert %_PREFIX%
     %TOOLS_DIR%\StubConvert %_PREFIX%

echo -------------------------------------------------------------
echo    Generate API *.W files from XML and *.X files 
echo -------------------------------------------------------------
echo %TOOLS_DIR%\StubGen %_PREFIX% %_PREVVER% -ver:stubs\%_FRAMEWORK_NAME%.version.data.xml -outputdir:.
     %TOOLS_DIR%\StubGen %_PREFIX% %_PREVVER% -ver:stubs\%_FRAMEWORK_NAME%.version.data.xml -outputdir:.

echo -------------------------------------------------------------
echo    Generate local *.H files from *.W files
echo -------------------------------------------------------------
echo copy *.w *.h
     copy *.w *.h

rem echo -------------------------------------------------------------
rem echo    Generate verifier .cpp and .h files
rem echo -------------------------------------------------------------
rem echo %TOOLS_DIR%\StubGenVrf %_PREFIX% %_PREVVER% -ver:stubs\%_FRAMEWORK_NAME%.version.data.xml
rem      %TOOLS_DIR%\StubGenVrf %_PREFIX% %_PREVVER% -ver:stubs\%_FRAMEWORK_NAME%.version.data.xml

echo -------------------------------------------------------------
echo    Generate Trace ItemEnum defintions
echo -------------------------------------------------------------
echo %TOOLS_DIR%\StubTrace %_PREFIX%
     %TOOLS_DIR%\StubTrace %_PREFIX%

echo -----------------------------------------------------------------------
echo    Generate structure checks and previous version header checks
echo -----------------------------------------------------------------------
echo %TOOLS_DIR%\StubStruct  %_PREFIX% %_PREVSTUBS% %_PREVVERNUM% -ver:stubs\%_FRAMEWORK_NAME%.version.data.xml -publictypes:stubs\publictypes.h -checksdir:..\nfccx\vercheck\cur\
     %TOOLS_DIR%\StubStruct  %_PREFIX% %_PREVSTUBS% %_PREVVERNUM% -ver:stubs\%_FRAMEWORK_NAME%.version.data.xml -publictypes:stubs\publictypes.h -checksdir:..\nfccx\vercheck\cur\

REM echo -------------------------------------------------------------
REM echo    Copy private files to private directory
REM echo -------------------------------------------------------------
copy %_PRIVATE_PREFIX%Dynamics.h      private\%_PRIVATE_PREFIX%Dynamics.h
rem copy Vf%_PRIVATE_PREFIX%Dynamics.h    private\Vf%_PRIVATE_PREFIX%Dynamics.h
rem copy VfEventHooks.hpp   private\VfEventHooks.hpp
rem copy Vf%_PUBLIC_PREFIX%Dynamics.cpp  ..\nfccx\verifier\Vf%_PUBLIC_PREFIX%Dynamics.cpp
rem copy VfEventHooks.cpp  ..\nfcclx\verifier\VfEventHooks.cpp
del %_PRIVATE_PREFIX%Dynamics.h      
rem del Vf%_PRIVATE_PREFIX%dynamics.h
rem del Vf%_PUBLIC_PREFIX%dynamics.cpp
rem del VfEventHooks.cpp
rem del VfEventHooks.hpp
goto :done

:revert
echo ------------------------------------------------
echo    Reverting Un-modified files
echo ------------------------------------------------
sd revert -a ...

pushd ..\private
sd revert -a *
popd

pushd ..\nfccx\vercheck\cur
sd revert -a *
popd

rem pushd ..\nfccx\verifier
rem sd revert -a *
rem popd
goto :done

:close
echo ------------------------------------------------
echo    Reverting Un-modified files
echo ------------------------------------------------
sd revert -a

echo ------------------------------------------------
echo    Submitting modified files
echo ------------------------------------------------
sd submit ...
rem sd submit private\%_PRIVATE_PREFIX%Dynamics.h

goto :done

rem :snapshot
rem if "%2" == "" (
rem     echo You must provide the released version name as the 2nd parameter to snapshot.
rem     echo .
rem     goto :done
rem )
rem 
rem if "%3" == "" (
rem     echo You must provide the previous version name as the 3rd parameter to snapshot.
rem     echo .
rem     goto :done
rem )
rem 
rem set _STUBWORK_SNAPSHOT_PREVVER=-prevver:..\%3\stubs\%_FRAMEWORK_NAME%.version.data.xml
rem md ..\private\%2
rem 
rem echo ------------------------------------------------
rem echo    Copying files to %2
rem echo ------------------------------------------------
rem 
rem copy %NAMESPACE%.w ..\private\%2
rem copy *.x ..\private\%2
rem copy *.y ..\private\%2
rem 
rem set _STUBWORK_PREV_DIR=%CD:public=private%
rem 
rem rem remove the '.' for the version
rem set _STUBWORK_VER=%2
rem set _STUBWORK_VER=%_STUBWORK_VER:.=%
rem 
rem pushd ..\private\%2
rem 
rem rem the src files are probably read only, make sure we can modify the copies
rem attrib -r *
rem 
rem echo -----------------------------------------------------------------------
rem echo    Converting the snapshoted files to XML
rem echo -----------------------------------------------------------------------
rem %TOOLS_DIR%\StubConvert
rem 
rem echo -----------------------------------------------------------------------
rem echo    Generate API *.W files from XML and *.X files
rem echo -----------------------------------------------------------------------
rem %TOOLS_DIR%\StubGen -hdrtagver:%2 %_STUBWORK_SNAPSHOT_PREVVER% -ver:stubs\%_FRAMEWORK_NAME%.version.data.xml
rem 
rem rem we don't need %_PRIVATE_PREFIX%Dynamics.h and the file is pretty big to begin with (450+kb)
rem rem so don't keep it
rem del %_PRIVATE_PREFIX%dynamics.h
rem 
rem echo -----------------------------------------------------------------------
rem echo    Generate structure checks and previous version headers
rem echo -----------------------------------------------------------------------
rem %TOOLS_DIR%\StubStruct -ver:%2 -publictypes:%_STUBWORK_PREV_DIR%\%_PREFIX%%_STUBWORK_VER%.h
rem 
rem echo -----------------------------------------------------------------------
rem echo    Renaming *.w files to *.h (because they are not being published)
rem echo -----------------------------------------------------------------------
rem ren *.w *.h
rem 
rem echo -----------------------------------------------------------------------
rem echo    Adding the files to SD
rem echo -----------------------------------------------------------------------
rem sd add *
rem sd add %_STUBWORK_PREV_DIR%\%_PREFIX%%_STUBWORK_VER%.h
rem 
rem rem move back into ..\..\public
rem popd
rem 
rem goto :done

:done
echo done.

