cd .\data
..\bin\project1_task1.exe pens_rgb.bmp out1.bmp
..\bin\project1_task3_n_task4.exe out1.bmp bi-linear_out.bmp 10 off

start  mi_viewer bi-linear_out.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit