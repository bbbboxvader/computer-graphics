@echo off
if not exist "%~dp0bin\x64\Debug\Practice21\Practice21.exe" call "%~dp0BUILD.cmd"
if not exist "%~dp0bin\x64\Debug\Practice21\Practice21.exe" exit /b 1
"%~dp0bin\x64\Debug\Practice21\Practice21.exe"
if errorlevel 1 pause
