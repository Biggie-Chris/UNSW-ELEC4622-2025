cd .\data
..\bin\project2_task2.exe bike_mono.bmp out1.bmp 256 0 0
start  mi_viewer bike_mono.bmp
start  mi_viewer out1.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit