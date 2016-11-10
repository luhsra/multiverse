all: gcc-plugin libmultiverse tests

gcc-plugin libmultiverse tests:
	make -C $@

clean:
	make -C gcc-plugin clean
	make -C libmultiverse clean
	make -C tests clean

test:
	make -C tests test


.PHONY: gcc-plugin libmultiverse tests
