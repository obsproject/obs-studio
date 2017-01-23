xcopy /e C:\projects\obs-studio\build32\rundir\RelWithDebInfo C:\projects\obs-studio\build\
robocopy C:\projects\obs-studio\build64\rundir\RelWithDebInfo C:\projects\obs-studio\build\ /E /XC /XN /XO
7z a build.zip C:\projects\obs-studio\build\*