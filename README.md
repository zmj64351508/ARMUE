#General Introduction 
This is an ARM emulator project. The name means **"ARM Utility Emulator"**. It emulates the full function of an ARM based CPU such as ARM cortex m3.

The function includes instruction set, memory map, rom, ram, peripherals, etc. 

#Current Process 
Completing the core function of the emulator mentioned above, especially the arm v7m instruction set and memory map.

###ARM v7m Instruction Set
Here is the 16 bit thumb instruction set

- LSL(imm)
- LSR(imm)
- ASR(imm)
- ADD(reg)
- SUB(reg)
- ADD(imm 3bit)
- SUB(imm 3bit)
- MOV(imm)
- MOV(reg)
- CMP(imm)
- ADD(imm 8bit)
- SUB(imm 8bit)

#Developing Enviroment 
The source is currently developed and tested under Windows 7 64 bit and Visual Stdio 2010  and will be ported to Linux with gcc in the future.