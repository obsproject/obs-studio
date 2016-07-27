pushd ..\build\rundir\Debug\bin\64bit
start "nvenc m0" cmd /c ConsoleApp.exe -m 0 -e ffmpeg_nvenc -v 5000 -o e:\\libobs_vid\\teste_nvenc_0.mp4 -s e:\\libobs_vid\\teste_nvenc_0_2.mp4
start "nvenc m1" cmd /c ConsoleApp.exe -m 0 -e ffmpeg_nvenc -v 2500 -o e:\\libobs_vid\\teste_nvenc_1.mp4 -s e:\\libobs_vid\\teste_nvenc_1_2.mp4
start "x264 m2" cmd /c ConsoleApp.exe -m 0 -e obs_x264 -v 1200 -o e:\\libobs_vid\\teste_x264_2.mp4 -s e:\\libobs_vid\\teste_x264_2_2.mp4
start "x264 m3" cmd /c ConsoleApp.exe -m 3 -e obs_x264 -v 5000 -o e:\\libobs_vid\\teste_x264_3.mp4 -s e:\\libobs_vid\\teste_x264_3_2.mp4
