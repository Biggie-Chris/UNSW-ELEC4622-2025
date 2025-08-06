cd .\data
..\bin\project3_task3.exe f1785.bmp f1789.bmp out3.bmp 10 10 20
start mi_viewer f1785.bmp 
start mi_viewer f1789.bmp 
start mi_viewer out3.bmp

@echo Press Enter to close all viewer windows...
@pause > nul

:: close all mi_viewer process
taskkill /IM mi_viewer.exe /F

:: close current terminal
exit