CFLAGS = -std=c89 -Wall -Wextra -Wshadow -Wno-deprecated-declarations

OBJS     = rand.o flexar.o
OBJS_DBG = rand.g flexar.g

.SUFFIXES :
.SUFFIXES : .g .o .c

randln: $(OBJS) randln.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ randln.c $(OBJS)

randln.g: $(OBJS_DBG) randln.c
	$(CC) $(CFLAGS) -g $(LDFLAGS) -o $@ randln.c $(OBJS_DBG)

.c.g :
	$(CC) $(CFLAGS) -g -c $<
	mv $*.o $@

rand.o rand.g : rand.c rand.h
flexar.o flexar.g : flexar.c flexar.h
