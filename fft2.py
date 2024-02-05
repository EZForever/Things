#!/usr/bin/env python3

import numpy as np

from PIL import Image

signal = np.asarray(Image.open('res.png', 'r'))
fft = np.fft.fft2(signal)
fft = np.fft.fftshift(fft)
fftr, ffti = np.real(fft), np.imag(fft)

fftr = abs(fftr / 255).astype('uint8')
ffti = abs(ffti / 255).astype('uint8')

imgr, imgi = Image.fromarray(fftr), Image.fromarray(ffti)

#imgr.save('outr.png')
#imgi.save('outi.png')
imgr.save('out.gif',
        save_all = True,
        append_images = [imgi],
        duration = 10,
        unit = 'ms',
        loop = 0)

#fftc = np.log(np.absolute(fft)) * 12
#Image.fromarray(fftc.astype('uint8')).save('outc.png')

