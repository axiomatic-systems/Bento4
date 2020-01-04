@echo off
SET parent=%~dp0
python3 "%parent%..\utils\mp4-hls.py" %*
