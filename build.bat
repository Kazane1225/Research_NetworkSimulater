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
  src\main.c src\queue.c src\vector.c src\buffer.c src\shader.c src\font.c src\text.c src\graphics.c src\history_tree.c src\entity.c src\examples.c src\network.c src\auxdata.c src\stabilizing_algo.c src\terminating_algo.c src\compute_job.c src\render.c src\events.c ^
  -O2 ^
  -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 ^
  -s ALLOW_MEMORY_GROWTH=1 -s MAXIMUM_MEMORY=256mb ^
  --preload-file assets\font.fon@/font.fon --preload-file assets\font.tga@/font.tga ^
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
