cd .\data
..\bin\project3_task2.exe f853.bmp f857.bmp out2.bmp 10 10 20
start mi_viewer f853.bmp 
start mi_viewer f857.bmp 
start mi_viewer out2.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit