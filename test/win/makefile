include ../../config.mak

.PHONY: all default clean

all: default

SRCFILES=test.cpp

NAME=../../build/wintest$(EXT)

OBJS += $(SRCFILES:%.cpp=%.$(OBJ))

LD=g++ -o
LDFLAGS=-L../../build -lobs -Wl,--subsystem,windows

CPPFLAGS += -I../../libobs -DUNICODE -D_UNICODE

default: $(NAME)

.depend:
	@rm -f .depend
	@$(foreach SRC, $(addprefix $(SRCPATH)/, $(SRCFILES)), $(CCDEP) \
	       $(CPPFLAGS) $(SRC) \
	       -MT $(SRC:$(SRCPATH)/%.c=%.$(OBJ)) -MM 1>> .depend;)

$(NAME): .depend $(OBJS)
	$(LD)$@ $(LDFLAGS) $(OBJS)

depend: .depend
ifneq ($(wildcard .depend),)
include .depend
endif

clean:
	rm -f $(OBJS) $(NAME) *.a *.lib *.exp *.pdb .depend
