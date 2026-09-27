W ?= 1024
H ?= 768
FBE_ARGS ?=

CC = gcc
LD = ld
OBJCOPY = objcopy
NASM = nasm

CFLAGS = -std=c99 -m32 -Oz -mpreferred-stack-boundary=2 \
	-fno-ident -fmerge-all-constants -fno-align-functions -fno-align-loops \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-stack-protector \
	-fomit-frame-pointer -fdata-sections -ffunction-sections -fno-stack-check \
	-nostartfiles -nodefaultlibs -no-pie -fno-plt -fno-unroll-loops \
	-fno-tree-loop-im -fno-tree-ter -frename-registers \
	-DWIDTH=$(W) -DHEIGHT=$(H)

.NOTPARALLEL:
.PHONY: all release native emulator run-native run-emulator \
	../wilma-nativefb ../wilma-emulator ../framebuffer-emulator

all: release
	@:

release: native emulator
	@:

native: ../wilma-nativefb
	@:

emulator: ../wilma-emulator ../framebuffer-emulator
	@:

../wilma-nativefb: FBFLAG =
../wilma-emulator: FBFLAG = -DUSE_TMPFB
../wilma-nativefb ../wilma-emulator: demo.c syscall.h elf32.s script_32.ld
	@python3 -c "open('payload.bin','wb').write(b'\xde\xad\xbe\xef')"
	@$(NASM) -f bin -o demo_probe -Dwidth=$(W) -Dheight=$(H) $(FBFLAG) elf32.s
	@VADDR=$$(python3 -c "d=open('demo_probe','rb').read();print(hex(0x10000+d.index(b'\xde\xad\xbe\xef')))"); \
	$(CC) $(CFLAGS) -c -o demo.o demo.c; \
	$(LD) --gc-sections -m elf_i386 -e _start --defsym=PAYLOAD_VADDR=$$VADDR -T script_32.ld -o payload demo.o; \
	$(OBJCOPY) -j combined -O binary payload payload.bin; \
	$(NASM) -f bin -o "$@" -Dwidth=$(W) -Dheight=$(H) $(FBFLAG) elf32.s; \
	chmod +x "$@"; \
	rm -f demo_probe demo.o payload payload.bin

../framebuffer-emulator: ../fbe/fbe.c ../fbe/Makefile
	@$(MAKE) -s -C ../fbe W=$(W) H=$(H)
	@cp ../fbe/fbe "$@"
	@chmod +x "$@"

run-native: native
	@../wilma-nativefb

run-emulator: emulator
	@../framebuffer-emulator $(FBE_ARGS) --demo ../wilma-emulator $(W) $(H)
