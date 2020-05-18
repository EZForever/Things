#!/usr/bin/env python3

import hashlib

# Test data (w/o the pre-loop next_f64()): seed = 0xc6462a4c, passwd = "FgVQyXZY2XFk"
class rand48:
    
    def __init__(self):
        self.__x = 0
    
    def set_seed(self, seed):
        self.__x = (seed << 16) | 0x330e
    
    def next_f64(self):
        self.__x = (self.__x * 0x5deece66d + 0xb) & 0xffffffffffff
        return float(self.__x) * (2 ** -48)

CHARS = b'abcdefghijkmpqrstuvwxyzABCDEFGHJKLMPQRSTUVWXYZ23456789'
SALT = bytes.fromhex('50617373776f7264436865636b48617368c41639792846e456')
HASH = bytes.fromhex('f29f55f07c043ad34b3de150501535f44424edad')

sha1_template = hashlib.sha1()
sha1_template.update(SALT)

buf = bytearray()
rand = rand48()

for n in range(256):
    pos = n << 24
    print('Checking block %d/256 (seeds %#x-%#x)' % (n + 1, pos, pos + (1 << 24)))
    for seed in range(pos, pos + (1 << 24)):
        buf.clear()
        rand.set_seed(seed)
        rand.next_f64()
        for _ in range(12):
            buf.append(CHARS[int(rand.next_f64() * 54)])
        # .decode('utf-16') will prepend UTF BOM
        passwd = buf.decode().encode('utf-16le')
        hash = sha1_template.copy()
        hash.update(passwd)
        if hash.digest() == HASH:
            print('FOUND: seed = %#x, passwd = "%s"' % (seed, buf.decode()))
            break
        del hash
        del passwd

