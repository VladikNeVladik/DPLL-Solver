CFLAGS = \
	-std=c11 \
	-Wpedantic \
	-Werror \
	-O2 \
	-pg

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

UF100_SAT_TESTS   = $(NUMBERS1000:%=res/uf100-430/uf100-0%.cnf)
UF100_UNSAT_TESTS = $(NUMBERS1000:%=res/UUF100.430.1000/uuf100-0%.cnf)

UF150_SAT_TESTS   = $(NUMBERS100:%=res/UF150.645.100/uf150-0%.cnf)
UF150_UNSAT_TESTS = $(NUMBERS100:%=res/UUF150.645.100/uuf150-0%.cnf)

res/%: dpll
	./dpll $@

run: res/hanoi4.cnf

test: res/test.cnf

test-sat-little: $(UF20_SAT_TESTS)

test-sat-middle: $(UF100_SAT_TESTS)

test-unsat-middle: $(UF100_UNSAT_TESTS)

test-sat-big: $(UF150_SAT_TESTS)

test-unsat-big: $(UF150_UNSAT_TESTS)

clean:
	@rm -f dpll

.PHONY:
