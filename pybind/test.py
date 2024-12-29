import fastpycrc64
import time

# print(fastpycrc64.get_string_crc64("123456789"))
start = time.time()
print(fastpycrc64.get_file_crc64("c:/Users/Defin/Downloads/cuda_12.1.1_531.14_windows.exe"))
print(time.time()-start)

import crcmod

start1 = time.time()
crc64_func = crcmod.mkCrcFun(0x142F0E1EBA9EA3693, initCrc=0, xorOut=0xffffffffffffffff, rev=True)

crc64 = 0
with open("c:/Users/Defin/Downloads/cuda_12.1.1_531.14_windows.exe", 'rb') as f:
        for chunk in iter(lambda: f.read(16384), b''):
            crc64 = crc64_func(chunk, crc64)

print(crc64)
print(time.time()-start1)