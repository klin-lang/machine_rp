CC := arm-none-eabi-gcc
CFLAGS := -mcpu=cortex-m0plus -mthumb -ffreestanding -Os -ffunction-sections -fdata-sections
LDFLAGS := -T linker.ld -Wl,--gc-sections -nostdlib
KLIN ?= klin
.PHONY: all clean
all: i2c.elf
LINK_OBJS := $(shell test -f out/i2c.link && cat out/i2c.link)
out/i2c.c out/i2c.link: i2c.kl $(wildcard ../../machine_rp/*.kl)
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c i2c.kl ;; \
	  *) "$(KLIN)" --emit-c i2c.kl ;; \
	esac
i2c.elf: linker.ld out/i2c.c out/i2c.link $(LINK_OBJS)
	$(CC) $(CFLAGS) -I. $(LDFLAGS) $$(cat out/i2c.link) out/i2c.c -lgcc -o $@
clean:
	rm -rf out i2c.elf
