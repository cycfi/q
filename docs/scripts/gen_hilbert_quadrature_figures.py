#!/usr/bin/env python3
"""
Generate the q::hilbert_quadrature reference figure.

Produces, in docs/modules/ROOT/images/:

   hilbert_quadrature.svg  -- two panels. Left: the phase difference between
                          the two output paths, holding close to 90 degrees
                          across the useful band around Nyquist/2 and drifting
                          only near DC and Nyquist. Right: a mid-band sinusoid
                          fed through the actual filter, with the two outputs
                          a quarter cycle apart (one rides as cosine, the other
                          as sine), the raw material of the analytic signal.

The coefficients and structure mirror hilbert_quadrature.hpp exactly.

Style and palette follow gen_differentiator_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_hilbert_quadrature_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # input
SKY = '#64b5f6'            # in-phase output
AMBER = '#ffb300'          # quadrature output
GREEN = '#43a047'          # phase difference
MAGENTA = '#d81b60'        # band markers

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

# Exactly as in hilbert_quadrature.hpp
CHAIN_A = [0.47940086558884, 0.87621849353931,
           0.976597589508199, 0.997499255935549]   # + delay1 (z^-1)
CHAIN_B = [0.161758498367701, 0.733028932341491,
           0.945349700329113, 0.990599156684529]


class PolyphaseAllpass:
   # r = a*(s + y2) - x2 ; shift x1->x2, s->x1 ; y1->y2, r->y1
   def __init__(self, a):
      self.a = a
      self.x1 = self.x2 = self.y1 = self.y2 = 0.0

   def __call__(self, s):
      r = self.a * (s + self.y2) - self.x2
      self.x2 = self.x1
      self.x1 = s
      self.y2 = self.y1
      self.y1 = r
      return r


class Delay1:
   def __init__(self):
      self.y = 0.0

   def __call__(self, s):
      r = self.y
      self.y = s
      return r


class Hilbert:
   def __init__(self):
      self._a = [PolyphaseAllpass(a) for a in CHAIN_A]
      self._dly = Delay1()
      self._w = [PolyphaseAllpass(a) for a in CHAIN_B]

   def __call__(self, s):
      u = s
      for f in self._a:
         u = f(u)
      out_i = self._dly(u)
      v = s
      for f in self._w:
         v = f(v)
      return out_i, v


def chain_response(coeffs, w, extra_delay=False):
   # product of H_i(z) = (a - z^-2)/(1 - a z^-2), times z^-1 if extra_delay
   z2 = np.exp(-2j * w)
   H = np.ones_like(w, dtype=complex)
   for a in coeffs:
      H *= (a - z2) / (1 - a * z2)
   if extra_delay:
      H *= np.exp(-1j * w)
   return H


def gen():
   sps = 44100.0
   fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4.2))

   # -- Left: phase difference across the band --
   f = np.linspace(1.0, sps / 2 - 1.0, 4000)
   w = 2 * np.pi * f / sps
   Ha = chain_response(CHAIN_A, w, extra_delay=True)
   Hb = chain_response(CHAIN_B, w, extra_delay=False)
   # Phase difference straight from the complex ratio: bounded to
   # (-180, 180], no unwrap (which misfires at the near-unit-circle poles
   # of the sharp sections). Q leads I by ~90 deg across the whole band.
   diff = np.degrees(np.angle(Hb * np.conj(Ha)))

   ax1.axhline(90, color='#b0b0b0', linewidth=0.9, linestyle=':')
   ax1.plot(f / 1000, diff, color=GREEN, linewidth=1.9,
            label='phase(Q) - phase(I)')
   ax1.set_title('Quadrature: within ~1 deg of 90 across the band',
                 color='#333333', fontsize=11)
   ax1.set_xlabel('Frequency (kHz)')
   ax1.set_ylabel('Phase difference (degrees)')
   ax1.set_xlim(0, sps / 2 / 1000)
   ax1.set_ylim(88.5, 91.5)
   ax1.set_yticks([89, 90, 91])
   ax1.legend(loc='lower center', fontsize=9)

   # -- Right: time domain, a mid-band tone through the real filter --
   hb = Hilbert()
   n = 200
   f0 = 4000.0
   t = np.arange(n + 400)
   x = np.sin(2 * np.pi * f0 * t / sps)
   oi = np.zeros_like(x)
   oq = np.zeros_like(x)
   for i, s in enumerate(x):
      oi[i], oq[i] = hb(s)
   # let the IIR settle, then show a clean window
   s0 = 380
   tt = np.arange(n)
   ax2.plot(tt, x[s0:s0 + n], color=SITE_ACCENT, linewidth=1.6,
            label='input', zorder=2)
   ax2.plot(tt, oi[s0:s0 + n], color=SKY, linewidth=1.8,
            label='in-phase (I)', zorder=3)
   ax2.plot(tt, oq[s0:s0 + n], color=AMBER, linewidth=1.8,
            label='quadrature (Q)', zorder=4)
   ax2.set_title('One tone in, two outputs a quarter cycle apart',
                 color='#333333', fontsize=11)
   ax2.set_xlabel('Time (samples)')
   ax2.set_xlim(0, n - 1)
   ax2.set_ylim(-1.35, 1.75)
   ax2.axhline(0, color='#b0b0b0', linewidth=0.8, zorder=1)
   ax2.legend(loc='upper center', fontsize=9, ncol=3,
              columnspacing=1.0, handlelength=1.4)

   for ax in (ax1, ax2):
      for s in ('top', 'right'):
         ax.spines[s].set_visible(False)
      ax.spines['left'].set_color('#b0b0b0')
      ax.spines['bottom'].set_color('#b0b0b0')
      ax.grid(True, linestyle='--', color='#b0b0b0', alpha=0.5)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'hilbert_quadrature.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
