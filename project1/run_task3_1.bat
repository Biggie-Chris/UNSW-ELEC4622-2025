cd .\data
..\bin\project1_task3_n_task4.exe pens_rgb.bmp out.bmp 25 on
start  mi_viewer pens_rgb.bmp
start  mi_viewer out.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit