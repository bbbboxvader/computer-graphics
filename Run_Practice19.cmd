@echo off
if not exist "%~dp0bin\x64\Debug\Practice19\Practice19.exe" call "%~dp0BUILD.cmd"
if not exist "%~dp0bin\x64\Debug\Practice19\Practice19.exe" exit /b 1
"%~dp0bin\x64\Debug\Practice19\Practice19.exe"
if errorlevel 1 pause
