# Needs: devkitARM + libctru + citro2d/citro3d (devkitPro), plus makerom and bannertool in PATH.
DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
export PATH := $(DEVKITPRO)/tools/bin:$(DEVKITARM)/bin:$(PATH)

TARGET  := forest_adventure
TITLE   := Whispering Woods
DESC    := A forest adventure
AUTHOR  := You

ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS  := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) -D__3DS__ -std=gnu11 \
           -I$(DEVKITPRO)/libctru/include -I$(DEVKITPRO)/portlibs/3ds/include
LDFLAGS := -specs=3dsx.specs -g $(ARCH) -L$(DEVKITPRO)/libctru/lib -L$(DEVKITPRO)/portlibs/3ds/lib
LIBS    := -lcitro2d -lcitro3d -lctru -lm

.PHONY: all clean
all: $(TARGET).cia

$(TARGET).elf: main.c
	arm-none-eabi-gcc $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@

$(TARGET).smdh: icon.png
	smdhtool --create "$(TITLE)" "$(DESC)" "$(AUTHOR)" icon.png $@

$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool $< $@ --smdh=$(TARGET).smdh

banner.bnr: banner.png audio.wav
	bannertool makebanner -i banner.png -a audio.wav -o $@

icon.icn: icon.png
	bannertool makesmdh -s "$(TITLE)" -l "$(DESC)" -p "$(AUTHOR)" -i icon.png -o $@

$(TARGET).cia: $(TARGET).elf banner.bnr icon.icn app.rsf $(TARGET).3dsx
	makerom -f cia -o $@ -elf $(TARGET).elf -rsf app.rsf -icon icon.icn -banner banner.bnr -exefslogo -target t

clean:
	rm -f $(TARGET).elf $(TARGET).3dsx $(TARGET).smdh $(TARGET).cia banner.bnr icon.icn $(TARGET).map
