include ../../config.mak

.PHONY: all default clean

all: default

SRCFILES=test-filter.c test-input.c test-random.c

SONAME=../../build/test-input.$(SOEXT)

OBJS += $(SRCFILES:%.c=%.$(OBJ))

CPPFLAGS += -iquote../../libobs
LDFLAGS += -L../../build -lobs

default: $(SONAME)

.depend:
	@rm -f .depend
	@$(foreach SRC, $(addprefix $(SRCPATH)/, $(SRCFILES)), $(CCDEP) \
	       $(CPPFLAGS) $(SRC) \
	       -MT $(SRC:$(SRCPATH)/%.c=%.$(OBJ)) -MM 1>> .depend;)

$(SONAME): .depend $(OBJS)
	$(LD)$@ $(LDFLAGS) $(OBJS)

depend: .depend
ifneq ($(wildcard .depend),)
include .depend
endif

clean:
	rm -f $(OBJS) $(SONAME) *.a *.lib *.exp *.pdb .depend
