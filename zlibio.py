#!/usr/bin/env python3
import sys
import zlib
import getopt

USAGE = '''Usage: zlibio.py [OPTION]...
zlib compress or decompress standard input to standard output.

    -d, --decompress  decode data
    -h, --help        display this help and exit
    -0, --zero        do not compress data; just add zlib header
    -1, --fast        compress faster
    -9, --best        compress better
'''

def log(str):
    sys.stderr.write('%s: %s\n' % (sys.argv[0], str))

def usage():
    sys.stderr.write(USAGE)

def main():
    flgCompress = True
    compressLvl = -1 # Default by zlib for automatic

    try:
        opts, args = getopt.getopt(sys.argv[1 : ], 'hd0123456789', ['help', 'decompress', 'zero', 'fast', 'best'])
    except getopt.GetoptError as e:
        log(e)
        return 1

    for arg in args:
        log('ignoring unknown argument "%s"' % arg)

    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage()
            return 1
        elif opt in ('-d', '--decompress'):
            flgCompress = False
        elif opt == '--zero':
            compressLvl = 0
        elif opt == '--fast':
            compressLvl = 1
        elif opt == '--best':
            compressLvl = 9
        elif opt in ('-%d' % x for x in range(10)):
            compressLvl = -int(opt)

    obj = zlib.compressobj(compressLvl) if flgCompress else zlib.decompressobj()
    data = sys.stdin.buffer.read(1024)
    while data:
        sys.stdout.buffer.write(obj.compress(data) if flgCompress else obj.decompress(data))
        data = sys.stdin.buffer.read(1024)
    sys.stdout.buffer.write(obj.flush())

    return 0

if __name__ == '__main__':
    try:
        exit(main())
    except KeyboardInterrupt:
        exit(130)

