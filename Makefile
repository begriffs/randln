CFLAGS = -std=c89 -Wall -Wextra -Wshadow -Wno-deprecated-declarations

# Experiment to enable profiling with clang:
#   LDFLAGS = -fprofile-instr-generate
# does not work yet

randln: rand.o flexar.o randln.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ randln.c rand.o flexar.o

rand.o: rand.c rand.h

flexar.o: flexar.c flexar.h
