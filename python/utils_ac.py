import time
import ctypes
import struct
from ctypes import wintypes

import hid

# config

VENDOR_ID = 0xE502
PRODUCT_ID = 0xBBAB
UPDATE_INTERVAL = 1 / 30

# defaults from the ESP32 BLE Gamepad library
REPORT_ID = 3
REPORT_LENGTH = 64


# assetto Corsa shared memory


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


class StaticInfo(ctypes.Structure):
    _pack_ = 4
    _fields_ = [
        ("sm_version", ctypes.c_uint16 * 15),
        ("ac_version", ctypes.c_uint16 * 15),
        ("number_of_sessions", ctypes.c_int32),
        ("num_cars", ctypes.c_int32),
        ("car_model", ctypes.c_uint16 * 33),
        ("track", ctypes.c_uint16 * 33),
        ("player_name", ctypes.c_uint16 * 33),
        ("player_surname", ctypes.c_uint16 * 33),
        ("player_nick", ctypes.c_uint16 * 33),
        ("sector_count", ctypes.c_int32),
        ("max_torque", ctypes.c_float),
        ("max_power", ctypes.c_float),
        ("max_rpm", ctypes.c_int32),
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
kernel32.UnmapViewOfFile.restype = wintypes.BOOL
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
kernel32.CloseHandle.restype = wintypes.BOOL
FILE_MAP_READ = 0x0004


def open_mapping(name, structure):
    handle = kernel32.OpenFileMappingW(FILE_MAP_READ, False, name)
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())

    view = kernel32.MapViewOfFile(handle, FILE_MAP_READ, 0, 0, ctypes.sizeof(structure))
    if not view:
        error = ctypes.get_last_error()
        kernel32.CloseHandle(handle)
        raise ctypes.WinError(error)

    return handle, view


def read_mapping(view, structure):
    raw = ctypes.string_at(view, ctypes.sizeof(structure))
    return structure.from_buffer_copy(raw)


def open_mappings():
    handle, view = open_mapping("Local\\acpmf_physics", Physics)
    static_handle, static_view = open_mapping("Local\\acpmf_static", StaticInfo)
    return handle, view, static_handle, static_view
