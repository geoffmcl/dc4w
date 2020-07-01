@setlocal
@set TMPEXE=Release\dc4w.exe
@if NOT EXIST %TMPEXE% (
@echo Can NOT locate %TMPEXE%! *** FIX ME ***
@exit /b 1
)

@set TMPDIR1=D:\UTILS\dc4w\test\dir1
@if NOT EXIST %TMPDIR1%\nul (
@echo Can NOT locate %TMPDIR1%! *** FIX ME ***
@exit /b 1
)

@set TMPDIR2=D:\UTILS\dc4w\test\dir2
@if NOT EXIST %TMPDIR2%\nul (
@echo Can NOT locate %TMPDIR2%! *** FIX ME ***
@exit /b 1
)

%TMPEXE% %TMPDIR1% %TMPDIR2% -d:tempd.txt

%TMPEXE% %TMPDIR1% %TMPDIR2%

@REM eof
