@echo off

SET WORKDIR=%cd%
cd ..
SET RIOTDIR=%cd%\RIOT
cd %WORKDIR%

echo RIOTDIR:     %RIOTDIR%
echo WORKDIR:     %WORKDIR%
echo.
echo Start docker and build application
echo.
echo.

CALL docker run --rm -t -v %RIOTDIR%:/data/riotbuild/riotbase -v %WORKDIR%:/data/riotbuild/riotbase/examples/app riot/riotbuild:latest bash -C /data/riotbuild/riotbase/examples/app/make.sh
REM CALL docker run --rm -ti -v %RIOTDIR%:/data/riotbuild/riotbase -v %WORKDIR%:/data/riotbuild/riotbase/examples/app riot/riotbuild:latest bash

PAUSE