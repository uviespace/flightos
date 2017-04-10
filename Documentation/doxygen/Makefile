CSSFILES = $(shell ls *.css)
JSFILES = $(shell ls *.js)
HTFILES = header.html footer.html

.PHONY: all

all:
	@which plantuml
	@test -d html || mkdir html
	doxygen
	cp $(CSSFILES) $(JSFILES) $(HTFILES) html/

.PHONY: clean
clean:
	rm -rf html

