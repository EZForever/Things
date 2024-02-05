#!/usr/bin/env python3

from itertools import chain

def tupper(k: int, width: int, height: int) -> list[list[int]]:
	def __fn(x: int, y: int) -> int:
		y += k
		n = 2 ** -(x * -17 - y % 17)
		n = y // 17 // n
		return int(n % 2 > 0.5)
	return [[__fn(x, y) for x in range(width)][ : : -1] for y in range(height)]

def tupper_inv(img: list[list[int]]) -> tuple[int, int, int]:
	w, h = len(img[0]), len(img)
	assert all(len(x) == w for x in img)
	
	k = int(''.join(map(str, chain(*zip(*img[ : : -1])))), 2) * 17
	return k, w, h

k = 960939379918958884971672962127852754715004339660129306651505519271702802395266424689642842174350718121267153782770623355993237280874144307891325963941337723487857735749823926629715517173716995165232890538221612403238855866184013235585136048828693337902491454229288667081096184496091705183454067827731551705405381627380967602565625016981482083418783163849115590225610003652351370343874461848378737238198224849863465033159410054974700593138339226497249461751545728366702369745461014655997933798537483143786841806593422227898388722980000748404719
w, h = 106, 17

img = tupper(k, w, h)
for line in img:
	for c in line:
		print('#' if c else '.', end = '')
	print()

k2, _, _ = tupper_inv(img)
assert k2 == k

