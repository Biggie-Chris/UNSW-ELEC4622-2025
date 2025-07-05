cd .\data
..\bin\project1_task6.exe barbara.bmp out.bmp 1 off

start  mi_viewer barbara.bmp
start  mi_viewer out.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit