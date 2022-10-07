#!/bin/bash

g++ -o sim sim.cpp UtilityFunctions.o
../bin/mips-linux-gnu-as ../test/unit/$1/$1.asm -o ../test/unit/$1/$1.elf
../bin/mips-linux-gnu-objcopy ../test/unit/$1/$1.elf -j .text -O binary ../test/unit/$1/$1.bin
./sim ../test/unit/$1/$1.bin
