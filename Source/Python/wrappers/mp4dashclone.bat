@echo off
SET parent=%~dp0
python3 "%parent%..\utils\mp4-dash-clone.py" %*
