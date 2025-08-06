cd .\data
..\bin\project3_task4.exe mb14.bmp out4.bmp 800 5
start  mi_viewer mb14.bmp
start  mi_viewer out4.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit