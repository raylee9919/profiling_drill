@echo off

set CCFLAGS=/std:c++17 -Od -nologo -MTd -GR- -EHa- -Oi -W4 -wd4530 -FC -Z7 -D__PS_INTERNAL_BUILD__=1

if not exist ..\build mkdir ..\build
pushd ..\build

cl %CCFLAGS% ..\src\main.cpp
popd
