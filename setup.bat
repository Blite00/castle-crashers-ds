@echo off
rem Castle Crashers DS setup: finds your own copy of Castle Crashers, converts its assets and builds castle-crashers-ds.nds.
rem Missing Python, Java, ffmpeg and the DS toolchain are installed automatically.
setlocal
cd /d "%~dp0"
call :findpy
if not defined PY (
  where winget >nul 2>nul && (
    echo Python 3 is missing; installing it with winget. Windows may ask for permission.
    winget install -e --id Python.Python.3.13 --scope user --accept-package-agreements --accept-source-agreements --disable-interactivity
    call :findpy
  )
)
if not defined PY (
  echo Python 3 is required. Install it from https://www.python.org/downloads/ ^(tick "Add python.exe to PATH"^), then run this again.
  pause
  exit /b 1
)
"%PY%" tools\setup_assets.py %*
set "RC=%ERRORLEVEL%"
echo.
pause
exit /b %RC%

rem Sets PY to a working python.exe. "python" on PATH can be the Microsoft Store placeholder, so each
rem candidate is asked for its real path; that also gives the steps one fixed interpreter.
:findpy
set "PY="
for %%C in ("py -3" "python") do if not defined PY (
  for /f "delims=" %%P in ('%%~C -c "import sys; print(sys.executable)" 2^>nul') do set "PY=%%P"
)
if not defined PY for /d %%D in ("%LOCALAPPDATA%\Programs\Python\Python3*") do if exist "%%D\python.exe" set "PY=%%D\python.exe"
exit /b 0
