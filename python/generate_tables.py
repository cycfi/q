import math
import sys

_2pi = 2 * math.pi
_4pi = 4 * math.pi

def generate_sin_table(len):
   for i in range(len):
      yield math.sin(_2pi * i/len)

def generate_hamming_table(len):
   for i in range(len):
      yield 0.54 - 0.46 * math.cos(_2pi*i/len)

def generate_blackman_table(len):
   for i in range(len):
      yield 0.42 - 0.5 * math.cos(_2pi*i/len) + 0.08 * math.cos(_4pi*i/len)

def generate_crossfade_table(len):
   for i in range(len):
      yield math.sqrt(float(i)/(len-1));

use_float = True

if __name__ == "__main__":
   count = 0
   what = sys.argv[1]
   columns = 8
   if len(sys.argv) == 3:
      columns = int(sys.argv[2])

   func = None
   if what == "sin":
      func = generate_sin_table
   elif what == "hamming":
      func = generate_hamming_table
   elif what == "blackman":
         func = generate_blackman_table
   elif what == "crossfade":
         func = generate_crossfade_table

   if func:
      for val in func(1024):
         if use_float:
            print("{0:.15f},".format(val)),
         else:
            print "%d," % int(math.floor(val * 32767)),
         count += 1
         if count == columns:
            print
            count = 0
