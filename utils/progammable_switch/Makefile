
all: switch.hex

switch.hex: switch.c
	avr-gcc -mmcu=attiny84a -Wall -Os -o switch.elf switch.c
	avr-objcopy -j .text -j .data -O ihex switch.elf switch.hex

flash:
	avrdude -p t84 -c usbasp -e -U flash:w:switch.hex

fuse:
	avrdude -p t84 -c usbasp -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
