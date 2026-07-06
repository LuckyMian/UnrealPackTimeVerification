@echo off
chcp 65001 >nul

for /f "delims=" %%i in ('powershell -NoProfile -Command "(Get-CimInstance Win32_Processor).ProcessorId"') do set "CPU_ID=%%i"
for /f "delims=" %%j in ('powershell -NoProfile -Command "((Get-CimInstance Win32_DiskDrive | Select-Object -First 1).SerialNumber).Trim()"') do set "DISK_SN=%%j"

:: 拆分成两行打印
echo CPU=%CPU_ID%;
echo DiskSN=%DISK_SN%;

pause