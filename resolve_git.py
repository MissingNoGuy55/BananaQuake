import subprocess
import random

conflicting = True  # Assume conflicting
output = None
err = None
choices = ["Y", "N"]
process = None

while conflicting:
    process = subprocess.run(["git", "checkout", "*"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, shell=False)

    output = process.stdout
    err = process.stderr

    print(output)
    print(err)
    
    barray = bytearray(output, encoding='utf-8')

    if (barray[8] == b'0'): # check the literal output of git -- there is no way to get the return value of checkout
        conflicting = False
        break
