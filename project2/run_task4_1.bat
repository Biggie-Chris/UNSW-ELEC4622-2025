cd .\data

@echo Running FFT program...
..\bin\project2_task4.exe bike_mono.bmp out1.bmp 256 100 100 20
start  mi_viewer bike_mono.bmp
start  mi_viewer out1.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit