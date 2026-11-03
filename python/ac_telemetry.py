from utils_ble import *
from utils_ac import *


def main():
    _, view, static_handle, static_view = open_mappings()
    gamepad = open_gamepad()
    try:
        print("Sending RPM and max RPM")
        while True:
            data = read_mapping(view, Physics)
            static = read_mapping(static_view, StaticInfo)
            rpm = max(0, min(data.rpm, 65535))
            max_rpm = max(0, min(static.max_rpm, 65535))

            # hID writes include the report ID and the full report body.
            sendPayload(rpm, max_rpm, gamepad)
            time.sleep(UPDATE_INTERVAL)
    finally:
        gamepad.close()
        kernel32.UnmapViewOfFile(static_view)
        kernel32.CloseHandle(static_handle)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nStopped.")
