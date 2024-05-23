#!/usr/bin/env python3
import random
import itertools

# The algorithm for encrypting VBA project protection fields, with modifications
# See [MS-OVBA], section 2.4.3; also section 2.3.1.15 to 2.3.1.17

def ovba_enc(projid: str | None, seed: int, data: bytes) -> str:
    buf = bytearray()
    
    assert seed & 0xFF == seed
    buf.append(seed)
    
    v = 1 if projid is None else 2
    ve = seed ^ v
    buf.append(ve)
    
    pk = 0x47 if projid is None else sum(projid.encode('ascii')) & 0xFF
    pke = seed ^ pk
    buf.append(pke)
    
    ub1 = pk
    eb1 = pke
    eb2 = ve
    
    # One interesting thing to note: the "Ignore" field produced by VBA is always the seed byte repeating
    # This is probably an oversight somewhere in calculation since "proper" randomness is attempted by calling GetTickCount for each of the bytes
    # Spec says that it "could be any value". I guess that includes byte repeating. So well
    il = (seed & 6) // 2
    #i = [random.randint(0, 0xFF) for _ in range(il)]
    i = [seed] * il

    dl = len(data).to_bytes(4, 'little')
    for b in itertools.chain(i, dl, data):
        be = b ^ ((eb2 + ub1) & 0xFF)
        buf.append(be)
        
        eb2 = eb1
        eb1 = be
        ub1 = b
    
    return buf.hex().upper()

def ovba_dec(encrypted: str) -> (int, int, bytes):
    buf = list(bytes.fromhex(encrypted))
    
    seed = buf.pop(0)
    
    ve = buf.pop(0)
    v = seed ^ ve
    assert v in (1, 2)
    
    pke = buf.pop(0)
    pk = seed ^ pke

    ub1 = pk
    eb1 = pke
    eb2 = ve
    
    il = (seed & 6) // 2
    data = bytearray()
    for be in buf:
        b = be ^ ((eb2 + ub1) & 0xFF)
        data.append(b)
        
        eb2 = eb1
        eb1 = be
        ub1 = b
    
    data = data[il : ]
    dl, data = data[ : 4], data[4 : ]
    assert int.from_bytes(dl, 'little') == len(data)
    return pk, seed, bytes(data)

# ---

ID="{9E76F5A0-DAC8-4521-BCEF-644697F88708}"
CMG="1715EF1A11262C2A2C2A2C2A2C2A"

pk, seed, data = ovba_dec(CMG)
assert int.from_bytes(data, 'little') == 0

data = (0b100).to_bytes(4, 'little')
print(ovba_enc(ID, seed, data))

# ---

# VBA SDK license keys also use this algorithm (with v == 1, pk fixed to 0x47 == 71, hence the support in routines above)
# Data field: YYMMDDNNFFFFFFFFX, e.g. for eval key it is "990625017FFEF180\x00"
# - YYMMDD: ASCII, license issue date
# - NN: ASCII, license number of the date? (Only seen "01")
# - FFFFFFFF: Hex, feature flags? (Only seen "7FFEF180")
# - X: Raw byte, unknown (Seen \x00 and \x03)

# The algorithm is implemented in VBA as VBE7!Project::Encrypt and VBE7!Project::Unencrypt
# However no xrefs outside of reading/writing project protection fields is seen
# Thus there is currently no evidence that VBA performs any kind of validation on the license key

lickeys = [
    '16175148714896599659AFABD8ED3C2A416B45E4CD6F5484BD8CED', # Evaluation key
    # REDACTED
]
for key in lickeys:
    print(ovba_dec(key))

