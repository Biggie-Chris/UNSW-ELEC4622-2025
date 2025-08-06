cd .\data
..\bin\project3_task5.exe mb14.bmp mb15.bmp out5.bmp 20 20 500 1
start  mi_viewer mb14.bmp
start  mi_viewer mb15.bmp
start  mi_viewer out5.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit