LIBOPENCM3 = ../libopencm3/lib/libopencm3_stm32f0.a

all: $(LIBOPENCM3) universaltx.dfu

universaltx.elf: universaltx.c
	arm-none-eabi-gcc --static -nostartfiles -Tuniversaltx.ld -Wl,-Map=universaltx.map -Wl,--gc-sections -DSTM32F0 -mthumb -mcpu=cortex-m0 -msoft-float -o universaltx.elf -I../libopencm3/include/ -L../libopencm3/lib/ -L../libopencm3/lib/stm32/f0/ universaltx.c -lopencm3_stm32f0 -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

universaltx.dfu: universaltx.elf
	arm-none-eabi-objcopy -O binary universaltx.elf universaltx.bin
	arm-none-eabi-objdump -d universaltx.elf > universaltx.list
	../../utils/dfu.py -b 0x08000000:universaltx.bin universaltx.dfu

$(LIBOPENCM3):
	make -C ../libopencm3 TARGETS=stm32/f0 lib
