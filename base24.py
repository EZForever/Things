CHARMAP = [bytes([x]) for x in b'BCDFGHJKMPQRTVWXY2346789']

def b24encode(data):
    enc = b''
    for c in data:
        enc += CHARMAP[c >> 4]
        enc += CHARMAP[-(c & 0x0f) - 1]
    return enc

def b24decode(data):
    dec = b''
    hi = -1
    for c in data:
        if hi < 0:
            hi = CHARMAP.index(bytes([c])) << 4
        else:
            dec += bytes([hi | (23 - CHARMAP.index(bytes([c])))])
            hi = -1
    return dec

