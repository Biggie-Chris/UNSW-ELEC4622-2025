cd .\data
..\bin\project2_task2.exe pens_mono.bmp out2.bmp 256 128 128
start  mi_viewer pens_mono.bmp
start  mi_viewer out2.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit