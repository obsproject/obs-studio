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
#      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
#

DLL_VER	= 2

CP	= copy
RM	= erase
CAT	= type
MKDIR	= mkdir
TOUCH	= echo Passed >
ECHO	= echo

# The next path is relative to $BUILD_DIR
QAPC	= # ..\QueueUserAPCEx\User\quserex.dll

CPHDR	= pthread.h semaphore.h sched.h

OPTIM	= /O2 /Ob0

XXLIBS	= ws2_32.lib

# C++ Exceptions
VCEFLAGS	= /EHsc /TP /DPtW32NoCatchWarn /D__CLEANUP_CXX
VCELIB	= pthreadVCE$(DLL_VER).lib
VCEDLL	= pthreadVCE$(DLL_VER).dll
# Structured Exceptions
VSEFLAGS	= /D__CLEANUP_SEH
VSELIB	= pthreadVSE$(DLL_VER).lib
VSEDLL	= pthreadVSE$(DLL_VER).dll
# C cleanup code
VCFLAGS	= /D__CLEANUP_C
VCLIB	= pthreadVC$(DLL_VER).lib
VCDLL	= pthreadVC$(DLL_VER).dll
# C++ Exceptions in application - using VC version of pthreads dll
VCXFLAGS	= /EHsc /TP /D__CLEANUP_C

# Defaults
CPLIB	= $(VCLIB)
CPDLL	= $(VCDLL)

CFLAGS= $(OPTIM) /W3 /MD /nologo /Z7
LFLAGS= /INCREMENTAL:NO
INCLUDES=-I.
BUILD_DIR=..

COPYFILES	= $(CPHDR) $(CPLIB) $(CPDLL) $(QAPC)

TEST		=
EHFLAGS	=

# If a test case returns a non-zero exit code to the shell, make will
# stop.

PASSES = sizes.pass $(REGULAR_PASSES)

REGULAR_PASSES = loadfree.pass \
	  self1.pass  mutex5.pass  \
	  mutex1.pass  mutex1n.pass  mutex1e.pass  mutex1r.pass  \
	  semaphore1.pass  semaphore2.pass  semaphore3.pass  \
	  mutex2.pass  mutex3.pass  \
	  mutex2r.pass  mutex2e.pass  mutex3r.pass  mutex3e.pass  \
	  condvar1.pass  condvar1_1.pass  condvar1_2.pass  condvar2.pass  condvar2_1.pass  \
	  exit1.pass  create1.pass  create2.pass create3.pass reuse1.pass  reuse2.pass  equal1.pass  \
	  sequence1.pass  kill1.pass  valid1.pass  valid2.pass  \
	  exit2.pass  exit3.pass  exit4.pass  exit5.pass  \
	  join0.pass  join1.pass  detach1.pass  join2.pass join3.pass  \
	  mutex4.pass  mutex6.pass  mutex6n.pass  mutex6e.pass  mutex6r.pass  \
	  mutex6s.pass  mutex6es.pass  mutex6rs.pass  \
	  mutex7.pass  mutex7n.pass  mutex7e.pass  mutex7r.pass  \
	  mutex8.pass  mutex8n.pass  mutex8e.pass  mutex8r.pass  \
	  robust1.pass  robust2.pass  robust3.pass  robust4.pass  robust5.pass  \
	  count1.pass  \
	  once1.pass  once2.pass  once3.pass  once4.pass  \
	  self2.pass  \
	  cancel1.pass  cancel2.pass  \
	  semaphore4.pass  semaphore4t.pass  semaphore5.pass  \
	  barrier1.pass  barrier2.pass  barrier3.pass  barrier4.pass  barrier5.pass  barrier6.pass  \
	  tsd1.pass  tsd2.pass  delay1.pass  delay2.pass  eyal1.pass  \
	  condvar3.pass  condvar3_1.pass  condvar3_2.pass  condvar3_3.pass  \
	  condvar4.pass  condvar5.pass  condvar6.pass  \
	  condvar7.pass  condvar8.pass  condvar9.pass  \
	  errno1.pass  \
	  rwlock1.pass  rwlock2.pass  rwlock3.pass  rwlock4.pass  \
	  rwlock5.pass  rwlock6.pass  rwlock7.pass  rwlock8.pass  \
	  rwlock2_t.pass  rwlock3_t.pass  rwlock4_t.pass  rwlock5_t.pass  rwlock6_t.pass  rwlock6_t2.pass  \
	  context1.pass  \
	  cancel3.pass  cancel4.pass  cancel5.pass  cancel6a.pass  cancel6d.pass  \
	  cancel7.pass  cancel8.pass  \
	  cleanup0.pass  cleanup1.pass  cleanup2.pass  cleanup3.pass  \
	  priority1.pass priority2.pass inherit1.pass  \
	  spin1.pass  spin2.pass  spin3.pass  spin4.pass  \
	  exception1.pass  exception2.pass  exception3.pass  \
	  cancel9.pass  stress1.pass

BENCHRESULTS = \
	  benchtest1.bench benchtest2.bench benchtest3.bench benchtest4.bench benchtest5.bench

STRESSRESULTS = \
	  stress1.stress

STATICRESULTS = \
	  sizes.pass  \
	  self1.pass  mutex5.pass  \
	  mutex1.pass  mutex1n.pass  mutex1e.pass  mutex1r.pass  \
	  semaphore1.pass  semaphore2.pass  semaphore3.pass  \
	  mutex2.pass  mutex3.pass  \
	  mutex2r.pass  mutex2e.pass  mutex3r.pass  mutex3e.pass  \
	  condvar1.pass  condvar1_1.pass  condvar1_2.pass  condvar2.pass  condvar2_1.pass  \
	  exit1.pass  create1.pass  create2.pass  create3.pass  reuse1.pass  reuse2.pass  equal1.pass  \
	  sequence1.pass  kill1.pass  valid1.pass  valid2.pass  \
	  exit2.pass  exit3.pass  exit4.pass  exit5.pass  \
	  join0.pass  join1.pass  detach1.pass  join2.pass join3.pass  \
	  mutex4.pass  mutex6.pass  mutex6n.pass  mutex6e.pass  mutex6r.pass  \
	  mutex6s.pass  mutex6es.pass  mutex6rs.pass  \
	  mutex7.pass  mutex7n.pass  mutex7e.pass  mutex7r.pass  \
	  mutex8.pass  mutex8n.pass  mutex8e.pass  mutex8r.pass  \
	  robust1.pass  robust2.pass  robust3.pass  robust4.pass  robust5.pass  \
	  count1.pass  \
	  once1.pass  once2.pass  once3.pass  once4.pass  \
	  self2.pass  \
	  cancel1.pass  cancel2.pass  \
	  semaphore4.pass  semaphore4t.pass  semaphore5.pass  \
	  barrier1.pass  barrier2.pass  barrier3.pass  barrier4.pass  barrier5.pass  barrier6.pass  \
	  tsd1.pass  tsd2.pass  delay1.pass  delay2.pass  eyal1.pass  \
	  condvar3.pass  condvar3_1.pass  condvar3_2.pass  condvar3_3.pass  \
	  condvar4.pass  condvar5.pass  condvar6.pass  \
	  condvar7.pass  condvar8.pass  condvar9.pass  \
	  errno1.pass  \
	  rwlock1.pass  rwlock2.pass  rwlock3.pass  rwlock4.pass  \
	  rwlock5.pass  rwlock6.pass  rwlock7.pass  rwlock8.pass  \
	  rwlock2_t.pass  rwlock3_t.pass  rwlock4_t.pass  rwlock5_t.pass  rwlock6_t.pass  rwlock6_t2.pass  \
	  context1.pass  \
	  cancel3.pass  cancel4.pass  cancel5.pass  cancel6a.pass  cancel6d.pass  \
	  cancel7.pass  cancel8.pass  \
	  cleanup0.pass  cleanup1.pass  cleanup2.pass  cleanup3.pass  \
	  priority1.pass priority2.pass inherit1.pass  \
	  spin1.pass  spin2.pass  spin3.pass  spin4.pass  \
	  exception1.pass  exception2.pass  exception3.pass  \
	  cancel9.pass  stress1.pass

help:
	@ $(ECHO) Run one of the following command lines:
	@ $(ECHO) nmake clean VC          (to test using VC dll with VC (no EH) apps)
	@ $(ECHO) nmake clean VC-bench    (to benchtest using VC dll with C bench apps)
	@ $(ECHO) nmake clean VC-stress   (to stresstest using VC dll with C stress apps)
	@ $(ECHO) nmake clean VC-static   (to test using VC static lib with VC (no EH) apps)
	@ $(ECHO) nmake clean VCX         (to test using VC dll with VC++ (EH) applications)
	@ $(ECHO) nmake clean VCX-bench   (to benchtest using VC dll with C++ bench apps)
	@ $(ECHO) nmake clean VCX-stress  (to stresstest using VC dll with C++ stress apps)
	@ $(ECHO) nmake clean VCE         (to test using the VCE dll with VC++ EH applications)
	@ $(ECHO) nmake clean VCE-bench   (to benchtest using VCE dll with C++ bench apps)
	@ $(ECHO) nmake clean VCE-stress  (to stresstest using VCE dll with C++ stress apps)
	@ $(ECHO) nmake clean VSE         (to test using VSE dll with VC (SEH) apps)
	@ $(ECHO) nmake clean VSE-bench   (to benchtest using VSE dll with SEH bench apps)
	@ $(ECHO) nmake clean VSE-stress  (to stresstest using VSE dll with SEH stress apps)

all:
	@ $(MAKE) /E clean VC
	@ $(MAKE) /E clean VCX
	@ $(MAKE) /E clean VCE
	@ $(MAKE) /E clean VSE
	@ $(MAKE) /E clean VC-bench
	@ $(MAKE) /E clean VC-stress

# This allows an individual test application to be made using the default lib.
# e.g. nmake clean test cancel3.exe
test: $(CPLIB) $(CPDLL) $(CPHDR) $(QAPC)

tests: $(CPLIB) $(CPDLL) $(CPHDR) $(QAPC) $(PASSES)
	@ $(ECHO) ALL TESTS PASSED! Congratulations!

benchtests: $(CPLIB) $(CPDLL) $(CPHDR) $(XXLIBS) $(BENCHRESULTS)
	@ $(ECHO) ALL BENCH TESTS DONE.

stresstests: $(CPLIB) $(CPDLL) $(CPHDR) $(STRESSRESULTS)
	@ $(ECHO) ALL STRESS TESTS DONE.

statictests: $(CPLIB) $(CPDLL) $(CPHDR) $(STATICRESULTS)
	@ $(ECHO) ALL STATIC TESTS DONE.

sizes.pass: sizes.exe
	@ $(ECHO) ... Running $(TEST)$(DLL_VER) test: $*.exe
	@ .\$*.exe > SIZES.$(TEST)
	@ $(CAT) SIZES.$(TEST)
	@ $(ECHO) ...... Passed
	@ $(TOUCH) $*.pass

$(REGULAR_PASSES): $*.exe
	@ $(ECHO) ... Running $(TEST) test: $*.exe
	@ .\$*.exe
	@ $(ECHO) ...... Passed
	@ $(TOUCH) $*.pass

$(BENCHRESULTS): $*.exe
	@ $(ECHO) ... Running $(TEST) benchtest: $*.exe
	@ .\$*.exe
	@ $(ECHO) ...... Done
	@ $(TOUCH) $*.bench

$(STRESSRESULTS): $*.exe
	@ $(ECHO) ... Running $(TEST) stresstest: $*.exe
	@ .\$*.exe
	@ $(ECHO) ...... Done
	@ $(TOUCH) $*.pass

VC:
	@ $(MAKE) /E TEST="$@" CPLIB="$(VCLIB)" CPDLL="$(VCDLL)" EHFLAGS="$(VCFLAGS)" tests

VCE:
	@ $(MAKE) /E TEST="$@" CPLIB="$(VCELIB)" CPDLL="$(VCEDLL)" EHFLAGS="$(VCEFLAGS)" tests

VSE:	
	@ $(MAKE) /E TEST="$@" CPLIB="$(VSELIB)" CPDLL="$(VSEDLL)" EHFLAGS="$(VSEFLAGS)" tests

VCX:
	@ $(MAKE) /E TEST="$@" CPLIB="$(VCLIB)" CPDLL="$(VCDLL)" EHFLAGS="$(VCXFLAGS)" tests

VC-bench:
	@ $(MAKE) /E TEST="$@" CPLIB="$(VCLIB)" CPDLL="$(VCDLL)" EHFLAGS="$(VCFLAGS)" XXLIBS="benchlib.o" benchtests

VCE-bench:
	@ $(MAKE) /E TEST="$@" CPLIB="$(VCELIB)" CPDLL="$(VCEDLL)" EHFLAGS="$(VCEFLAGS)" XXLIBS="benchlib.o" benchtests

VSE-bench:
	@ $(MAKE) /E TEST="$@" CPLIB="$(VSELIB)" CPDLL="$(VSEDLL)" EHFLAGS="$(VSEFLAGS)" XXLIBS="benchlib.o" benchtests

VCX-bench:
	@ $(MAKE) /E TEST="$@" CPLIB="$(VCLIB)" CPDLL="$(VCDLL)" EHFLAGS="$(VCXFLAGS)" XXLIBS="benchlib.o" benchtests

VC-stress:
	@ $(MAKE) /E TEST="$@" CPLIB="$(VCLIB)" CPDLL="$(VCDLL)" EHFLAGS="$(VCFLAGS)" stresstests

VCE-stress:
	@ $(MAKE) /E TEST="$@" CPLIB="$(VCELIB)" CPDLL="$(VCEDLL)" EHFLAGS="$(VCEFLAGS)" stresstests

VSE-stress:
	@ $(MAKE) /E TEST="$@" CPLIB="$(VSELIB)" CPDLL="$(VSEDLL)" EHFLAGS="$(VSEFLAGS)" stresstests

VCX-stress:
	@ $(MAKE) /E TEST="$@" CPLIB="$(VCLIB)" CPDLL="$(VCDLL)" EHFLAGS="$(VCXFLAGS)" stresstests

VC-static:
	@ $(MAKE) /E TEST="$@" CPLIB="$(VCLIB)" CPDLL="" EHFLAGS="$(VCFLAGS) /DPTW32_STATIC_LIB" statictests

.c.exe:
	@ $(ECHO) $(CC) $(EHFLAGS) $(CFLAGS) $(INCLUDES) $< /Fe$@ /link $(LFLAGS) $(CPLIB) $(XXLIBS)
	@ $(CC) $(EHFLAGS) $(CFLAGS) $(INCLUDES) $< /Fe$@ /link $(LFLAGS) $(CPLIB) $(XXLIBS)

.c.o:
	@ $(ECHO) $(CC) $(EHFLAGS) /c $(CFLAGS) $(INCLUDES) $< /Fo$@
	@ $(CC) $(EHFLAGS) $(CFLAGS) /c $(INCLUDES) $< /Fo$@

.c.i:
	@ $(CC) /P $(EHFLAGS) $(CFLAGS) $(INCLUDES) $<

$(COPYFILES):
	@ $(ECHO) Copying $(BUILD_DIR)\$@
	@ $(CP) $(BUILD_DIR)\$@ .

pthread.dll: $(CPDLL)
	@ $(CP) $(CPDLL) pthread.dll
	@ $(CP) $(CPLIB) pthread.lib

clean:
	- $(RM) *.dll
	- $(RM) *.lib
	- $(RM) pthread.h
	- $(RM) semaphore.h
	- $(RM) sched.h
	- $(RM) *.e
	- $(RM) *.i
	- $(RM) *.obj
	- $(RM) *.pdb
	- $(RM) *.o
	- $(RM) *.asm
	- $(RM) *.exe
	- $(RM) *.pass
	- $(RM) *.bench
	- $(RM) *.log

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
exit5.pass: kill1.pass
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
stress1.pass: condvar9.pass barrier5.pass
tsd1.pass: barrier5.pass join1.pass
tsd2.pass: tsd1.pass
valid1.pass: join1.pass
valid2.pass: valid1.pass
