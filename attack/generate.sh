#!/bin/bash
python touch.py 1
./hex2raw -i touch1.txt > touch1

gcc -c touch2.s 
objcopy touch2.o -O binary touch2
