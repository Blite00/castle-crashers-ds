@echo off
rem Castle Crashers DS setup: finds your own copy of Castle Crashers, converts its assets and builds castle-crashers-ds.nds.
setlocal
cd /d "%~dp0"
set "PY="
where py >nul 2>nul && set "PY=py -3"
if not defined PY (where python >nul 2>nul && set "PY=python")
if not defined PY (
  echo Python 3 is required. Install it from https://www.python.org/downloads/ ^(tick "Add python.exe to PATH"^), then run this again.
  pause
  exit /b 1
)
%PY% tools\setup_assets.py %*
set "RC=%ERRORLEVEL%"
echo.
pause
exit /b %RC%
