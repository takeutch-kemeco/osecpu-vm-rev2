# for Mac OSX

CC = cc
SRCS = api.c driver.c extend.c float.c integer.c osecpu-vm.c other.c pointer.c
BINNAME = osecpu-m32.bin
DEL = rm
CFLAGS = -x objective-c -Wall -arch x86_64 -framework Cocoa

$(BINNAME): $(SRCS) Makefile
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	$(DEL) $(BINNAME)
