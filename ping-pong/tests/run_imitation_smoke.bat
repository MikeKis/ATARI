@echo off
setlocal
REM .specs/2026-10-13_ping-pong-imitation-learning.md
set ROOT=%~dp0..
set EXP=%ROOT%\Experiments
set WP=%ROOT%\Workplace
cd /d "%WP%"
if not exist ArNIGPU.exe call "%WP%\setup_links.bat"
if not exist ArNICPU.exe (
  echo ArNICPU.exe is not linked into ping-pong\Workplace
  exit /b 1
)
if not exist ArNIGPU.exe (
  echo ArNIGPU.exe is not linked into ping-pong\Workplace
  exit /b 1
)
if not exist ping-pong-environment.dll (
  echo ping-pong-environment.dll is not linked into ping-pong\Workplace
  exit /b 1
)

call :run 901 teacher-echo.nnc -T3000
if errorlevel 1 exit /b 1
move /Y ping_pong_state.csv echo_teacher_state.csv >nul
REM A forcing spike is delivered on the next tact, so it is scored in the next
REM decision. A command change therefore leaves one FP. Same-decision misses are FN.
powershell -NoProfile -Command ^
  "$t=Get-Content echo_teacher_report.txt -Raw;" ^
  "if($t -notmatch 'fp_down (\d+)'){ exit 1 }; $fd=[int]$Matches[1];" ^
  "if($t -notmatch 'fp_up (\d+)'){ exit 1 }; $fu=[int]$Matches[1];" ^
  "if($t -notmatch 'fn_down (\d+)'){ exit 1 }; $nd=[int]$Matches[1];" ^
  "if($t -notmatch 'fn_up (\d+)'){ exit 1 }; $nu=[int]$Matches[1];" ^
  "if($fd -gt 2 -or $fu -gt 2 -or $nd -lt 1 -or $nu -lt 1){ Write-Host \"echo fp $fd $fu fn $nd $nu\"; exit 1 }"
echo teacher echo FP check passed

call :run 902 teacher-echo-agent.nnc -T3000
if errorlevel 1 exit /b 1
move /Y ping_pong_state.csv echo_agent_state.csv >nul
powershell -NoProfile -Command ^
  "$P=15; $t=Import-Csv echo_teacher_state.csv; $a=Import-Csv echo_agent_state.csv;" ^
  "$tm=@{}; foreach($r in $t){$tm[[int]$r.ntact]=[double]$r.ry};" ^
  "$bad=0; foreach($r in $a){$n=[int]$r.ntact; if($n -ge $P -and $n -le 80){$u=$n-$P; if($tm.ContainsKey($u)){ if([math]::Abs([double]$r.ry - $tm[$u]) -gt 1e-4){$bad++}}}};" ^
  "if($bad -gt 0){ Write-Host \"agent echo mismatch $bad\"; exit 1 }"
if errorlevel 1 exit /b 1
echo agent echo trajectory check passed

del /Q ping_pong_state.csv ping_pong_state_discrete.csv 2>nul
call :run 903 diversity.nnc -T8000
if errorlevel 1 exit /b 1
powershell -NoProfile -Command ^
  "$s=Select-String -Path diversity_report.txt -Pattern 'imitation_summary left=(\d+) down=(\d+) up=(\d+) jitter=(\d+)';" ^
  "if(-not $s){ exit 1 }; $m=$s.Matches[0].Groups;" ^
  "$left=[int]$m[1].Value; $down=[int]$m[2].Value; $up=[int]$m[3].Value; $jit=[int]$m[4].Value;" ^
  "if($down -lt 1 -or $up -lt 1){ Write-Host 'diversity: both actions required'; exit 1 };" ^
  "if($jit -lt [math]::Floor($left/20) -or $jit -gt [math]::Ceiling($left/5.0)){ Write-Host \"diversity jitter $jit left $left\"; exit 1 };" ^
  "$rows=Import-Csv ping_pong_state.csv; $prev=$null; $ry=$null;" ^
  "foreach($r in $rows){ if([double]$r.vx -ge 0){ if($prev -and [double]$prev.vx -ge 0 -and [math]::Abs([double]$r.ry-$ry) -gt 1e-5){ Write-Host 'racket moved while ball flew right'; exit 1 }; $ry=[double]$r.ry }; $prev=$r }"
if errorlevel 1 exit /b 1
echo diversity check passed

call :expect_fail 904 bad-action-period.nnc action_period
if errorlevel 1 exit /b 1
call :expect_fail 905 bad-output-neurons.nnc nOutputNeurons
if errorlevel 1 exit /b 1
call :expect_fail 906 bad-jitter-every.nnc jitter_every
if errorlevel 1 exit /b 1
echo config errors passed

del /Q ping_pong_state.csv ping_pong_state_discrete.csv 2>nul
call :run 907 legacy-defaults.nnc -T30
if errorlevel 1 exit /b 1
if exist ping_pong_state.csv (
  echo legacy defaults wrote a state dump
  exit /b 1
)
echo legacy defaults passed

call :run 908 ImitationPingPong.nnc -T75000 -f45000
if errorlevel 1 exit /b 1
findstr /C:"Optmizing absolute WTA section with 2 neurons" 908.console.txt General908.log >nul
if errorlevel 1 (
  echo missing OUT WTA log line
  exit /b 1
)
findstr /C:",learn," imitation_smoke_report.txt >nul
if errorlevel 1 (
  echo learning curve has no learn row
  exit /b 1
)
findstr /C:",test," imitation_smoke_report.txt >nul
if errorlevel 1 (
  echo learning curve has no test row
  exit /b 1
)
echo imitation smoke passed
exit /b 0

:run
copy /Y "%EXP%\%2" "%1.nnc" >nul
echo --- %2 ---
ArNICPU.exe . -e%1 %3 %4 > "%1.console.txt" 2>&1
set RC=%ERRORLEVEL%
type "%1.console.txt"
del /Q "%1.nnc" 2>nul
if %RC% LSS 0 exit /b 1
exit /b 0

:expect_fail
copy /Y "%EXP%\%2" "%1.nnc" >nul
echo --- %2 ---
ArNICPU.exe . -e%1 -T20 > "%1.console.txt" 2>&1
set RC=%ERRORLEVEL%
findstr /C:"%3" "%1.console.txt"
set FOUND=%ERRORLEVEL%
del /Q "%1.nnc" "%1.console.txt" 2>nul
if %RC% EQU 0 exit /b 1
if %FOUND% NEQ 0 exit /b 1
exit /b 0
