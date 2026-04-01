@echo off
setlocal EnableDelayedExpansion

set "OPEN_BROWSER=1"
if /i "%~1"=="--no-open" set "OPEN_BROWSER=0"
if /i "%~1"=="--no-browser" set "OPEN_BROWSER=0"

rem Browser: brave | chrome | edge | default
set "BROWSER=brave"
set "PF=%ProgramFiles%"
set "PF86=%ProgramFiles(x86)%"

set PREFIX=C:\Emscripten\libs\sdl3minimal\prefix

pushd "%~dp0"

if not exist website mkdir website
del /f /q "website\index.js" "website\index.data" "website\index.wasm" 2>nul

rem Build (output goes to website\index.js/.wasm/.data)
call emcc ^
  main.c queue.c vector.c buffer.c shader.c font.c text.c graphics.c history_tree.c entity.c examples.c network.c auxdata.c stabilizing_algo.c terminating_algo.c render.c events.c ^
  -O2 ^
  -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 ^
  -s ALLOW_MEMORY_GROWTH=1 -s MAXIMUM_MEMORY=256mb ^
  --preload-file font.fon@/font.fon --preload-file font.tga@/font.tga ^
  -I "%PREFIX%\include" ^
  -L "%PREFIX%\lib" ^
  -lSDL3 ^
  -Wall -Wextra -Wformat -Wshadow -Wundef -Wpointer-arith -Wcast-align -Wstrict-aliasing -Wwrite-strings ^
  -o website\index.js

if errorlevel 1 (
  echo Build failed.
  pause
  popd
  exit /b 1
)

if "%OPEN_BROWSER%"=="1" (
  call "%~dp0serve.bat"
)

popd
endlocal
