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

all: halt matmult matmult2 sort testfiles testlock testlockbad testlockname testcondition testcondition1 testcondition2 testconditionbad1 testconditionbad2 testexec testforkexec1 testforkexec2 testforkexecbad testfork run_passport_office passport_office testrand testprint helloworld fork_matmult exec_matmult testmonitor testmonitorbad ppo_appclerk ppo_picclerk ppo_ppclerk ppo_cashier ppo_manager ppo_customer ppo_exec_customers ppo_exec_cashiers ppo_exec_manager ppo_exec_picappclerks ppo_exec_ppclerks

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

matmult2.o: matmult2.c
	$(CC) $(CFLAGS) -c matmult2.c
matmult2: matmult2.o start.o
	$(LD) $(LDFLAGS) start.o matmult2.o -o matmult2.coff
	../bin/coff2noff matmult2.coff matmult2

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

testlockbad.o: testlockbad.c
	$(CC) $(CFLAGS) -c testlockbad.c
testlockbad: testlockbad.o start.o
	$(LD) $(LDFLAGS) start.o testlockbad.o -o testlockbad.coff
	../bin/coff2noff testlockbad.coff testlockbad	

testlockname.o: testlockname.c
	$(CC) $(CFLAGS) -c testlockname.c
testlockname: testlockname.o start.o
	$(LD) $(LDFLAGS) start.o testlockname.o -o testlockname.coff
	../bin/coff2noff testlockname.coff testlockname	

testcondition.o: testcondition.c
	$(CC) $(CFLAGS) -c testcondition.c
testcondition: testcondition.o start.o
	$(LD) $(LDFLAGS) start.o testcondition.o -o testcondition.coff
	../bin/coff2noff testcondition.coff testcondition

testcondition1.o: testcondition1.c
	$(CC) $(CFLAGS) -c testcondition1.c
testcondition1: testcondition1.o start.o
	$(LD) $(LDFLAGS) start.o testcondition1.o -o testcondition1.coff
	../bin/coff2noff testcondition1.coff testcondition1

testcondition2.o: testcondition2.c
	$(CC) $(CFLAGS) -c testcondition2.c
testcondition2: testcondition2.o start.o
	$(LD) $(LDFLAGS) start.o testcondition2.o -o testcondition2.coff
	../bin/coff2noff testcondition2.coff testcondition2

testconditionbad1.o: testconditionbad1.c
	$(CC) $(CFLAGS) -c testconditionbad1.c
testconditionbad1: testconditionbad1.o start.o
	$(LD) $(LDFLAGS) start.o testconditionbad1.o -o testconditionbad1.coff
	../bin/coff2noff testconditionbad1.coff testconditionbad1

testconditionbad2.o: testconditionbad2.c
	$(CC) $(CFLAGS) -c testconditionbad2.c
testconditionbad2: testconditionbad2.o start.o
	$(LD) $(LDFLAGS) start.o testconditionbad2.o -o testconditionbad2.coff
	../bin/coff2noff testconditionbad2.coff testconditionbad2	

testexec.o: testexec.c
	$(CC) $(CFLAGS) -c testexec.c
testexec: testexec.o start.o
	$(LD) $(LDFLAGS) start.o testexec.o -o testexec.coff
	../bin/coff2noff testexec.coff testexec

testforkexec1.o: testforkexec1.c
	$(CC) $(CFLAGS) -c testforkexec1.c
testforkexec1: testforkexec1.o start.o
	$(LD) $(LDFLAGS) start.o testforkexec1.o -o testforkexec1.coff
	../bin/coff2noff testforkexec1.coff testforkexec1

testforkexec2.o: testforkexec2.c
	$(CC) $(CFLAGS) -c testforkexec2.c
testforkexec2: testforkexec2.o start.o
	$(LD) $(LDFLAGS) start.o testforkexec2.o -o testforkexec2.coff
	../bin/coff2noff testforkexec2.coff testforkexec2

testforkexecbad.o: testforkexecbad.c
	$(CC) $(CFLAGS) -c testforkexecbad.c
testforkexecbad: testforkexecbad.o start.o
	$(LD) $(LDFLAGS) start.o testforkexecbad.o -o testforkexecbad.coff
	../bin/coff2noff testforkexecbad.coff testforkexecbad

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

run_passport_office.o: run_passport_office.c
	$(CC) $(CFLAGS) -c run_passport_office.c
run_passport_office: run_passport_office.o start.o
	$(LD) $(LDFLAGS) start.o run_passport_office.o -o run_passport_office.coff
	../bin/coff2noff run_passport_office.coff run_passport_office
	
fork_matmult.o: fork_matmult.c
	$(CC) $(CFLAGS) -c fork_matmult.c
fork_matmult: fork_matmult.o start.o
	$(LD) $(LDFLAGS) start.o fork_matmult.o -o fork_matmult.coff
	../bin/coff2noff fork_matmult.coff fork_matmult
	
exec_matmult.o: exec_matmult.c
	$(CC) $(CFLAGS) -c exec_matmult.c
exec_matmult: exec_matmult.o start.o
	$(LD) $(LDFLAGS) start.o exec_matmult.o -o exec_matmult.coff
	../bin/coff2noff exec_matmult.coff exec_matmult
	
testmonitor.o: testmonitor.c
	$(CC) $(CFLAGS) -c testmonitor.c
testmonitor: testmonitor.o start.o
	$(LD) $(LDFLAGS) start.o testmonitor.o -o testmonitor.coff
	../bin/coff2noff testmonitor.coff testmonitor
	
testmonitorbad.o: testmonitorbad.c
	$(CC) $(CFLAGS) -c testmonitorbad.c
testmonitorbad: testmonitorbad.o start.o
	$(LD) $(LDFLAGS) start.o testmonitorbad.o -o testmonitorbad.coff
	../bin/coff2noff testmonitorbad.coff testmonitorbad	

ppo_appclerk.o: ppo_appclerk.c
	$(CC) $(CFLAGS) -c ppo_appclerk.c
ppo_appclerk: ppo_appclerk.o start.o
	$(LD) $(LDFLAGS) start.o ppo_appclerk.o -o ppo_appclerk.coff
	../bin/coff2noff ppo_appclerk.coff ppo_appclerk	

ppo_picclerk.o: ppo_picclerk.c
	$(CC) $(CFLAGS) -c ppo_picclerk.c
ppo_picclerk: ppo_picclerk.o start.o
	$(LD) $(LDFLAGS) start.o ppo_picclerk.o -o ppo_picclerk.coff
	../bin/coff2noff ppo_picclerk.coff ppo_picclerk	

ppo_ppclerk.o: ppo_ppclerk.c
	$(CC) $(CFLAGS) -c ppo_ppclerk.c
ppo_ppclerk: ppo_ppclerk.o start.o
	$(LD) $(LDFLAGS) start.o ppo_ppclerk.o -o ppo_ppclerk.coff
	../bin/coff2noff ppo_ppclerk.coff ppo_ppclerk

ppo_manager.o: ppo_manager.c
	$(CC) $(CFLAGS) -c ppo_manager.c
ppo_manager: ppo_manager.o start.o
	$(LD) $(LDFLAGS) start.o ppo_manager.o -o ppo_manager.coff
	../bin/coff2noff ppo_manager.coff ppo_manager

ppo_cashier.o: ppo_cashier.c
	$(CC) $(CFLAGS) -c ppo_cashier.c
ppo_cashier: ppo_cashier.o start.o
	$(LD) $(LDFLAGS) start.o ppo_cashier.o -o ppo_cashier.coff
	../bin/coff2noff ppo_cashier.coff ppo_cashier	

ppo_customer.o: ppo_customer.c
	$(CC) $(CFLAGS) -c ppo_customer.c
ppo_customer: ppo_customer.o start.o
	$(LD) $(LDFLAGS) start.o ppo_customer.o -o ppo_customer.coff
	../bin/coff2noff ppo_customer.coff ppo_customer

ppo_exec_cashiers.o: ppo_exec_cashiers.c
	$(CC) $(CFLAGS) -c ppo_exec_cashiers.c
ppo_exec_cashiers: ppo_exec_cashiers.o start.o
	$(LD) $(LDFLAGS) start.o ppo_exec_cashiers.o -o ppo_exec_cashiers.coff
	../bin/coff2noff ppo_exec_cashiers.coff ppo_exec_cashiers

ppo_exec_customers.o: ppo_exec_customers.c
	$(CC) $(CFLAGS) -c ppo_exec_customers.c
ppo_exec_customers: ppo_exec_customers.o start.o
	$(LD) $(LDFLAGS) start.o ppo_exec_customers.o -o ppo_exec_customers.coff
	../bin/coff2noff ppo_exec_customers.coff ppo_exec_customers

ppo_exec_manager.o: ppo_exec_manager.c
	$(CC) $(CFLAGS) -c ppo_exec_manager.c
ppo_exec_manager: ppo_exec_manager.o start.o
	$(LD) $(LDFLAGS) start.o ppo_exec_manager.o -o ppo_exec_manager.coff
	../bin/coff2noff ppo_exec_manager.coff ppo_exec_manager

ppo_exec_picappclerks.o: ppo_exec_picappclerks.c
	$(CC) $(CFLAGS) -c ppo_exec_picappclerks.c
ppo_exec_picappclerks: ppo_exec_picappclerks.o start.o
	$(LD) $(LDFLAGS) start.o ppo_exec_picappclerks.o -o ppo_exec_picappclerks.coff
	../bin/coff2noff ppo_exec_picappclerks.coff ppo_exec_picappclerks

ppo_exec_ppclerks.o: ppo_exec_ppclerks.c
	$(CC) $(CFLAGS) -c ppo_exec_ppclerks.c
ppo_exec_ppclerks: ppo_exec_ppclerks.o start.o
	$(LD) $(LDFLAGS) start.o ppo_exec_ppclerks.o -o ppo_exec_ppclerks.coff
	../bin/coff2noff ppo_exec_ppclerks.coff ppo_exec_ppclerks

clean:
	rm -f *.o *.coff
