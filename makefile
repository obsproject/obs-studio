include config.mak

all: default

PROJECTS=libobs test libobs-opengl

.PHONY: all default clean

default: $(PROJECTS)
	@$(foreach PROJECT, $(PROJECTS), $(MAKE) -C $(PROJECT);)

clean: $(PROJECTS)
	@$(foreach PROJECT, $(PROJECTS), $(MAKE) clean -C $(PROJECT);)
