all: gcc-plugin libmultiverse tests


gcc-plugin libmultiverse tests:
	$(MAKE) -C $@

clean:
	$(MAKE) -C gcc-plugin clean
	$(MAKE) -C libmultiverse clean
	$(MAKE) -C tests clean

test:
	$(MAKE) -C tests test

PREFIX ?= /usr/local
GCCPLUGINS_DIR:= $(shell $(CXX) -print-file-name=plugin)

.PHONY: install
install:
	$(MAKE) -C gcc-plugin install
	$(MAKE) -C libmultiverse install
	install -d $(DESTDIR)$(PREFIX)/lib/pkgconfig
	install libmultiverse.pc $(DESTDIR)$(PREFIX)/lib/pkgconfig/

.PHONY: uninstall
uninstall:
	$(MAKE) -C gcc-plugin uninstall
	$(MAKE) -C libmultiverse uninstall
	rm -f $(DESTDIR)$(PREFIX)/lib/pkgconfig/libmultiverse.pc

# Docker rules for build testing
PWD=$(shell pwd)
DOCKERRUN=docker run -v $(PWD):/src

build-docker-images:
	cat docker/Dockerfile.gcc5 | docker build -t multiverse-test-gcc5 -
	cat docker/Dockerfile.gcc6 | docker build -t multiverse-test-gcc6 -
	cat docker/Dockerfile.gcc7 | docker build -t multiverse-test-gcc7 -

run-docker-gcc5:
	$(DOCKERRUN) multiverse-test-gcc5

run-docker-gcc6:
	$(DOCKERRUN) multiverse-test-gcc6

run-docker-gcc7:
	$(DOCKERRUN) multiverse-test-gcc7


.PHONY: gcc-plugin libmultiverse tests
