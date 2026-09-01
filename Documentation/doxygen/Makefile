CSSFILES = $(shell ls *.css)

.PHONY: all

all:
	@which plantuml
	@test -d html || mkdir html
	doxygen
	cp $(CSSFILES) html/

.PHONY: clean
clean:
	rm -rf html

