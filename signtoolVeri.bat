@echo off
setlocal enabledelayedexpansion

set count=0
for /f "tokens=*" %%x in (cat.txt) do (
    set /a count+=1
    set var1[!count!]=%%x
)

set c = 0
for /f "tokens=*" %%x in (dri.txt) do (
    set /a c+=1
    set var2[!c!]=%%x
)

::for /f "tokens=*" %%x in (dri.txt) do set var = %%x

SignTool verify /v /pa /c %var1[1]% %var2[1]% >> output.txt
