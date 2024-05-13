#!/usr/bin/env python3

crctbl = None

def crc32_bzip2(data: bytes) -> int:
	'''
		CRC-32/BZIP2
		Width : 32
		Poly  : 0x04c11db7
		Init  : 0xffffffff
		RefIn : false
		RefOut: false
		XorOut: 0xffffffff
	'''
	global crctbl
	if crctbl is None:
		# Validation: 0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, ...
		crctbl = []
		for i in range(256):
			x = i << 24
			for _ in range(8):
				if x & 0x80000000 == 0:
					x <<= 1
				else:
					x = (x << 1) ^ 0x04c11db7
			crctbl.append(x & 0xffffffff)
	
	value = 0xffffffff
	for x in data:
		value = (value << 8) ^ crctbl[(value >> 24) ^ x]
		value &= 0xffffffff 
	return value ^ 0xffffffff

# ---

alphabet = 'BCDFGHJKMPQRTVWXY2346789'
spc_pos = [5, 11, 17, 23]

def maskbits(x: int, bits: int) -> int:
	assert x >= 0
	assert bits >= 0
	return x & ((1 << bits) - 1)

def decode_key(key: str, new_key_only: bool = False) -> tuple[bool, int, int, int, int]:
	assert len(key) == 29
	assert all(key[i] == '-' for i in spc_pos)
	key = [x for x in key]
	for i in reversed(spc_pos):
		del key[i]
	
	new_key = False
	if 'N' in key:
		new_key = True
		n_idx = key.index('N')
		del key[n_idx]
		key.insert(0, alphabet[n_idx])
		assert 'N' not in key
	else:
		assert not new_key_only
	
	assert all(key[i] in alphabet for i in range(25))
	ki = 0
	for x in key:
		ki *= 24
		ki += alphabet.index(x)
	
	# Clear cksum and treat as a 128-bit little-endian integer
	vki = maskbits(ki, 103) | (ki & (1 << 113))
	vkd = vki.to_bytes(16, 'little')
	#print(vkd.hex(), hex(crc32_bzip2(vkd)))
	
	gid = maskbits(ki, 20)
	ki >>= 20
	kid = maskbits(ki, 30)
	ki >>= 30
	secret = maskbits(ki, 53)
	ki >>= 53
	cksum = maskbits(ki, 10)
	ki >>= 10
	is_upgrade = maskbits(ki, 1)
	ki >>= 1
	assert ki == 0
	assert cksum == maskbits(crc32_bzip2(vkd), 10)
	
	#print('%04x %010x' % (secret >> 40, maskbits(secret, 40)))
	return (new_key, gid, kid, secret, is_upgrade)

def encode_key(new_key: bool, gid: int, kid: int, secret: int, is_upgrade: int) -> str:
	assert gid == maskbits(gid, 20)
	assert kid == maskbits(kid, 30)
	assert secret == maskbits(secret, 53)
	assert is_upgrade == maskbits(is_upgrade, 1)
	
	vki = is_upgrade
	vki <<= 10
	# cksum here
	vki <<= 53
	vki |= secret
	vki <<= 30
	vki |= kid
	vki <<= 20
	vki |= gid
	
	vkd = vki.to_bytes(16, 'little')
	cksum = maskbits(crc32_bzip2(vkd), 10)
	ki = vki | (cksum << 103)
	
	key = []
	for _ in range(25):
		key.append(alphabet[ki % 24])
		ki //= 24
	assert ki == 0
	key = key[ : : -1]
	
	if new_key:
		n_idx = alphabet.index(key[0])
		del key[0]
		key.insert(n_idx, 'N')
	
	for i in spc_pos:
		key.insert(i, '-')
	
	return ''.join(key)

# ---

# Windows Server 2016-2022 KMS client keys
# https://learn.microsoft.com/en-us/windows-server/get-started/kms-client-activation-keys
keys = [
	'WX4NM-KYWYW-QJJR4-XV3QB-6VM33',
	'NTBV8-9K7Q8-V27C6-M2BTV-KHMXV',
	'VDYBN-27WPP-V4HQT-9VMD4-VMK7H',
	'WMDGN-G9PQG-XVVXX-R3X43-63DFG',
	'N69G4-B89J2-4G8F4-WWYCC-J464C',
	'WVDHN-86M7X-466P6-VHXV7-YY726',
	'CB7KF-BWN84-R7R2Y-793K2-8XDDG',
	'WC2BQ-8NRM3-FDDYY-2BFGV-KHKQY',
	'JCKRF-N37P4-C2D82-9YXRT-4M63B',
	'6NMRW-2C8FM-D24W7-TQWMY-CWH2D',
	'N2KJX-J94YW-TQVFB-DG9YT-724CC'
]

for key in keys:
	decoded = decode_key(key)
	print(decoded)
	print(key, encode_key(*decoded) == key)

