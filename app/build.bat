@echo off

SET WORKDIR=%cd%
cd ..\..
SET RIOTDIR=%cd%\RIOT-2020.01-ssv
cd %WORKDIR%

echo RIOTDIR:     %RIOTDIR%
echo WORKDIR:     %WORKDIR%
echo.
echo Start docker and build application
echo.
echo.

CALL docker run --rm -ti -v %RIOTDIR%:/data/riotbuild/riotbase -v %WORKDIR%:/data/riotbuild/riotbase/examples/app riot/riotbuild:latest make app -C /data/riotbuild/riotbase/examples/app RIOTBASE=/data/riotbuild/riotbase
REM CALL docker run --rm -ti -v %RIOTDIR%:/data/riotbuild/riotbase -v %WORKDIR%:/data/riotbuild/riotbase/examples/app riot/riotbuild:latest bash

PAUSE
