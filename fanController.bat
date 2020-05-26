@echo off
setlocal EnableDelayedExpansion
TITLE COM3 Monitor starting...
cd C:\TEMP\USRTMP
rem taskkill /IM "cmd.exe" /F
rem powershell "$id = gwmi Win32_Process -Filter \"name = 'cmd.exe'\" | select CommandLine, ProcessID  | Where-Object {$_.CommandLine -match 'fanController'} | select ProcessID; Stop-Process -ID $id.ProcessID"
rem powershell "get-process | where-object {$_.MainWindowTitle -eq 'COM3 Monitor'} | stop-process;Start-Sleep -s 0;"
wmic process where "commandline like '%%new-Object System.IO.Ports.SerialPort%%'" call terminate
TITLE COM3 Monitor
rem curve 4 7 127 200 80 255 20 200
powershell "cd C:\TEMP\USRTMP; Start-Sleep -s 4;$port= new-Object System.IO.Ports.SerialPort COM3,9600,None,8,one; $port.open();$port.Close();$port.open();$port.WriteLine('curve 4 7 127 200 80 255 20 200');$port.ReadLine();$port.ReadLine(); $port.Close();$port.open();$port.WriteLine('map 705 24 628 33');$port.ReadLine();$port.ReadLine(); $port.Close();$port.open();$port.WriteLine('pwm 150');$port.ReadLine();$port.ReadLine();$port.Close();$port.open();$port.WriteLine('pwm 0');$port.ReadLine();$port.ReadLine();$port.Close(); while ($true){ Start-Sleep -s 4; $Results = Get-WmiObject -Namespace "root/wmi" -Class AIDA64_SensorValues; $port= new-Object System.IO.Ports.SerialPort COM3,9600,None,8,one;$port.open();$port.Close();$port.open();$port.WriteLine(\"pc $($Results.Value[1]) $($Results.Value[3]) $($Results.Value[2]) $($Results.Value[0])\");&{$port.ReadLine(); $port.ReadLine();$port.ReadLine();$port.ReadLine();$port.ReadLine()} > 'Fan Controller2.log'; $port.Close(); type 'Fan Controller2.log' > 'Fan Controller.log'; Set-Variable var (cat 'Fan Controller.log'  | %%{$_.Split(' ')});Set-Variable registryPath HKCU:\Software\FinalWire\AIDA64\ImportValues; New-ItemProperty -Path $registryPath -Name Str1 -Value $var[1] -PropertyType STRING -Force | Out-Null ; New-ItemProperty -Path $registryPath -Name Str2 -Value $var[6] -PropertyType STRING -Force | Out-Null ; New-ItemProperty -Path $registryPath -Name Str3 -Value $var[8] -PropertyType STRING -Force | Out-Null; New-ItemProperty -Path $registryPath -Name Str4 -Value $var[12] -PropertyType STRING -Force | Out-Null;}"

