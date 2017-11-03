BUILDIR = ./build

all: $(BUILDIR)
	+$(MAKE) -C Source

$(BUILDIR):
	mkdir -p $(BUILDIR)

clean:
	rm -rf $(BUILDIR)/*


