@echo off
SET parent=%~dp0
python "%parent%..\utils\mp4-dash.py" %*
