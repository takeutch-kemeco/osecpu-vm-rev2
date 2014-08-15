# for Mac OSX

CC = cc
SRCS = api.c decode.c driver.c extend.c float.c integer.c osecpu-vm.c other.c pointer.c tek.c debug.c
BINNAME = osecpu.bin
DEL = rm
CFLAGS = -x objective-c -Wall -arch x86_64 -framework Cocoa

$(BINNAME): $(SRCS) Makefile
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	$(DEL) $(BINNAME)

%.ose : %.ask
	gcc -E -o a_0ask.txt -x c $<
	./osectols tool:aska in:a_0ask.txt out:a_1oas_$*.txt
	gcc -E -P -o a_2cas.txt -x c a_1oas_$*.txt
	./osectols tool:lbstk in:a_2cas.txt out:a_3cas.txt lst:a_3lbl.txt
	./osectols tool:db2bin in:a_3cas.txt out:$@
	./osectols tool:appack in:$@ out:$*_.ose

osectols: osectols.c
	gcc $< -o $@ -Wl,-s -Os -m32

