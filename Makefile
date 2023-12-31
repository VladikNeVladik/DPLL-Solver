CFLAGS = \
	-std=c11 \
	-Wpedantic \
	-Werror \
	-O2

HEADERS = \
	dimacs.h \
	formula.h \
	utils.h \
	template_stack.h

dpll: dpll.c $(HEADERS)
	@gcc $< $(CFLAGS) -o $@

NUMBERS100  = $(shell seq 1  100)
NUMBERS1000 = $(shell seq 1 1000)

UF20_SAT_TESTS    = $(NUMBERS1000:%=res/uf20-91/uf20-0%.cnf)

UF50_SAT_TESTS   = $(NUMBERS1000:%=res/uf50-218/uf50-0%.cnf)
UF50_UNSAT_TESTS = $(NUMBERS1000:%=res/UUF50.218.1000/uuf50-0%.cnf)

UF75_SAT_TESTS   = $(NUMBERS100:%=res/UF75.325.100/uf75-0%.cnf)
UF75_UNSAT_TESTS = $(NUMBERS100:%=res/UUF75.325.100/uuf75-0%.cnf)

UF100_SAT_TESTS   = $(NUMBERS1000:%=res/uf100-430/uf100-0%.cnf)
UF100_UNSAT_TESTS = $(NUMBERS1000:%=res/UUF100.430.1000/uuf100-0%.cnf)

UF125_SAT_TESTS   = $(NUMBERS100:%=res/uf100-430/uf100-0%.cnf)
UF125_UNSAT_TESTS = $(NUMBERS100:%=res/UUF100.430.1000/uuf100-0%.cnf)

UF150_SAT_TESTS   = $(NUMBERS100:%=res/UF150.645.100/uf150-0%.cnf)
UF150_UNSAT_TESTS = $(NUMBERS100:%=res/UUF150.645.100/uuf150-0%.cnf)

res/%: dpll
	./dpll $@

run: res/hanoi4.cnf

test: res/test.cnf

test-sat-20: $(UF20_SAT_TESTS)

test-sat-50:   $(UF50_SAT_TESTS)
test-unsat-50: $(UF50_UNSAT_TESTS)

test-sat-75:   $(UF75_SAT_TESTS)
test-unsat-75: $(UF75_UNSAT_TESTS)

test-sat-100:   $(UF100_SAT_TESTS)
test-unsat-100: $(UF100_UNSAT_TESTS)

test-sat-125:   $(UF125_SAT_TESTS)
test-unsat-125: $(UF125_UNSAT_TESTS)

test-sat-150:   $(UF150_SAT_TESTS)
test-unsat-150: $(UF150_UNSAT_TESTS)

clean:
	@rm -f dpll

.PHONY:
