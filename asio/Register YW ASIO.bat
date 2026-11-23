@echo off
rem Registers YW ASIO so that other programs (FL Studio, Reaper, Ableton...) list it as an ASIO driver:
rem yw-asio.dll for 64-bit programs, yw-asio32.dll for 32-bit programs.
rem Needs administrator rights: Windows asks for them. YW Studio itself does not need this step.
net session >nul 2>&1 || (powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs" & exit /b)
"%SystemRoot%\System32\regsvr32.exe" "%~dp0yw-asio.dll"
if exist "%~dp0yw-asio32.dll" if exist "%SystemRoot%\SysWOW64\regsvr32.exe" "%SystemRoot%\SysWOW64\regsvr32.exe" "%~dp0yw-asio32.dll"
