cd .\data

@echo Running FFT program...
..\bin\project2_task6.exe sanity.bmp out3.bmp 16 20


start  mi_viewer sanity.bmp
start  mi_viewer out3.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit