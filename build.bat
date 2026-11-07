@echo off
setlocal
where g++ >nul 2>nul
if errorlevel 1 (
  echo MinGW-w64 g++ was not found. Install MinGW-w64 and add its bin folder to PATH.
  exit /b 1
)
g++ -std=c++17 -O2 -static -static-libgcc -static-libstdc++ -municode -mwindows KeyDesk.cpp -o KeyDesk.exe -lcomctl32 -lcomdlg32 -lshell32 -lgdi32 -luser32
if errorlevel 1 exit /b %errorlevel%
echo Built KeyDesk.exe
