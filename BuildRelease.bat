echo on

REM Set VERSION to last git annoted tag
FOR /F %%i IN ('git describe') DO set TAG=%%i
FOR /F %%i IN ('git rev-parse --short HEAD') DO set HASH=%%i
REM SET VERSION=%TAG%-%HASH%
SET VERSION=%TAG%

ECHO %VERSION%

SET ZIP=%ProgramFiles%\7-Zip\7z.exe
SET ARCHIVE_NAME=JammaMia_%VERSION%.zip
SET OUTPUTDIR=JammaMiaRelease
SET BUILDTOOL=BuildTool.exe
set host=%COMPUTERNAME%
echo %host%
SET ARDUINO_CLI=C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe


:version
echo %DATE% %TIME%: Update version (AnyCPU)
"%BUILDTOOL%" -b build.txt -v . "JammaMiaOpen\version.tmpl" "JammaMiaOpen\version.h"

:arduino
"%ARDUINO_CLI%" compile --clean -b arduino:avr:leonardo -e JammaMiaOpen
REM Copy hex to Firmware directory
RMDIR /S /Q "Firmware"
MKDIR "Firmware"
XCOPY /Y "JammaMiaOpen\build\arduino.avr.leonardo\JammaMiaOpen.ino.hex" "Firmware"
CD JammaMiaOpen
"%ZIP%" a -tzip "..\%ARCHIVE_NAME%.zip" .
CD ..
set BUILD_STATUS=%ERRORLEVEL%
if not %BUILD_STATUS%==0 goto fail

:fail
exit /b 1


