cd .\data

@echo Running DFT program...
..\bin\project2_task3.exe barbara.bmp out2.bmp 512 0 0 20


start  mi_viewer barbara.bmp
start  mi_viewer out2.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit