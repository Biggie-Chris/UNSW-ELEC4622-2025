cd .\data
..\bin\project3_task2.exe mb14.bmp mb20.bmp out2.bmp 10 10 5 
start  mi_viewer mb14.bmp
start  mi_viewer mb20.bmp
start  mi_viewer out2.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit