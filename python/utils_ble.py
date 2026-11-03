import hid
import struct

# config
VENDOR_ID = 0xE502
PRODUCT_ID = 0xBBAB
UPDATE_INTERVAL = 1 / 30

# defaults from the ESP32 BLE Gamepad library
REPORT_ID = 3
REPORT_LENGTH = 64


def sendPayload(rpm, max_rpm, gamepad):
    payload = bytes([REPORT_ID]) + struct.pack(">HH", rpm, max_rpm)
    payload = payload.ljust(REPORT_LENGTH + 1, b"\x00")
    written = gamepad.write(payload)
    if written != len(payload):
        raise RuntimeError(f"Incomplete HID write: {written} bytes")
    print(f"\rRPM sent: {rpm:5d} / {max_rpm:5d}", end="", flush=True)


def open_gamepad():
    matches = [
        device
        for device in hid.enumerate(VENDOR_ID, PRODUCT_ID)
        if device["usage_page"] == 1 and device["usage"] == 5
    ]
    if len(matches) != 1:
        raise RuntimeError("check HID")
    device = hid.device()
    device.open_path(matches[0]["path"])
    return device
