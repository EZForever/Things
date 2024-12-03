#!/usr/bin/env python3
import sys
import math

'''
    Entropy does not consider the order of bytes.
    python3 -c 'with open("seq256.bin", "wb") as f: f.write(bytes(range(256)))'
    The file 'seq256.bin' will have a entropy of 8.0 (i.e. randomness == 1.0)
'''

def entropy_file(fp):
    freq = [0] * 256
    b = fp.read(1)
    while b:
        freq[b[0]] += 1
        b = fp.read(1)
    total = sum(freq)
    for i in range(256):
        if freq[i]:
            freqFrac = freq[i] / total
            freq[i] = freqFrac * math.log2(freqFrac)
    # entropy == How many bits of information I can get from 1 byte of data (on average)
    entropy = -sum(freq)
    # randomness == How random a bit in data is (on average)
    randomness = entropy / 8 # math.log2(256)
    return entropy, randomness

def main():
    for file_name in sys.argv[1 : ]:
        with open(file_name, 'rb') as f:
            e, r = entropy_file(f)
        print('%s: e = %.4f, r = %.4f' % (file_name, e, r))

if __name__ == '__main__':
    main()

