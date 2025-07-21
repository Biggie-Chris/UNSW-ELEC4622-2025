cd .\data
..\bin\project2_task1.exe bike_mono.bmp out.bmp 128 128 128
start  mi_viewer bike_mono.bmp
start  mi_viewer out.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit