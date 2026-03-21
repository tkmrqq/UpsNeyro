"""
shm_windows.py — Windows shared memory через ctypes MapViewOfFile
"""

import ctypes
import ctypes.wintypes as wt
import struct
import sys

kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)

# WinAPI функции
kernel32.OpenFileMappingW.restype  = wt.HANDLE
kernel32.OpenFileMappingW.argtypes = [wt.DWORD, wt.BOOL, wt.LPCWSTR]

kernel32.MapViewOfFile.restype  = ctypes.c_void_p
kernel32.MapViewOfFile.argtypes = [wt.HANDLE, wt.DWORD,
                                   wt.DWORD, wt.DWORD, ctypes.c_size_t]

kernel32.UnmapViewOfFile.restype  = wt.BOOL
kernel32.UnmapViewOfFile.argtypes = [ctypes.c_void_p]

kernel32.CloseHandle.restype  = wt.BOOL
kernel32.CloseHandle.argtypes = [wt.HANDLE]

FILE_MAP_ALL_ACCESS = 0xF001F


class WinSharedMemory:
    """Обёртка над Windows FileMapping для работы с QSharedMemory."""

    def __init__(self, key: str, size: int):
        self._handle = None
        self._view   = None
        self._size   = size
        self._buf    = None
        self._key    = key

    def open(self):
        self._handle = kernel32.OpenFileMappingW(
            FILE_MAP_ALL_ACCESS, False, self._key)
        if not self._handle:
            err = ctypes.get_last_error()
            raise RuntimeError(
                f"OpenFileMappingW failed for key='{self._key}', "
                f"error={err}")

        self._view = kernel32.MapViewOfFile(
            self._handle, FILE_MAP_ALL_ACCESS, 0, 0, self._size)
        if not self._view:
            err = ctypes.get_last_error()
            kernel32.CloseHandle(self._handle)
            raise RuntimeError(f"MapViewOfFile failed, error={err}")

        # Создаём ctypes буфер поверх mapped memory
        self._buf = (ctypes.c_uint8 * self._size).from_address(self._view)
        return self

    def close(self):
        if self._view:
            kernel32.UnmapViewOfFile(self._view)
            self._view = None
        if self._handle:
            kernel32.CloseHandle(self._handle)
            self._handle = None
        self._buf = None

    def read_byte(self, offset: int) -> int:
        return self._buf[offset]

    def write_byte(self, offset: int, value: int):
        self._buf[offset] = value

    def read_bytes(self, offset: int, length: int) -> bytes:
        return bytes(self._buf[offset:offset + length])

    def write_bytes(self, offset: int, data: bytes):
        for i, b in enumerate(data):
            self._buf[offset + i] = b

    def __enter__(self):
        return self.open()

    def __exit__(self, *_):
        self.close()