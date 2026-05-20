@echo off
REM Wrapper de build pour Windows / cmd.exe.
SET PATH=C:\msys64\ucrt64\bin;C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%
IF NOT DEFINED TMP SET TMP=C:\msys64\tmp
IF NOT DEFINED TEMP SET TEMP=C:\msys64\tmp
IF NOT DEFINED TMPDIR SET TMPDIR=C:\msys64\tmp

IF EXIST "C:\msys64\usr\bin\make.exe" (
    "C:\msys64\usr\bin\make.exe" TMP=C:\msys64\tmp TEMP=C:\msys64\tmp TMPDIR=C:\msys64\tmp %*
    EXIT /B %ERRORLEVEL%
) ELSE IF EXIST "C:\msys64\ucrt64\bin\mingw32-make.exe" (
    "C:\msys64\ucrt64\bin\mingw32-make.exe" TMP=C:\msys64\tmp TEMP=C:\msys64\tmp TMPDIR=C:\msys64\tmp %*
    EXIT /B %ERRORLEVEL%
) ELSE (
    echo Error: make not found. Install MSYS2 make or mingw32-make.
    EXIT /B 1
)
