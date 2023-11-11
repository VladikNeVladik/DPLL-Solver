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

run: dpll
	./dpll res/hanoi4.cnf

clean:
	@rm -f dpll
