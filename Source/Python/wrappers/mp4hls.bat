@ECHO OFF
SET python_command=py
WHERE /Q %python_command%
IF %ERRORLEVEL% EQU 0 (
  GOTO RUN
)
SET python_command=python
WHERE /Q %python_command%
IF %ERRORLEVEL% NEQ 0 (
  ECHO Python installation not found, please install Python 3
  EXIT /B 1
)

:RUN
SET parent=%~dp0
%python_command% "%parent%..\utils\mp4-hls.py" %*
