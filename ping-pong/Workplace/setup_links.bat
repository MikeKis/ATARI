@echo off
REM .specs/2026-10-13_ping-pong-imitation-learning.md
REM Symbolic links into ArNI. Nothing is copied.
set HERE=%~dp0
set ARNI=C:\SNN\ArNI\Workplace
if not exist "%ARNI%\ArNIGPU.exe" (
  echo ArNI Workplace not found at %ARNI%
  exit /b 1
)
REM Symlinks need developer mode or an elevated shell. On the same volume, a hard link is the fallback.
for %%F in (ArNIGPU.exe ArNI_GA.exe ArNICPU.exe) do (
  if exist "%ARNI%\%%F" if not exist "%HERE%%%F" (
    mklink "%HERE%%%F" "%ARNI%\%%F" || mklink /H "%HERE%%%F" "%ARNI%\%%F"
  )
)
for %%F in ("%ARNI%\*.dll") do (
  if not exist "%HERE%%%~nxF" (
    mklink "%HERE%%%~nxF" "%%F" || mklink /H "%HERE%%%~nxF" "%%F"
  )
)
if not exist "%HERE%ping-pong-environment.dll" (
  mklink "%HERE%ping-pong-environment.dll" "%HERE%..\Experiments\ping-pong-environment.dll" || mklink /H "%HERE%ping-pong-environment.dll" "%HERE%..\Experiments\ping-pong-environment.dll"
)
if not exist "%HERE%ArNI_OPT.inc" (
  mklink "%HERE%ArNI_OPT.inc" "%HERE%..\Experiments\ImitationPingPong.inc" || mklink /H "%HERE%ArNI_OPT.inc" "%HERE%..\Experiments\ImitationPingPong.inc"
)
echo Workplace links are in %HERE%
echo On the Linux GA machine, also symlink ArNI/ArNI_OPT.inc to ImitationPingPong.inc and rebuild SuperArNI.
