ECHO OFF
ECHO ESP32 Upload Tool
ECHO Installing esptool
pip install esptool
python upload_to_node.py
ECHO Upload done.
PAUSE