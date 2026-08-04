CC := arm-none-eabi-gcc
CFLAGS := -mcpu=cortex-m0plus -mthumb -ffreestanding -Os -ffunction-sections -fdata-sections
LDFLAGS := -T linker.ld -Wl,--gc-sections -nostdlib
KLIN ?= klin
.PHONY: all clean
all: adc.elf
LINK_OBJS := $(shell test -f out/adc.link && cat out/adc.link)
out/adc.c out/adc.link: adc.kl $(wildcard ../../machine_rp/*.kl)
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c adc.kl ;; \
	  *) "$(KLIN)" --emit-c adc.kl ;; \
	esac
adc.elf: linker.ld out/adc.c out/adc.link $(LINK_OBJS)
	$(CC) $(CFLAGS) -I. $(LDFLAGS) $$(cat out/adc.link) out/adc.c -lgcc -o $@
clean:
	rm -rf out adc.elf
