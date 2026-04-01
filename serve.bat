@echo off
setlocal EnableDelayedExpansion

set "PORT=8000"
set "URL=http://localhost:%PORT%/"

pushd "%~dp0"

rem --- Check website folder ---
if not exist "website\index.html" (
    echo [ERROR] website\index.html not found. Please run build.bat first.
    pause
    popd
    exit /b 1
)

rem --- Kill any process using the port ---
for /f "tokens=5" %%p in ('netstat -ano 2^>nul ^| findstr "LISTENING" ^| findstr ":%PORT% "') do (
    taskkill /F /PID %%p >nul 2>&1
)

echo.
echo  Anonymous Dynamic Networks Simulator
echo  URL  : %URL%
echo  Stop : Ctrl+C
echo.

rem --- Start Node.js server ---
start "" "%URL%"
node "%~dp0serve.js" %PORT%

popd
endlocal
