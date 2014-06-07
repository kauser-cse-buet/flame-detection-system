#!/usr/bin/env python
# coding: utf-8
import numpy as np
import matplotlib.pyplot as plt


def plot(vals):
    l = len(vals)
    x = np.arange(0, l)
    y = vals
    z = map(abs, np.fft.fft(vals))
    f, (ax1, ax2) = plt.subplots(2, 1)
    # original
    ax1.plot(x, y, 'o-')
    ax1.grid()
    ax1.set_xlabel("n")
    ax1.set_ylabel("Area")
    ax1.set_title("Time Domain")
    ax1.set_xlim(0, np.max(x))
    ax1.xaxis.set_ticks([i * l / 8 for i in range(8)])
    # after fft
    z[0] = 0
    ax2.plot(x, z, 'o-')
    ax2.grid()
    ax2.set_xlabel("n")
    ax2.set_ylabel("Amplitude")
    ax2.set_title("Frequency Domain")
    ax2.set_xlim(0, np.max(x))
    ax2.xaxis.set_ticks([i * l / 8 for i in range(8)])
    # show result
    f.tight_layout()
    plt.show()
