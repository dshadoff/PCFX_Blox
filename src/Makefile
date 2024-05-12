PREFIX         = v810
LIBERIS        = $(HOME)/devel/liberis
V810GCC        = $(HOME)/devel/pcfx/bin/v810-gcc

ASFLAGS        = -a=$*.lst
# CFLAGS        += -I$(LIBERIS)/include/ -I$(V810GCC)/include/ -I$(V810GCC)/$(PREFIX)/include/ -O2 -Wall -std=gnu99 -mv810 -msda=256 -mprolog-function
CFLAGS        += -I$(LIBERIS)/include/ -I$(V810GCC)/include/ -I$(V810GCC)/$(PREFIX)/include/ -Wall -std=gnu99 -mv810 -msda=256 -mprolog-function
CPFLAGS       += -I$(LIBERIS)/include/ -I$(V810GCC)/include/ -I$(V810GCC)/$(PREFIX)/include/ -O2 -Wall -std=gnu++11 -fno-rtti -fno-exceptions -mv810 -msda=256 -mprolog-function
LDFLAGS       += -T$(LIBERIS)/ldscripts/v810.x -L$(LIBERIS)/ -L$(V810GCC)/lib/ -L$(V810GCC)/$(PREFIX)/lib/ -L$(V810GCC)/lib/gcc/$(PREFIX)/4.7.4/ $(V810GCC)/$(PREFIX)/lib/crt0.o

LIBS           = -leris -lc -lsim -lgcc

CHRDATA  = bkchr1_data.gen_data bkchr2_data.gen_data bottomchr_data.gen_data cornerchr_data.gen_data
CHRDATA += endchr_data.gen_data fullchr_data.gen_data offchr_data.gen_data

SPRDATA  = p0ph0_data.gen_data p0ph1_data.gen_data p0ph2_data.gen_data p0ph3_data.gen_data
SPRDATA += p1ph0_data.gen_data p1ph1_data.gen_data p1ph2_data.gen_data p1ph3_data.gen_data
SPRDATA += p2ph0_data.gen_data p2ph1_data.gen_data p2ph2_data.gen_data p2ph3_data.gen_data
SPRDATA += p3ph0_data.gen_data p3ph1_data.gen_data
SPRDATA += p4ph0_data.gen_data p4ph1_data.gen_data
SPRDATA += p5ph0_data.gen_data p5ph1_data.gen_data
SPRDATA += p6ph0_data.gen_data
SPRDATA += p7ph0_data.gen_data

blox.cue: cdlink_blox.txt blox
	pcfx-cdlink cdlink_blox.txt blox

blox.flashboot: blox
	python3 mkflashboot.py blox

blox: blox.o font.o
	v810-ld $(LDFLAGS) blox.o font.o $(LIBS) --sort-common=descending -o blox.linked -Map blox.map
	v810-objcopy -O binary blox.linked blox

font.o: font.s
	v810-as $(ASFLAGS) font.s -o font.o

blox.o: blox.source
	v810-as $(ASFLAGS) blox.source -o blox.o

%.o: %.s
	v810-as $(ASFLAGS) $< -o $@

blox.source: blox.c $(CHRDATA) $(SPRDATA)
	v810-gcc $(CFLAGS) blox.c -S -o blox.source

$(CHRDATA): bgdata.xlate bgdata.txt
	python3 cvtgfx.py tile offchr_data bgdata.txt 0 0 1 1 1 bgdata.xlate
	python3 cvtgfx.py tile bkchr1_data bgdata.txt 8 0 1 1 1 bgdata.xlate
	python3 cvtgfx.py tile bkchr2_data bgdata.txt 16 0 1 1 1 bgdata.xlate
	python3 cvtgfx.py tile cornerchr_data bgdata.txt 24 0 1 1 1 bgdata.xlate
	python3 cvtgfx.py tile endchr_data bgdata.txt 32 0 1 1 1 bgdata.xlate
	python3 cvtgfx.py tile bottomchr_data bgdata.txt 40 0 1 1 1 bgdata.xlate
	python3 cvtgfx.py tile fullchr_data bgdata.txt 48 0 1 1 1 bgdata.xlate

p0ph%_data.gen_data : bgdata.xlate spr0data.txt
	python3 cvtgfx.py sprite p0ph0_data spr0data.txt 0 0 2 2 2 bgdata.xlate
	python3 cvtgfx.py sprite p0ph1_data spr0data.txt 0 32 2 2 2 bgdata.xlate
	python3 cvtgfx.py sprite p0ph2_data spr0data.txt 0 64 2 2 2 bgdata.xlate
	python3 cvtgfx.py sprite p0ph3_data spr0data.txt 0 96 2 2 2 bgdata.xlate

p1ph%_data.gen_data : bgdata.xlate spr1data.txt
	python3 cvtgfx.py sprite p1ph0_data spr1data.txt 0 0 2 2 2 bgdata.xlate
	python3 cvtgfx.py sprite p1ph1_data spr1data.txt 0 32 2 2 2 bgdata.xlate
	python3 cvtgfx.py sprite p1ph2_data spr1data.txt 0 64 2 2 2 bgdata.xlate
	python3 cvtgfx.py sprite p1ph3_data spr1data.txt 0 96 2 2 2 bgdata.xlate

p2ph%_data.gen_data : bgdata.xlate spr2data.txt
	python3 cvtgfx.py sprite p2ph0_data spr2data.txt 0 0 2 2 2 bgdata.xlate
	python3 cvtgfx.py sprite p2ph1_data spr2data.txt 0 32 2 2 2 bgdata.xlate
	python3 cvtgfx.py sprite p2ph2_data spr2data.txt 0 64 2 2 2 bgdata.xlate
	python3 cvtgfx.py sprite p2ph3_data spr2data.txt 0 96 2 2 2 bgdata.xlate

p3ph%_data.gen_data : bgdata.xlate spr3data.txt
	python3 cvtgfx.py sprite p3ph0_data spr3data.txt 0 0 2 2 2 bgdata.xlate
	python3 cvtgfx.py sprite p3ph1_data spr3data.txt 0 32 2 2 2 bgdata.xlate

p4ph%_data.gen_data : bgdata.xlate spr4data.txt
	python3 cvtgfx.py sprite p4ph0_data spr4data.txt 0 0 2 2 2 bgdata.xlate
	python3 cvtgfx.py sprite p4ph1_data spr4data.txt 0 32 2 2 2 bgdata.xlate

p5ph%_data.gen_data : bgdata.xlate spr5data.txt
	python3 cvtgfx.py sprite p5ph0_data spr5data.txt 0 0 2 2 2 bgdata.xlate
	python3 cvtgfx.py sprite p5ph1_data spr5data.txt 0 32 2 2 2 bgdata.xlate

p6ph%_data.gen_data : bgdata.xlate spr6data.txt
	python3 cvtgfx.py sprite p6ph0_data spr6data.txt 0 0 2 2 2 bgdata.xlate

p7ph%_data.gen_data : bgdata.xlate spr7data.txt
	python3 cvtgfx.py sprite p7ph0_data spr7data.txt 0 0 2 2 2 bgdata.xlate

%.elf: $(OBJECTS)
	v810-ld $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@ -Map $*.map

%.bin: %.elf
	v810-objcopy -O binary $< $@

lbas.h:
	bincat out.bin lbas.h $(BIN_TARGET) $(ADD_FILES)

clean:
	rm -rf blox *.o *.source *.map *.lst *.linked lbas.h out.bin blox.bin blox.cue $(CHRDATA) $(SPRDATA)
