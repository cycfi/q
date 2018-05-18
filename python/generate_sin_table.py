import math

def generate_sin_table(len):
    for i in range(len):
        yield math.sin((2 * math.pi) * i/len)

use_float = True

if __name__ == "__main__":
    count = 0
    for val in generate_sin_table(1024):
        if use_float:
            print("{0:.15f},".format(val)),
        else:
            print "%d," % int(math.floor(val * 32767)),
        count += 1
        if count == 8:
            print
            count = 0
