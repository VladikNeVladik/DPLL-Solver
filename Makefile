CFLAGS = \
	-std=c11 \
	-Wpedantic \
	-Werror \
	-O2

dpll: dpll.c
	gcc $< $(CFLAGS) -o $@

run:
	./dpll hanoi4.cnf

clean:
	rm dpll
