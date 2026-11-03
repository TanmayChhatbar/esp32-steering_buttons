import ctypes
import time
from ctypes import wintypes


class Physics(ctypes.Structure):
    _pack_ = 4
    _fields_ = [
        ("packet_id", ctypes.c_int32),
        ("gas", ctypes.c_float),
        ("brake", ctypes.c_float),
        ("fuel", ctypes.c_float),
        ("gear", ctypes.c_int32),
        ("rpm", ctypes.c_int32),
    ]


kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
kernel32.OpenFileMappingW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
kernel32.OpenFileMappingW.restype = wintypes.HANDLE
kernel32.MapViewOfFile.argtypes = [
    wintypes.HANDLE,
    wintypes.DWORD,
    wintypes.DWORD,
    wintypes.DWORD,
    ctypes.c_size_t,
]
kernel32.MapViewOfFile.restype = ctypes.c_void_p
kernel32.UnmapViewOfFile.argtypes = [ctypes.c_void_p]
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
FILE_MAP_READ = 0x0004
MAPPING_NAME = "Local\\acpmf_physics"


def main():
    handle = kernel32.OpenFileMappingW(FILE_MAP_READ, False, MAPPING_NAME)
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())

    view = kernel32.MapViewOfFile(handle, FILE_MAP_READ, 0, 0, ctypes.sizeof(Physics))
    if not view:
        error = ctypes.get_last_error()
        kernel32.CloseHandle(handle)
        raise ctypes.WinError(error)

    try:
        while True:
            data = Physics.from_buffer_copy(
                ctypes.string_at(view, ctypes.sizeof(Physics))
            )
            print(f"\rRPM: {data.rpm}")
            time.sleep(1 / 30)  # approx 30 hz

    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        kernel32.UnmapViewOfFile(view)
        kernel32.CloseHandle(handle)


if __name__ == "__main__":
    main()
