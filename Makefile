all: gcc-plugin libmultiverse tests


gcc-plugin libmultiverse tests:
	$(MAKE) -C $@

clean:
	$(MAKE) -C gcc-plugin clean
	$(MAKE) -C libmultiverse clean
	$(MAKE) -C tests clean

test:
	$(MAKE) -C tests test

GCCPLUGINS_DIR:= $(shell $(CXX) -print-file-name=plugin)

.PHONY: install
install:
	cp libmultiverse.pc $(DESTDIR)/usr/lib/pkgconfig/libmultiverse.pc
	$(MAKE) -C gcc-plugin install
	$(MAKE) -C libmultiverse install

.PHONY: uninstall
uninstall:
	$(MAKE) -C gcc-plugin uninstall
	$(MAKE) -C libmultiverse uninstall
	rm -f $(DESTDIR)/usr/lib/pkgconfig/libmultiverse.pc

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
