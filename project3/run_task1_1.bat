cd .\data
..\bin\project3_task1.exe mb14.bmp mb15.bmp 20 20 10
start  mi_viewer mb14.bmp
start  mi_viewer mb15.bmp


@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit