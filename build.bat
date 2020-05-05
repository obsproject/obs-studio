@echo off

setlocal
set ERRORLEVEL=0
endlocal

goto :MAIN

:: @function to check environment variables/dependencies for this project
:-check
    echo Checking required dependencies... 
    if not defined QTDIR goto :error 
    if not defined obsInstallerTempDir goto :error 
    if not defined DepsPath goto :error 
    if not defined LIBCAFFEINE_DIR goto :error 
    if not defined CEF_ROOT_DIR goto :error
    echo Successfully found all the dependencies.
    goto:EOF


:: @function to clean the build directories
:clean
    IF EXIST %~1 (
    del /q /f %~1\* && for /d %%x in (%~1\*) do @rd /s /q "%%x"
    ) ELSE (
    mkdir %~1
    )
    goto:EOF


:: @function to build 64 bit version of COBS
:-build 
    call :-check
    if ERRORLEVEL 1 goto:EOF
    call :clean build64
    echo Building 64 bit version 
    cmake -H. -Bbuild64 -G "Visual Studio 16 2019" -A"x64" -T"host=x64" -DCOPIED_DEPENDENCIES=OFF -DENABLE_SCRIPTING=ON -DENABLE_UI=ON -DCOMPILE_D3D12_HOOK=ON -DDepsPath="%DepsPath%" -DQTDIR="%QTDIR%" -DLIBCAFFEINE_DIR="%LIBCAFFEINE_DIR%" -DCEF_ROOT_DIR="%CEF_ROOT_DIR%" -DBUILD_BROWSER=ON 
    cmake --build build64 --config RelWithDebInfo
    echo Built 64 bit COBS
    goto:EOF


:: @function to create package COBS
:-package
    call :-check 
    if ERRORLEVEL 1 goto:EOF
    call :clean cmbuild
    echo Configuring and generating cmbuild...
    cmake -H. -Bcmbuild -G "Visual Studio 16 2019" -A"x64" -T"host=x64" -DINSTALLER_RUN=ON -DCMAKE_INSTALL_PREFIX=%obsInstallerTempDir%
    cmake --build cmbuild --config RelWithDebInfo --target PACKAGE
    echo Build complete.    
    goto:EOF


:: @function to print the supported options 
:-help 
    echo Supported options are:
    echo -help : Print this output.
    echo -check : Checks environment variables/ dependencies for this project.
    echo -build : Builds 64 bit version of obs. 
    echo -package : Builds package.
    goto:EOF


:: @function for error handling
:error
    echo Error missing dependencies.Check if environment variables are set for QTDIR, obsInstallerTempDir, DepsPath, LIBCAFFEINE_DIR or CEF_ROOT_DIR.
    EXIT /b 1
    goto:EOF


:: @function entry point for the script
:MAIN
    if [%1]==[] (
        echo Please choose from supported options:
        call:-help
        goto:EOF
    )
    call :%~1
    goto:EOF
