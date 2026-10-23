@echo off
setlocal EnableExtensions EnableDelayedExpansion

if /I "%~1"=="-h" goto :help
if /I "%~1"=="--help" goto :help
if /I "%~1"=="/?" goto :help

if /I "%~1"=="--mode" (
  if "%~2"=="" goto :help
  set "MG_MODE=%~2"
)

set "MODE=both"
if defined MG_MODE set "MODE=!MG_MODE!"
if /I not "!MODE!"=="exe" if /I not "!MODE!"=="dll" if /I not "!MODE!"=="both" (
  echo build_windows.bat: --mode must be exe, dll or both
  exit /b 1
)

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
if not exist "%ROOT%\platform\CMakeLists.txt" (
  echo build_windows.bat: platform\CMakeLists.txt not found next to this script
  exit /b 1
)

set "CURL=%SystemRoot%\System32\curl.exe"
set "TAR=%SystemRoot%\System32\tar.exe"
set "CERTUTIL=%SystemRoot%\System32\certutil.exe"
if not exist "%CURL%" echo build_windows.bat: %CURL% is missing. Windows 10 or newer is required & exit /b 1
if not exist "%TAR%" echo build_windows.bat: %TAR% is missing. Windows 10 or newer is required & exit /b 1
if not exist "%CERTUTIL%" echo build_windows.bat: %CERTUTIL% is missing & exit /b 1

set "WORK=%ROOT%\build\mingw"
set "DEPS=%WORK%\deps"
set "PREFIX=%WORK%\win-libs"
set "CACHE=%WORK%\cache"
set "TOOLS=%WORK%\tools"
set "SDL=%WORK%\SDL2-2.30.10"
if defined MG_OUT (set "OUT=%MG_OUT%") else set "OUT=%ROOT%\out\windows\x86_64"
if not exist "%WORK%" mkdir "%WORK%"
if not exist "%CACHE%" mkdir "%CACHE%"

call :fetch winlibs "https://github.com/brechtsanders/winlibs_mingw/releases/download/16.2.0posix-23.1.3-14.0.0-ucrt-r3/winlibs-x86_64-posix-seh-gcc-16.2.0-mingw-w64ucrt-14.0.0-r3.zip" 3c69305081e4a647144339a936244fcf7d3d9e558319e1ad7be2cd2886d4b6a1 "%TOOLS%" 0 || exit /b 1

set "BINDIR=%TOOLS%\mingw64\bin\"
set "GXX=%BINDIR%g++.exe"
set "GCC=%BINDIR%gcc.exe"
set "CMAKE=%BINDIR%cmake.exe"
set "WINDRES=%BINDIR%windres.exe"
set "MAKE=%BINDIR%mingw32-make.exe"
set "OBJDUMP=%BINDIR%objdump.exe"
if not exist "%GXX%" echo build_windows.bat: g++.exe missing from the toolchain & exit /b 1
if not exist "%GCC%" echo build_windows.bat: gcc.exe missing from the toolchain & exit /b 1
if not exist "%CMAKE%" echo build_windows.bat: cmake.exe missing from the toolchain & exit /b 1
if not exist "%WINDRES%" echo build_windows.bat: windres.exe missing from the toolchain & exit /b 1
if not exist "%MAKE%" echo build_windows.bat: mingw32-make.exe missing from the toolchain & exit /b 1
if not exist "%OBJDUMP%" echo build_windows.bat: objdump.exe missing from the toolchain & exit /b 1

set "PATH=%BINDIR%;%SystemRoot%\System32;%SystemRoot%"

"%GXX%" -v > "%TEMP%\mg-gxx.txt" 2>&1
"%SystemRoot%\System32\findstr.exe" /C:"Thread model: posix" "%TEMP%\mg-gxx.txt" >nul
if errorlevel 1 (
  echo build_windows.bat: downloaded g++ is not the posix thread model
  exit /b 1
)
set "MACHINE="
for /f "delims=" %%M in ('"%GXX%" -dumpmachine') do set "MACHINE=%%M"
if /I not "!MACHINE!"=="x86_64-w64-mingw32" (
  echo build_windows.bat: g++ targets !MACHINE!, wanted x86_64-w64-mingw32
  exit /b 1
)
echo ==^> compiler !GXX!

set "CMAKE_SH=CMAKE_SH-NOTFOUND"
set "JOBS=%NUMBER_OF_PROCESSORS%"
if defined MG_JOBS set "JOBS=%MG_JOBS%"

call :fetch bx "https://github.com/bkaradzic/bx/archive/59a1e105b6f5dc4600c8bdc24b32499e135df73c.tar.gz" 753db194f0b975cdc7e4824a90de1be9566256e2e091d12b4a6118a55e76c759 "%DEPS%\bx" || exit /b 1
call :fetch bimg "https://github.com/bkaradzic/bimg/archive/671fe06dd7012a1ab85aa931019ed16112909756.tar.gz" 8149e79b567db0ba5d7246910ffa82d13b03b6192ce69fefd3a82e07d13ec7e3 "%DEPS%\bimg" || exit /b 1
call :fetch bgfx "https://github.com/bkaradzic/bgfx/archive/73183bea9b92766cfaac2e6f757d0bcc3c0253ef.tar.gz" df8cb756e1552c2608abda18addc62035618dc3a26e835ee692b8519ea1daf06 "%DEPS%\bgfx" || exit /b 1
call :fetch bgfx.cmake "https://github.com/bkaradzic/bgfx.cmake/archive/dba713af2d5a9acd7bd6d6a87498fd8751db6015.tar.gz" 8870e62b5048d32cb7dcbcd634bd903d99386404927fd23a7da9386751495f50 "%DEPS%\bgfx.cmake" || exit /b 1
call :fetch zlib "https://github.com/madler/zlib/archive/refs/tags/v1.2.11.tar.gz" 629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff "%WORK%\zlib" || exit /b 1
call :fetch libpng "https://github.com/glennrp/libpng/archive/refs/tags/v1.6.37.tar.gz" ca74a0dace179a8422187671aee97dd3892b53e168627145271cad5b5ac81307 "%WORK%\libpng" || exit /b 1
call :fetch sdl2-mingw "https://github.com/libsdl-org/SDL/releases/download/release-2.30.10/SDL2-devel-2.30.10-mingw.tar.gz" a7763f9439ea25685b053e9257dac1eac012e5cd0824f1a801b27b1d92ebe321 "%SDL%" || exit /b 1

if not exist "%SDL%\x86_64-w64-mingw32\lib\libSDL2.dll.a" (
  echo build_windows.bat: SDL2 MinGW tree is incomplete under %SDL%
  exit /b 1
)
if /I not "!MODE!"=="exe" if not exist "%SDL%\x86_64-w64-mingw32\lib\libSDL2.a" (
  echo build_windows.bat: dll mode needs libSDL2.a
  exit /b 1
)

set "PNG_A=%PREFIX%\lib\libpng16.a"
set "ZLIB_A=%PREFIX%\lib\libzlibstatic.a"
if not exist "%PNG_A%" goto :build_libs
if not exist "%ZLIB_A%" goto :build_libs
goto :libs_ready

:build_libs
echo ==^> building zlib and libpng
set CM_SRC=%WORK%\zlib
set CM_BUILD=%WORK%\zlib-build
set CM_EXTRA=-DCMAKE_INSTALL_PREFIX="%PREFIX%" -DBUILD_SHARED_LIBS=OFF
call :cmake_cfg || exit /b 1
"%CMAKE%" --build "%WORK%\zlib-build" --target install -j %JOBS% || exit /b 1
if not exist "%ZLIB_A%" set "ZLIB_A=%PREFIX%\lib\libz.a"
if not exist "%ZLIB_A%" (
  echo build_windows.bat: static zlib was not installed under %PREFIX%
  exit /b 1
)
set CM_SRC=%WORK%\libpng
set CM_BUILD=%WORK%\libpng-build
set CM_EXTRA=-DCMAKE_INSTALL_PREFIX="%PREFIX%" -DPNG_SHARED=OFF -DPNG_TESTS=OFF -DPNG_BUILD_ZLIB=OFF -DZLIB_INCLUDE_DIR="%PREFIX%\include" -DZLIB_LIBRARY="%ZLIB_A%"
call :cmake_cfg || exit /b 1
"%CMAKE%" --build "%WORK%\libpng-build" --target install -j %JOBS% || exit /b 1

:libs_ready
if not exist "%PNG_A%" (
  echo build_windows.bat: libpng16.a is missing under %PREFIX%
  exit /b 1
)
if not exist "%ZLIB_A%" set "ZLIB_A=%PREFIX%\lib\libz.a"
if not exist "%ZLIB_A%" (
  echo build_windows.bat: static zlib is missing under %PREFIX%
  exit /b 1
)

echo ==^> configuring !MODE!
set CM_SRC=%ROOT%\platform
set CM_BUILD=%WORK%\build
set CM_EXTRA=-DMG_DEPS="%DEPS%" -DMG_SDL2_ROOT="%SDL%" -DMG_PNG_INCLUDE="%PREFIX%\include" -DMG_PNG_LIBRARY="%PNG_A%" -DMG_ZLIB_INCLUDE="%PREFIX%\include" -DMG_ZLIB_LIBRARY="%ZLIB_A%" -DMG_TARGET=!MODE!
call :cmake_cfg || exit /b 1
"%CMAKE%" --build "%WORK%\build" -j %JOBS% || exit /b 1

if not exist "%OUT%" mkdir "%OUT%"

if /I "!MODE!"=="dll" goto :do_dll
if not exist "%WORK%\build\matrixgame.exe" (
  echo build_windows.bat: matrixgame.exe was not produced
  exit /b 1
)
copy /y "%WORK%\build\matrixgame.exe" "%OUT%\matrixgame.exe" >nul || exit /b 1
copy /y "%SDL%\x86_64-w64-mingw32\bin\SDL2.dll" "%OUT%\SDL2.dll" >nul || exit /b 1
echo ==^> built %OUT%\matrixgame.exe
"%OBJDUMP%" -p "%OUT%\matrixgame.exe" | "%SystemRoot%\System32\findstr.exe" /I /C:"libwinpthread" >nul
if not errorlevel 1 (
  if not exist "%BINDIR%libwinpthread-1.dll" (
    echo build_windows.bat: matrixgame.exe imports libwinpthread-1.dll and it is not next to g++
    exit /b 1
  )
  copy /y "%BINDIR%libwinpthread-1.dll" "%OUT%\libwinpthread-1.dll" >nul || exit /b 1
)

:do_dll
if /I "!MODE!"=="exe" goto :done
if not exist "%WORK%\build\MatrixGame.dll" (
  echo build_windows.bat: MatrixGame.dll was not produced
  exit /b 1
)
copy /y "%WORK%\build\MatrixGame.dll" "%OUT%\MatrixGame.dll" >nul || exit /b 1
echo ==^> built %OUT%\MatrixGame.dll
"%OBJDUMP%" -p "%OUT%\MatrixGame.dll" > "%WORK%\imports.txt"
"%SystemRoot%\System32\findstr.exe" /I /C:"DLL Name:" "%WORK%\imports.txt" > "%WORK%\dllnames.txt"
if errorlevel 1 (
  echo build_windows.bat: could not read the import table of MatrixGame.dll
  exit /b 1
)
"%SystemRoot%\System32\findstr.exe" /I "SDL2 libpng zlib winpthread stdc gcc_s" "%WORK%\dllnames.txt" >nul
if not errorlevel 1 (
  echo build_windows.bat: MatrixGame.dll still imports a library it should carry itself
  type "%WORK%\dllnames.txt"
  exit /b 1
)
"%SystemRoot%\System32\findstr.exe" /C:"GetRobotInterface" "%WORK%\imports.txt" >nul
if errorlevel 1 (
  echo build_windows.bat: MatrixGame.dll does not export GetRobotInterface
  exit /b 1
)
echo ==^> MatrixGame.dll is self-contained and exports GetRobotInterface

:done
echo ==^> artifacts in %OUT%
echo ==^> note that .exe requires SDL2.dll, but .dll does not
echo ==^> put MatrixGame.dll next to the host Rangers.exe
exit /b 0

:cmake_cfg
set "C_SRC=!CM_SRC:\=/!"
set "C_BUILD=!CM_BUILD:\=/!"
set "C_CC=!GCC:\=/!"
set "C_CXX=!GXX:\=/!"
set "C_RC=!WINDRES:\=/!"
set "C_MAKE=!MAKE:\=/!"
set "CM_EXTRA=!CM_EXTRA:\=/!"
"!CMAKE!" -S "!C_SRC!" -B "!C_BUILD!" -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_C_COMPILER="!C_CC!" ^
  -DCMAKE_CXX_COMPILER="!C_CXX!" ^
  -DCMAKE_RC_COMPILER="!C_RC!" ^
  -DCMAKE_MAKE_PROGRAM="!C_MAKE!" ^
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
  !CM_EXTRA!
if errorlevel 1 exit /b 1
exit /b 0

:fetch
set "FNAME=%~1"
set "FURL=%~2"
set "FWANT=%~3"
set "FDEST=%~4"
set "FSTRIP=1"
if not "%~5"=="" set "FSTRIP=%~5"
set "FEXT=tar.gz"
if /I "!FURL:~-4!"==".zip" set "FEXT=zip"
set "FSTAMP=%FDEST%\.mg-fetch-sha"
if exist "%FSTAMP%" if exist "%FDEST%\" (
  set /p FHAVE=<"%FSTAMP%"
  if /I "!FHAVE!"=="!FWANT!" goto :eof
)
set "FARC=%CACHE%\!FNAME!.!FEXT!"
set "FNEED=1"
if exist "!FARC!" (
  call :sha "!FARC!"
  if /I "!GOTSHA!"=="!FWANT!" set "FNEED="
)
if not defined FNEED goto :fetched
echo ==^> downloading !FNAME!
"%CURL%" -fsSL --retry 3 -L --output "!FARC!.part" "!FURL!"
if errorlevel 1 (
  echo build_windows.bat: download failed for !FNAME!
  exit /b 1
)
move /y "!FARC!.part" "!FARC!" >nul
if errorlevel 1 (
  echo build_windows.bat: could not save !FARC!
  exit /b 1
)
call :sha "!FARC!"
if /I not "!GOTSHA!"=="!FWANT!" (
  echo build_windows.bat: sha256 mismatch for !FNAME!
  echo got !GOTSHA!
  del /f /q "!FARC!"
  exit /b 1
)
:fetched
echo ==^> unpacking !FNAME!
if exist "%FDEST%" rmdir /s /q "%FDEST%"
mkdir "%FDEST%" || exit /b 1
if "!FSTRIP!"=="0" (
  "%TAR%" -xf "!FARC!" -C "%FDEST%" || exit /b 1
) else (
  "%TAR%" -xf "!FARC!" -C "%FDEST%" --strip-components=!FSTRIP! || exit /b 1
)
> "%FSTAMP%" echo !FWANT!
goto :eof

:sha
set "GOTSHA="
"%CERTUTIL%" -hashfile "%~1" SHA256 > "%TEMP%\mg-sha.txt" 2>&1
for /f "usebackq skip=1 delims=" %%H in ("%TEMP%\mg-sha.txt") do (
  if not defined GOTSHA (
    set "LINE=%%H"
    set "LINE=!LINE: =!"
    echo(!LINE!| "%SystemRoot%\System32\findstr.exe" /R "^[0-9A-Fa-f][0-9A-Fa-f]*$" >nul && set "GOTSHA=!LINE!"
  )
)
goto :eof

:help
echo Usage: build_windows.bat [--mode exe^|dll^|both]
exit /b 0
