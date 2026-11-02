@echo off
if not exist "%~dp0bin\x64\Debug\Practice20\Practice20.exe" call "%~dp0BUILD.cmd"
if not exist "%~dp0bin\x64\Debug\Practice20\Practice20.exe" exit /b 1
"%~dp0bin\x64\Debug\Practice20\Practice20.exe"
if errorlevel 1 pause
