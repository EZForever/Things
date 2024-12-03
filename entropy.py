#!/usr/bin/env python3
import sys
import math

def entropy_file(fp):
    freq = [0] * 256
    while blk := fp.read(4096):
        for b in blk:
            freq[b] += 1
    
    total = sum(freq)
    for i in range(256):
        if freq[i]:
            prob = freq[i] / total
            freq[i] = prob * math.log2(prob)
    
    entropy = -sum(freq)
    randomness = entropy / math.log2(256)
    return entropy, randomness

def main():
    files = sys.argv[1 : ]
    if len(files) == 0:
        files.append('-')
    
    for file in files:
        if file == '-':
            e, r = entropy_file(sys.stdin.buffer)
        else:
            with open(file, 'rb') as f:
                e, r = entropy_file(f)
        print('%s: e = %.4f, r = %.4f' % (file, e, r))

if __name__ == '__main__':
    main()

