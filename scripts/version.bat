@echo off
if defined QNAPI_DISPLAYABLE_VERSION (
  echo %QNAPI_DISPLAYABLE_VERSION%
  goto :eof
)
set VERSION_FILE=libqnapi\src\version.h
for /f "tokens=3" %%A in ('findstr /C:"#define QNAPI_DISPLAYABLE_VERSION " %VERSION_FILE%') do (
  set VERSION_TOKEN=%%A
)
set VERSION_TOKEN=%VERSION_TOKEN:"=%
echo %VERSION_TOKEN%
