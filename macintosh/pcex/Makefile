# Standalone PCEX build (not wired into ESP-IDF by default).
# Requires: m68k-linux-gnu-gcc -m68000, m68k-linux-gnu-ld, m68k-linux-gnu-objcopy

CROSS ?= m68k-linux-gnu-
CC := $(CROSS)gcc
LD := $(CROSS)ld
OC := $(CROSS)objcopy

CFLAGS := -m68000 -nostdlib
TEXT := 0xf80000

all: pcex_illegal.rom pcex_mmio.rom

pcex_illegal.rom: pcex.o
	$(LD) --entry=0 -Ttext $(TEXT) -o pcex.elf pcex.o
	$(OC) --output-target binary pcex.elf $@
	@chmod 644 $@

pcex_mmio.rom: pcex_mmio.o
	$(LD) --entry=0 -Ttext $(TEXT) -o pcex_mmio.elf pcex_mmio.o
	$(OC) --output-target binary pcex_mmio.elf $@
	@chmod 644 $@

pcex.o: pcex.S
	$(CC) $(CFLAGS) -c -o $@ $<

pcex_mmio.o: pcex_mmio.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f pcex.o pcex.elf pcex_illegal.rom pcex_mmio.o pcex_mmio.elf pcex_mmio.rom

.PHONY: all clean
