@echo off
Rem ember to edit this file and change MINGWDIR to your MinGW directory! (Also watch out, GCC 4.4.0 is used here, that might need changing too). After that remove the following line
goto EOF
set MINGWDIR=f:\mingw
set PATH=%MINGWDIR%\bin;%PATH%
set INCLUDE=%MINGWDIR%\lib\gcc\mingw32\4.4.0\include;%MINGWDIR%\include
make %*

:EOF