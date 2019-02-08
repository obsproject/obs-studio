
robocopy C:\projects\obs-studio\build32\rundir\RelWithDebInfo C:\projects\obs-studio\build\ /E /XF .gitignore
robocopy C:\projects\obs-studio\build64\rundir\RelWithDebInfo C:\projects\obs-studio\build\ /E /XC /XN /XO /XF .gitignore
mkdir C:\projects\obs-studio\build\deps\w32-pthreads
robocopy C:\projects\obs-studio\deps\w32-pthreads C:\projects\obs-studio\build\deps\w32-pthreads pthread.h
7z a build.zip C:\projects\obs-studio\build\*
