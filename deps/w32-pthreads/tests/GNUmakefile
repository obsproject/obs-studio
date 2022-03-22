# Makefile for the pthreads test suite.
# If all of the .pass files can be created, the test suite has passed.
#
# --------------------------------------------------------------------------
#
#      Pthreads-win32 - POSIX Threads Library for Win32
#      Copyright(C) 1998 John E. Bossom
#      Copyright(C) 1999,2005 Pthreads-win32 contributors
# 
#      Contact Email: rpj@callisto.canberra.edu.au
# 
#      The current list of contributors is contained
#      in the file CONTRIBUTORS included with the source
#      code distribution. The list can also be seen at the
#      following World Wide Web location:
#      http://sources.redhat.com/pthreads-win32/contributors.html
# 
#      This library is free software; you can redistribute it and/or
#      modify it under the terms of the GNU Lesser General Public
#      License as published by the Free Software Foundation; either
#      version 2 of the License, or (at your option) any later version.
# 
#      This library is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#      Lesser General Public License for more details.
# 
#      You should have received a copy of the GNU Lesser General Public
#      License along with this library in the file COPYING.LIB;
#      if not, write to the Free Software Foundation, Inc.,
#      51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
#

DLL_VER	= 2

CP	= cp -f
MV	= mv -f
RM	= rm -f
CAT	= cat
MKDIR	= mkdir
TOUCH	= echo Passed >
ECHO	= @echo
MAKE	= make -k

# For cross compiling use e.g.
# # make CROSS=i386-mingw32msvc- clean GC
CROSS   =

# For cross testing use e.g.
# # make RUN=wine CROSS=i386-mingw32msvc- clean GC
RUN     =

AR      = $(CROSS)ar
DLLTOOL = $(CROSS)dlltool
CC      = $(CROSS)gcc
CXX     = $(CROSS)g++
RANLIB  = $(CROSS)ranlib

#
# Mingw32
#
XXCFLAGS	= 
XXLIBS	= -lws2_32 -lgomp
OPT	= -O3
DOPT	= -g -O0
#CFLAGS	= -O3 -UNDEBUG -Wall $(XXCFLAGS)
CFLAGS	= ${OPT} -UNDEBUG -Wall $(XXCFLAGS)
BUILD_DIR	= ..
INCLUDES	= -I.

.INTERMEDIATE: %.exe %.pass
.SECONDARY: %.exe %.pass
.PRECIOUS: %.exe %.pass

TEST	= GC

# Default lib version
GCX	= $(TEST)$(DLL_VER)

# Files we need to run the tests
# - paths are relative to pthreads build dir.
HDR	= pthread.h semaphore.h sched.h
LIB	= libpthread$(GCX).a
DLL	= pthread$(GCX).dll
# The next path is relative to $BUILD_DIR
QAPC	=  # ../QueueUserAPCEx/User/quserex.dll

COPYFILES	= $(HDR) $(LIB) $(DLL) $(QAPC)

# If a test case returns a non-zero exit code to the shell, make will
# stop.

TESTS	= \
	  sizes loadfree \
	  self1 mutex5 mutex1 mutex1e mutex1n mutex1r \
	  semaphore1 semaphore2 semaphore3 \
	  condvar1 condvar1_1 condvar1_2 condvar2 condvar2_1 exit1 \
	  create1 create2 create3 reuse1 reuse2 equal1 \
	  sequence1 kill1 valid1 valid2 \
	  exit2 exit3 exit4 exit5 \
	  join0 join1 detach1 join2 join3 \
	  mutex2 mutex2r mutex2e mutex3 mutex3r mutex3e \
	  mutex4 mutex6 mutex6n mutex6e mutex6r \
	  mutex6s mutex6es mutex6rs \
	  mutex7 mutex7n mutex7e mutex7r mutex8 mutex8n mutex8e mutex8r \
	  robust1 robust2 robust3 robust4 robust5 \
	  count1 \
	  once1 once2 once3 once4 self2 \
	  cancel1 cancel2 \
	  semaphore4 semaphore4t semaphore5 \
	  barrier1 barrier2 barrier3 barrier4 barrier5 barrier6 \
	  tsd1 tsd2 openmp1 delay1 delay2 eyal1 \
	  condvar3 condvar3_1 condvar3_2 condvar3_3 \
	  condvar4 condvar5 condvar6 condvar7 condvar8 condvar9 \
	  errno1 \
	  rwlock1 rwlock2 rwlock3 rwlock4 rwlock5 rwlock6 rwlock7 rwlock8 \
	  rwlock2_t rwlock3_t rwlock4_t rwlock5_t rwlock6_t rwlock6_t2 \
	  context1 cancel3 cancel4 cancel5 cancel6a cancel6d \
	  cancel7 cancel8 \
	  cleanup0 cleanup1 cleanup2 cleanup3 \
	  priority1 priority2 inherit1 \
	  spin1 spin2 spin3 spin4 \
	  exception1 exception2 exception3 \
	  cancel9 stress1

STRESSTESTS = \
	stress1

BENCHTESTS = \
	benchtest1 benchtest2 benchtest3 benchtest4 benchtest5

STATICTESTS = \
	  sizes \
	  self1 mutex5 mutex1 mutex1e mutex1n mutex1r \
	  semaphore1 semaphore2 semaphore3 \
	  condvar1 condvar1_1 condvar1_2 condvar2 condvar2_1 exit1 \
	  create1 create2 create3 reuse1 reuse2 equal1 \
	  sequence1 kill1 valid1 valid2 \
	  exit2 exit3 exit4 exit5 \
	  join0 join1 detach1 join2 join3 \
	  mutex2 mutex2r mutex2e mutex3 mutex3r mutex3e \
	  mutex4 mutex6 mutex6n mutex6e mutex6r \
	  mutex6s mutex6es mutex6rs \
	  mutex7 mutex7n mutex7e mutex7r mutex8 mutex8n mutex8e mutex8r \
	  robust1 robust2 robust3 robust4 robust5 \
	  count1 \
	  once1 once2 once3 once4 self2 \
	  cancel1 cancel2 \
	  semaphore4 semaphore4t semaphore5 \
	  barrier1 barrier2 barrier3 barrier4 barrier5 barrier6 \
	  tsd1 tsd2 delay1 delay2 eyal1 \
	  condvar3 condvar3_1 condvar3_2 condvar3_3 \
	  condvar4 condvar5 condvar6 condvar7 condvar8 condvar9 \
	  errno1 \
	  rwlock1 rwlock2 rwlock3 rwlock4 rwlock5 rwlock6 rwlock7 rwlock8 \
	  rwlock2_t rwlock3_t rwlock4_t rwlock5_t rwlock6_t rwlock6_t2 \
	  context1 cancel3 cancel4 cancel5 cancel6a cancel6d \
	  cancel7 cancel8 \
	  cleanup0 cleanup1 cleanup2 cleanup3 \
	  priority1 priority2 inherit1 \
	  spin1 spin2 spin3 spin4 \
	  exception1 exception2 exception3 \
	  cancel9 stress1

ALLTESTS = $(TESTS) $(BENCHTESTS)

ASM		= $(ALLTESTS:%=%.s)
PASSES		= $(TESTS:%=%.pass)
BENCHRESULTS	= $(BENCHTESTS:%=%.bench)
STRESSRESULTS	= $(STRESSTESTS:%=%.pass)
STATICRESULTS	= $(STATICTESTS:%=%.pass)

help:
	@ $(ECHO) "Run one of the following command lines:"
	@ $(ECHO) "make clean GC    (to test using GC dll with C (no EH) applications)"
	@ $(ECHO) "make clean GCX   (to test using GC dll with C++ (EH) applications)"
	@ $(ECHO) "make clean GCE   (to test using GCE dll with C++ (EH) applications)"
	@ $(ECHO) "make clean GC-bench	  (to benchtest using GNU C dll with C cleanup code)"
	@ $(ECHO) "make clean GCE-bench   (to benchtest using GNU C dll with C++ exception handling)"
	@ $(ECHO) "make clean GC-stress	  (to stresstest using GNU C dll with C cleanup code)"
	@ $(ECHO) "make clean GCE-stress   (to stresstest using GNU C dll with C++ exception handling)"
	@ $(ECHO) "make clean GC-static   (to test using GC static lib with C (no EH) applications)"
	@ $(ECHO) "make clean GC-debug    (to test using GC dll with C (no EH) applications)"

all:
	@ $(MAKE) clean GC
	@ $(MAKE) clean GCX
	@ $(MAKE) clean GCE

GC:
	$(MAKE) TEST=GC CC=$(CC) XXCFLAGS="-fopenmp -D__CLEANUP_C" all-pass

GC-asm:
	$(MAKE) TEST=GC CC=$(CC) XXCFLAGS="-fopenmp -D__CLEANUP_C" all-asm

GCE:
	$(MAKE) TEST=GCE CC=$(CXX) XXCFLAGS="-fopenmp -mthreads -D__CLEANUP_CXX" all-pass

GCX:
	$(MAKE) TEST=GC CC=$(CXX) XXCFLAGS="-fopenmp -mthreads -D__CLEANUP_C" all-pass

GC-bench:
	$(MAKE) TEST=GC CC=$(CC) XXCFLAGS="-D__CLEANUP_C" XXLIBS="benchlib.o" all-bench

GCE-bench:
	$(MAKE) TEST=GCE  CC=$(CXX) XXCFLAGS="-mthreads -D__CLEANUP_CXX" XXLIBS="benchlib." all-bench

GC-debug:
	$(MAKE) TEST=GC CC=$(CC) XXCFLAGS="-fopenmp -D__CLEANUP_C" OPT="${DOPT}" DLL_VER="$(DLL_VER)d" all-pass

GCE-debug:
	$(MAKE) TEST=GCE CC=$(CXX) XXCFLAGS="-fopenmp -D__CLEANUP_CXX" OPT="${DOPT}" DLL_VER="$(DLL_VER)d" all-pass

GC-bench-debug:
	$(MAKE) TEST=GC CC=$(CC) XXCFLAGS="-D__CLEANUP_C" XXLIBS="benchlib.o" OPT="${OPT}" DLL_VER="$(DLL_VER)d" all-bench

GC-static:
	$(MAKE) TEST=GC CC=$(CC) XXCFLAGS="-D__CLEANUP_C -DPTW32_STATIC_LIB" XXLIBS="-lws2_32" DLL="" all-static

GC-stress:
	$(ECHO) Stress tests can take a long time since they are trying to
	$(ECHO) expose weaknesses that may be intermittant or statistically rare.
	$(ECHO) A pass does not prove correctness, but may give greater confidence.
	$(MAKE) TEST=GC CC=$(CC) XXCFLAGS="-D__CLEANUP_C" XXLIBS="" all-stress

GCE-stress:
	$(MAKE) TEST=GCE  CC=$(CXX) XXCFLAGS="-mthreads -D__CLEANUP_CXX" XXLIBS="" all-stress

all-asm: $(ASM)
	@ $(ECHO) ALL TESTS PASSED! Congratulations!

all-pass: $(PASSES)
	@ $(ECHO) ALL TESTS PASSED! Congratulations!

all-bench: $(BENCHRESULTS)
	@ $(ECHO) BENCH TESTS COMPLETED.

all-stress: $(STRESSRESULTS)
	@ $(ECHO) STRESS TESTS COMPLETED.

all-static: $(STATICRESULTS)
	@ $(ECHO) ALL STATIC TESTS PASSED! Congratulations!

benchtest1.bench:
benchtest2.bench:
benchtest3.bench:
benchtest4.bench:
benchtest5.bench:

barrier1.pass: semaphore4.pass
barrier2.pass: barrier1.pass
barrier3.pass: barrier2.pass
barrier4.pass: barrier3.pass
barrier5.pass: barrier4.pass
barrier6.pass: barrier5.pass
cancel1.pass: create1.pass
cancel2.pass: cancel1.pass
cancel3.pass: context1.pass
cancel4.pass: cancel3.pass
cancel5.pass: cancel3.pass
cancel6a.pass: cancel3.pass
cancel6d.pass: cancel3.pass
cancel7.pass: kill1.pass
cancel8.pass: cancel7.pass
cancel9.pass: cancel8.pass
cleanup0.pass: cancel5.pass
cleanup1.pass: cleanup0.pass
cleanup2.pass: cleanup1.pass
cleanup3.pass: cleanup2.pass
condvar1.pass:
condvar1_1.pass: condvar1.pass
condvar1_2.pass: join2.pass
condvar2.pass: condvar1.pass
condvar2_1.pass: condvar2.pass join2.pass
condvar3.pass: create1.pass condvar2.pass
condvar3_1.pass: condvar3.pass join2.pass
condvar3_2.pass: condvar3_1.pass
condvar3_3.pass: condvar3_2.pass
condvar4.pass: create1.pass
condvar5.pass: condvar4.pass
condvar6.pass: condvar5.pass
condvar7.pass: condvar6.pass cleanup1.pass
condvar8.pass: condvar7.pass
condvar9.pass: condvar8.pass
context1.pass: cancel1.pass
count1.pass: join1.pass
create1.pass: mutex2.pass
create2.pass: create1.pass
create3.pass: create2.pass
delay1.pass:
delay2.pass: delay1.pass
detach1.pass: join0.pass
equal1.pass: create1.pass
errno1.pass: mutex3.pass
exception1.pass: cancel4.pass
exception2.pass: exception1.pass
exception3.pass: exception2.pass
exit1.pass:
exit2.pass: create1.pass
exit3.pass: create1.pass
exit4.pass:
exit5.pass: exit4.pass kill1.pass
eyal1.pass: tsd1.pass
inherit1.pass: join1.pass priority1.pass
join0.pass: create1.pass
join1.pass: create1.pass
join2.pass: create1.pass
join3.pass: join2.pass
kill1.pass:
loadfree.pass: pthread.dll
mutex1.pass: self1.pass
mutex1n.pass: mutex1.pass
mutex1e.pass: mutex1.pass
mutex1r.pass: mutex1.pass
mutex2.pass: mutex1.pass
mutex2r.pass: mutex2.pass
mutex2e.pass: mutex2.pass
mutex3.pass: create1.pass
mutex3r.pass: mutex3.pass
mutex3e.pass: mutex3.pass
mutex4.pass: mutex3.pass
mutex5.pass:
mutex6.pass: mutex4.pass
mutex6n.pass: mutex4.pass
mutex6e.pass: mutex4.pass
mutex6r.pass: mutex4.pass
mutex6s.pass: mutex6.pass
mutex6rs.pass: mutex6r.pass
mutex6es.pass: mutex6e.pass
mutex7.pass: mutex6.pass
mutex7n.pass: mutex6n.pass
mutex7e.pass: mutex6e.pass
mutex7r.pass: mutex6r.pass
mutex8.pass: mutex7.pass
mutex8n.pass: mutex7n.pass
mutex8e.pass: mutex7e.pass
mutex8r.pass: mutex7r.pass
once1.pass: create1.pass
once2.pass: once1.pass
once3.pass: once2.pass
once4.pass: once3.pass
openmp1.pass: tsd2.pass
priority1.pass: join1.pass
priority2.pass: priority1.pass barrier3.pass
reuse1.pass: create3.pass
reuse2.pass: reuse1.pass
robust1.pass: mutex8r.pass
robust2.pass: mutex8r.pass
robust3.pass: robust2.pass
robust4.pass: robust3.pass
robust5.pass: robust4.pass
rwlock1.pass: condvar6.pass
rwlock2.pass: rwlock1.pass
rwlock3.pass: rwlock2.pass join2.pass
rwlock4.pass: rwlock3.pass
rwlock5.pass: rwlock4.pass
rwlock6.pass: rwlock5.pass
rwlock7.pass: rwlock6.pass
rwlock8.pass: rwlock7.pass
rwlock2_t.pass: rwlock2.pass
rwlock3_t.pass: rwlock2_t.pass
rwlock4_t.pass: rwlock3_t.pass
rwlock5_t.pass: rwlock4_t.pass
rwlock6_t.pass: rwlock5_t.pass
rwlock6_t2.pass: rwlock6_t.pass
self1.pass:
self2.pass: create1.pass
semaphore1.pass:
semaphore2.pass:
semaphore3.pass: semaphore2.pass
semaphore4.pass: semaphore3.pass cancel1.pass
semaphore4t.pass: semaphore4.pass
semaphore5.pass: semaphore4.pass
sequence1.pass: reuse2.pass
sizes.pass:
spin1.pass:
spin2.pass: spin1.pass
spin3.pass: spin2.pass
spin4.pass: spin3.pass
stress1.pass:
tsd1.pass: barrier5.pass join1.pass
tsd2.pass: tsd1.pass
valid1.pass: join1.pass
valid2.pass: valid1.pass

sizes.pass: sizes.exe
	@ $(ECHO) Running $*
	@ $(RUN) ./$< > SIZES.$(TEST)
	@ $(CAT) SIZES.$(TEST)
	@ $(ECHO) Passed
	@ $(TOUCH) $@

%.pass: %.exe
	@ $(ECHO) Running $*
	@ $(RUN) ./$*
	@ $(ECHO) Passed
	@ $(TOUCH) $@

%.bench: $(LIB) $(DLL) $(HDR) $(QAPC) $(XXLIBS) %.exe
	@ $(ECHO) Running $*
	@ $(RUN) ./$*
	@ $(ECHO) Done
	@ $(TOUCH) $@

%.exe: %.c $(LIB) $(DLL) $(HDR) $(QAPC)
	@ $(ECHO) Compiling $@
	@ $(ECHO) $(CC) $(CFLAGS) -o $@ $< $(INCLUDES) -L. -lpthread$(GCX) -lsupc++ $(XXLIBS)
	@ $(CC) $(CFLAGS) -o $@ $< $(INCLUDES) -L. -lpthread$(GCX) -lsupc++ $(XXLIBS)

%.pre: %.c $(HDR)
	@ $(CC) -E $(CFLAGS) -o $@ $< $(INCLUDES)

%.s: %.c $(HDR)
	@ $(ECHO) Compiling $@
	@ $(CC) -S $(CFLAGS) -o $@ $< $(INCLUDES)

$(COPYFILES):
	@ $(ECHO) Copying $(BUILD_DIR)/$@
	@ $(CP) $(BUILD_DIR)/$@ .

benchlib.o: benchlib.c
	@ $(ECHO) Compiling $@
	@ $(ECHO) $(CC) -c $(CFLAGS) $< $(INCLUDES)
	@ $(CC) -c $(CFLAGS) $< $(INCLUDES)

pthread.dll: $(DLL)
	@ $(CP) $(DLL) $@

clean:
	- $(RM) *.dll
	- $(RM) *.lib
	- $(RM) pthread.h
	- $(RM) semaphore.h
	- $(RM) sched.h
	- $(RM) *.a
	- $(RM) *.e
	- $(RM) *.i
	- $(RM) *.o
	- $(RM) *.s
	- $(RM) *.so
	- $(RM) *.obj
	- $(RM) *.pdb
	- $(RM) *.exe
	- $(RM) *.pass
	- $(RM) *.bench
	- $(RM) *.static
	- $(RM) *.log
