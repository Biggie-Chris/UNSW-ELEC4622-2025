cd .\data
..\bin\project3_task5.exe f853.bmp f857.bmp out5.bmp 6 25 1200 0.6
start mi_viewer f853.bmp 
start mi_viewer f857.bmp 
start mi_viewer out5.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit