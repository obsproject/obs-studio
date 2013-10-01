include ../config.mak

all: default

SRCFILES=GS_D3D11SubSystem.cpp \
	 GS_D3D11IndexBuffer.cpp \
	 GS_D3D11Shader.cpp \
	 GS_D3D11ShaderProcessor.cpp \
	 GS_D3D11StageSurf.cpp \
	 GS_D3D11Texture2D.cpp \
	 GS_D3D11VertexBuffer.cpp \
	 GS_D3D11ZStencilBuffer.cpp

SONAME=../build/libobs-d3d11.$(SOEXT)

LD=g++ -o 

CPPFLAGS += -iquote $(BASEINC) 
CPPFLAGS += -iquote ../libobs-graphics/
CPPFLAGS += -isystem ./mingw/

LDFLAGS += -L../build/

ifdef monolithic
LDFLAGS += -lobs
else
LDFLAGS += -lobs-util -lobs-graphics
endif

.PHONY: all monolithic default clean

OBJS += $(SRCFILES:%.cpp=%.$(OBJ))

default: $(SONAME)

$(SONAME): .depend $(OBJS)
	$(LD)$@ $(LDFLAGS) $(OBJS)

.depend:
	@rm -f .depend
	@$(foreach SRC, $(addprefix $(SRCPATH)/, $(SRCFILES)), $(CCDEP) \
	       -std=c++11 $(CPPFLAGS) $(SRC) -I$(BASEINC) \
	       -MT $(SRC:$(SRCPATH)/%.cpp=%.$(OBJ)) -MM 1>> .depend;)

depend: .depend
ifneq ($(wildcard .depend),)
include .depend
endif

clean:
	rm -f $(OBJS) $(SONAME) *.a *.lib *.exp *.pdb .depend
