# https://fhnw.mit-license.org/

import serial.tools.list_ports
import esptool
import time
import sys
import argparse

PORT = None
args = None
PARTITIONS = "partitions.bin"
SKETCH = "ESP32LoRaMqttPaxCounter.ino.feather_esp32.bin"
SKETCH_DEBUG = "ESP32LoRaMqttPaxCounter.ino.feather_esp32.debug.bin"

title = """
 ______  _____ _____ ____ ___    _    _       _           _   _______          _ 
|  ____|/ ____|  __ \___ \__ \  | |  | |     | |         | | |__   __|        | |
| |__  | (___ | |__) |__) | ) | | |  | |_ __ | | __ _  __| |    | | ___   ___ | |
|  __|  \___ \|  ___/|__ < / /  | |  | | '_ \| |/ _` |/ _` |    | |/ _ \ / _ \| |
| |____ ____) | |    ___) / /_  | |__| | |_) | | (_| | (_| |    | | (_) | (_) | |
|______|_____/|_|   |____/____|  \____/| .__/|_|\__,_|\__,_|    |_|\___/ \___/|_|
                                       | |                                       
                                       |_|                                       
"""

if len(sys.argv) > 1:
    parser = argparse.ArgumentParser(description="Uploads a sketch to the ESP32")
    parser.add_argument(
        "-d",
        "--debug",
        help="Uploads a debug version of the sketch",
        required=False,
        action="store_true",
    )
    parser.add_argument(
        "-s", "--sketch", help="The sketch to upload, -d is ignored", required=False
    )
    parser.add_argument(
        "-p", "--partitions", help="The partitions file to upload", required=False
    )
    parser.add_argument(
        "-P",
        "--port",
        default=None,
        help="The port to upload to, autodiscover by default",
        required=False,
    )
    args = parser.parse_args()


print(title)
print("Starting...")


def get_device_port():
    initports = serial.tools.list_ports.comports()
    ports = initports
    noports = len(initports)
    print("Plugin your device now...")
    while len(ports) <= noports:
        if len(ports) < noports:
            initports = ports
            noports = len(ports)
        ports = serial.tools.list_ports.comports()
        time.sleep(1)

    for port in initports:
        try:
            ports.remove(port)
        except:
            pass

    for port, desc, hwid in sorted(ports):
        if "USB" in hwid:
            print("found device on port", port, "with description", desc)
            return port


if args:
    if args.debug:
        SKETCH = SKETCH_DEBUG

    if args.sketch:
        SKETCH = args.sketch

    if args.partitions:
        PARTITIONS = args.partitions

    if args.port is None:
        PORT = get_device_port()
    else:
        PORT = args.port
else:
    PORT = get_device_port()

print("uploading ", SKETCH, " with Partitions", PARTITIONS, " to ", PORT)

write_flash_cmd = [
    "--chip",
    "esp32",
    "--port",
    PORT,
    "--baud",
    "921600",
    "--before",
    "default_reset",
    "--after",
    "hard_reset",
    "write_flash",
    "-z",
    "--flash_mode",
    "dio",
    "--flash_freq",
    "80m",
    "--flash_size",
    "detect",
    "0x10000",
    SKETCH,
    "0x8000",
    PARTITIONS,
]
esptool.main(write_flash_cmd)
print("Done!")
