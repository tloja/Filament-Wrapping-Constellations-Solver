CC = gcc
CFLAGS = -Iinc -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wcast-align -Wpointer-arith -Wmissing-prototypes -Wunused-parameter -Wfloat-equal -Wundef -Wredundant-decls -fsanitize=undefined

TARGET = wrap_solver

all: $(TARGET)

$(TARGET): wrap_solver.o
	$(CC) $(CFLAGS) -o $(TARGET) wrap_solver.o

wrap_solver.o: wrap_solver.c
	$(CC) $(CFLAGS) -c wrap_solver.c

clean: 
	rm -f $(TARGET) wrap_solver.o


.PHONY: all clean