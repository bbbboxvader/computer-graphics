@echo off
if not exist "%~dp0bin\x64\Debug\Practice18\Practice18.exe" call "%~dp0BUILD.cmd"
if not exist "%~dp0bin\x64\Debug\Practice18\Practice18.exe" exit /b 1
"%~dp0bin\x64\Debug\Practice18\Practice18.exe"
if errorlevel 1 pause
