## Print a random line in a file

This is an experiment to learn about stdio performance, error handling, usage
and portability. This project should be portable to any platform with a hosted
C environment.

There are currently four tactics it can use to choose a line to print.

### Random fseek

**Find a line with probability proportional to its length.** Fseek to the end
of the file, ftell the length, then fseek to a random point in between.
Advance to the beginning of the next line (wrapping to the start on EOF), and
print that line, character by character. So it's actually most likely to pick
a line immediately *after* a long one.

**Pros:**

Very fast. Does not have to read any characters except to find the start of a
line and print it. Because the block size for reads is 4096 bytes on the
OpenBSD system I tested, all needed data is usually returned by one block read.

**Cons:**

1. Fseek/ftell measure the byte position as a "long," which on some systems is
   32-bits. This limits the file size on such systems.
2. My use of the offset in fseek requires opening the file in binary mode. If a
   system has a newline convention where there are any characters after '\\n' then
   those characters will be considered as the start of the line to be printed.

### Line Bookmarks

**Choose all lines with equal probability.** Makes a full pass through the
file, calling fgetpos() at the start of each line and saving the results into
an array, reallocating the array at double size each time more space is needed.
After consuming all lines, picks a random line number, jumps to the bookmark
with fsetpos() and re-reads and prints the line from the file.

**Pros:**

* Can handle files with line numbers that fit in type size\_t. Even if that's
  32-bits, it's better than fseek's total character count limit of 32-bits.

**Cons:**

* Requires at least sizeof(fpos\_t)\*line\_count bytes of memory
* Requires scanning through the whole file.

Future improvement for this method would be to accept a paramter to bookmark
every n-th line rather than all lines.

### Exponential bookmarks

**Choose all lines with equal probability.** Like the line bookmark technique,
but puts the bookmarks at lines 1, 2, 4, 8...

**Pros:**

* Small fixed memory usage
* Practically unlimited file size (although you do have to scan through it all)

**Cons:**

* Requires scanning through the whole file.
* Bookmarks get sparse later in large files, requires a potentially substantial
  re-scan to locate a specific line.

### Poisson

(Not yet supported)

**Choose line with the Poisson distribution.** Scan through the file. At the
start of each line, pick a random number. If the number falls above/below a
threshold then consume and print the line and quit.

**Pros:**

* Does not require a full scan through the file.
* Works with input streams that do not support fseek.

**Cons:**

* Requires a knowledge of the best threshold given expected stream length.
* What happens if you hit the end of the stream and hadn't chosen a line to
  print? Just pick the last line? Print nothing and return an error code?
