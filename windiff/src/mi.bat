
@setlocal

@set TMPOBJS=gutils gbit gmem gdate status list table tprint tpaint tscroll tree utils

@for %%i in (%TMPOBJS%) do @(call :CHKIT %%i)

@goto END

:CHKIT
@if "%~1x" == "x" goto EOF
@echo %1
@move %1.* lib
@goto :EOF


:End
