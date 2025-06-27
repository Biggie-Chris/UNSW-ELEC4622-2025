cd .\data
..\bin\project1_task2.exe barbara.bmp out.bmp 4
start  mi_viewer barbara.bmp
start  mi_viewer out.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit