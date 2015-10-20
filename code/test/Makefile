# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# If you are running on SunOS 5.x machines, uncomment the next line
GCCDIR = ../gnu/
LDFLAGS = -T newscript -N
ASFLAGS = -mips1
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)


CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld
CPP = $(GCCDIR)cpp

INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt matmult sort testfiles testlock testcondition testexec testfork passport_office testrand testprint helloworld

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

testfiles.o: testfiles.c
	$(CC) $(CFLAGS) -c testfiles.c
testfiles: testfiles.o start.o
	$(LD) $(LDFLAGS) start.o testfiles.o -o testfiles.coff
	../bin/coff2noff testfiles.coff testfiles
	
testlock.o: testlock.c
	$(CC) $(CFLAGS) -c testlock.c
testlock: testlock.o start.o
	$(LD) $(LDFLAGS) start.o testlock.o -o testlock.coff
	../bin/coff2noff testlock.coff testlock	

testcondition.o: testcondition.c
	$(CC) $(CFLAGS) -c testcondition.c
testcondition: testcondition.o start.o
	$(LD) $(LDFLAGS) start.o testcondition.o -o testcondition.coff
	../bin/coff2noff testcondition.coff testcondition

testexec.o: testexec.c
	$(CC) $(CFLAGS) -c testexec.c
testexec: testexec.o start.o
	$(LD) $(LDFLAGS) start.o testexec.o -o testexec.coff
	../bin/coff2noff testexec.coff testexec

testmultiexec.o: testmultiexec.c
	$(CC) $(CFLAGS) -c testmultiexec.c
testmultiexec: testmultiexec.o start.o
	$(LD) $(LDFLAGS) start.o testmultiexec.o -o testmultiexec.coff
	../bin/coff2noff testmultiexec.coff testmultiexec
	
testfork.o: testfork.c
	$(CC) $(CFLAGS) -c testfork.c
testfork: testfork.o start.o
	$(LD) $(LDFLAGS) start.o testfork.o -o testfork.coff
	../bin/coff2noff testfork.coff testfork

testrand.o: testrand.c
	$(CC) $(CFLAGS) -c testrand.c
testrand: testrand.o start.o
	$(LD) $(LDFLAGS) start.o testrand.o -o testrand.coff
	../bin/coff2noff testrand.coff testrand

testprint.o: testprint.c
	$(CC) $(CFLAGS) -c testprint.c
testprint: testprint.o start.o
	$(LD) $(LDFLAGS) start.o testprint.o -o testprint.coff
	../bin/coff2noff testprint.coff testprint
	
helloworld.o: helloworld.c
	$(CC) $(CFLAGS) -c helloworld.c
helloworld: helloworld.o start.o
	$(LD) $(LDFLAGS) start.o helloworld.o -o helloworld.coff
	../bin/coff2noff helloworld.coff helloworld	
	
passport_office.o: passport_office.c
	$(CC) $(CFLAGS) -c passport_office.c
passport_office: passport_office.o start.o
	$(LD) $(LDFLAGS) start.o passport_office.o -o passport_office.coff
	../bin/coff2noff passport_office.coff passport_office		

clean:
	rm -f *.o *.coff
