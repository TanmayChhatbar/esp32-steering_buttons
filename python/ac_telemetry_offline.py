import time
from utils_ble import *


def main():
    gamepad = open_gamepad()
    try:
        print("Sending RPM and max RPM. Press Ctrl+C to stop.")
        while True:
            max_rpm = int(10000)
            rpm = int(
                min(max(0, -1000 + (time.time() * 2000) % (max_rpm + 2000)), max_rpm)
            )
            sendPayload(rpm, max_rpm, gamepad)
            time.sleep(UPDATE_INTERVAL)
    finally:
        gamepad.close()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nStopped.")
