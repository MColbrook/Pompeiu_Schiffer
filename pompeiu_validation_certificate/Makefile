CXX ?= g++
CPPFLAGS ?=
CXXFLAGS ?= -O3 -std=c++17 -Wall -Wextra -Wpedantic -fopenmp -frounding-math -fno-fast-math -ffp-contract=off
LDFLAGS ?= -fopenmp
LDLIBS ?=
MPFR_CFLAGS := $(shell pkg-config --cflags mpfr)
MPFR_LIBS := $(shell pkg-config --libs mpfr)
CPPFLAGS += $(MPFR_CFLAGS)
LDLIBS += $(MPFR_LIBS)

BIN := work/interval_assemble
CHECKER_SOURCE := $(if $(wildcard verify_certificate.py),verify_certificate.py,src/verify_certificate.py)
PRODUCTION_SOURCES := src/interval_assemble.cpp src/certify.py src/recurrence_reference.py tests/smoke.py tests/symbolic.py $(CHECKER_SOURCE)

.PHONY: all smoke symbolic finite-seed stats clean

all: $(BIN)

$(BIN): src/interval_assemble.cpp | work
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

work:
	mkdir -p work

smoke: $(BIN)
	python3 tests/smoke.py

symbolic:
	python3 tests/symbolic.py

# Finite-block diagnostic.  This intentionally returns INCOMPLETE rather than
# presenting a finite calculation as a full certificate.
finite-seed: $(BIN)
	OMP_NUM_THREADS=$${THREADS:-32} $(BIN) data/center_L60_S40_R30.hex work/finite_intervals.bin $${BITS:-192}
	python3 src/certify.py --center data/center_L60_S40_R30.hex --intervals work/finite_intervals.bin --work work/finite_certificate --output results/finite_seed.json || test $$? -eq 3

stats:
	@set -eu; \
	for file in $(PRODUCTION_SOURCES); do test -f "$$file"; done; \
	test "$(words $(PRODUCTION_SOURCES))" -eq 6; \
	unique=$$(printf '%s\n' $(PRODUCTION_SOURCES) | sort -u | wc -l); \
	test "$$unique" -eq 6; \
	lines=$$(awk ' \
	function strip_c(line, p, q, out) { \
	  out = ""; \
	  while (length(line)) { \
	    if (block) { q=index(line,"*/"); if (!q) return out; line=substr(line,q+2); block=0; } \
	    else { p=index(line,"/*"); q=index(line,"//"); \
	      if (q && (!p || q<p)) return out substr(line,1,q-1); \
	      if (p) { out=out substr(line,1,p-1); line=substr(line,p+2); block=1; } \
	      else return out line; \
	    } \
	  } \
	  return out; \
	} \
	FNR==1 { block=0 } \
	{ line=$$0; \
	  if (FILENAME ~ /[.]py$$/ && line ~ /^[[:space:]]*#/) next; \
	  if (FILENAME ~ /[.](cpp|hpp)$$/) line=strip_c(line); \
	  if (line !~ /^[[:space:]]*$$/) lines++; \
	} \
	END { print lines+0 }' $(PRODUCTION_SOURCES)); \
	echo "Hand-written production source files: 6"; \
	echo "Nonblank, noncomment source lines: $$lines / 3000"; \
	if test "$$lines" -gt 3000; then echo "ERROR: source-line limit exceeded" >&2; exit 1; fi

clean:
	rm -rf work/* results/*
	@mkdir -p work results
	@touch work/.gitkeep results/.gitkeep
