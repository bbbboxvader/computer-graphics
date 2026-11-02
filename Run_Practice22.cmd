@echo off
if not exist "%~dp0bin\x64\Debug\Practice22\Practice22.exe" call "%~dp0BUILD.cmd"
if not exist "%~dp0bin\x64\Debug\Practice22\Practice22.exe" exit /b 1
"%~dp0bin\x64\Debug\Practice22\Practice22.exe"
if errorlevel 1 pause
