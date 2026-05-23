@echo off
SET parent=%~dp0
python "%parent%..\utils\hls-dash.py" %*
