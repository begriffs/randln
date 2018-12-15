CFLAGS = -std=c89 -Wall -Wextra -Wshadow -Wno-deprecated-declarations
LDFLAGS = -lm

# Experiment to enable profiling with clang:
#   LDFLAGS = -lm -fprofile-instr-generate
# does not work yet

all: randln
