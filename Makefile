.PHONY: release

.PHONY: debug
.PHONY: clean
.PHONY: coverage
.PHONY: coverage-build

all: release release32 sanitize sanitize32 coverage

release:
	maike2 --configfiles=maikeconfig2.json,maikeconfig2-rel.json --target-dir=__targets_rel

release32:
	maike2 --configfiles=maikeconfig2_32.json,maikeconfig2-rel.json --target-dir=__targets_rel_32

sanitize:
	maike2 --configfiles=maikeconfig2.json,maikeconfig2-clangtidy.json --target-dir=__targets_clangtidy

sanitize32:
	maike2 --configfiles=maikeconfig2_32.json,maikeconfig2-clangtidy.json --target-dir=__targets_clangtidy32

debug:
	maike2 --configfiles=maikeconfig2.json,maikeconfig2-dbg.json --target-dir=__targets_dbg

clean:
	rm -rf __targets_*

coverage: __targets_gcov/.coverage/coverage.html

coverage-build:
	maike2 --configfiles=maikeconfig2.json,maikeconfig2-gcov.json --target-dir=__targets_gcov

__targets_gcov/.coverage/coverage.html: coverage-build ./coverage_collect.sh
	./coverage_collect.sh

DESTDIR?=""
PREFIX?="/usr"
install:
	mkdir -p $(DESTDIR)$(PREFIX)/include/jopp
	find -name '*.hpp' -exec cp \{\} $(DESTDIR)$(PREFIX)/include/jopp \;