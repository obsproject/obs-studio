
robocopy C:\projects\obs-studio\build32\rundir\RelWithDebInfo C:\projects\obs-studio\build\ /E /XF .gitignore
robocopy C:\projects\obs-studio\build64\rundir\RelWithDebInfo C:\projects\obs-studio\build\ /E /XC /XN /XO /XF .gitignore
cd C:projects\obs-studio\build\
mkdir deps
robocopy C:\projects\obs-studio\deps C:projects\obs-studio\build\deps /E
cd C:\projects\obs-studio\build\deps
dir
7z a build.zip C:\projects\obs-studio\build\*
