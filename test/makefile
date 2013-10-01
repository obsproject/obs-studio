include ../config.mak

all: default

PROJECTS=test-input win

.PHONY: all default clean

default: $(PROJECTS)
	@$(foreach PROJECT, $(PROJECTS), $(MAKE) -C $(PROJECT);)

clean: $(PROJECTS)
	@$(foreach PROJECT, $(PROJECTS), $(MAKE) clean -C $(PROJECT);)
