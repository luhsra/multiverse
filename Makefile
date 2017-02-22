all: gcc-plugin libmultiverse tests

gcc-plugin libmultiverse tests:
	$(MAKE) -C $@

clean:
	$(MAKE) -C gcc-plugin clean
	$(MAKE) -C libmultiverse clean
	$(MAKE) -C tests clean

test:
	$(MAKE) -C tests test


.PHONY: gcc-plugin libmultiverse tests
