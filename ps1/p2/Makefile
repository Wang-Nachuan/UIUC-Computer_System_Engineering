# Makefile ASM to C
# Harsh Agrawal
# 1/26/21

PROG := kthsmallest

CFLAGS += -m32 -Wall -std=c99

.PHONY: clean

all: CFLAGS += -O0
all: $(PROG)

debug: CFLAGS += -O0 -g
debug: $(PROG)

$(PROG): kthsmallest_asm.o kthsmallest.o main.o
	 $(CC) -m32 $^ -o $@

kthsmallest_asm.o: kthsmallest_asm.S
	$(CC) $(CFLAGS) -c $< -o $@

kthsmallest.o: kthsmallest.c
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(PROG)
