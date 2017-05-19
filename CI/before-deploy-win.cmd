robocopy C:\projects\obs-studio\build32\rundir\RelWithDebInfo C:\projects\obs-studio\build\ /E /XF .gitignore
robocopy C:\projects\obs-studio\build64\rundir\RelWithDebInfo C:\projects\obs-studio\build\ /E /XC /XN /XO /XF .gitignore
7z a build.zip C:\projects\obs-studio\build\*