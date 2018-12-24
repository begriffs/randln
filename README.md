## Print a random line in a file

This is an experiment to learn about stdio performance, error handling, usage
and portability. This project should be portable to any platform with a hosted
C environment.

```
usage: ./randln [-m(f|r|R|p)] [-p(fff)] [filename]
```

If filename is missing or "-" then the utility reads from stdin.

There are currently four tactics it can use to choose a line to print.

### Random fseek  (-mf)

**Find a line with probability proportional to its length.** Fseek to the end
of the file, ftell the length, then fseek to a random point in between.
Advance to the beginning of the next line (wrapping to the start on EOF), and
print that line, character by character. So it's actually most likely to pick
a line immediately *after* a long one.

This is the default method if no method is specified in command line arguments.

**Pros:**

Very fast. Does not have to read any characters except to find the start
of a line and print it. All needed data is usually returned by one
system I/O call.

**Cons:**

Using an offset in fseek requires opening the file in binary mode. If
a system has a newline convention with characters after '\\n' (like
'\\n\\r') then those characters will be considered as the start of the
next line to be printed.

Also the bias by line length.

### Reservoir Sampling of lines  (-mr)

**Choose a line with uniform probability.** Always makes a full pass
through the file, at each line replacing replacing a temporary buffer
with decreasing probability. Mathematically it works out that on
reaching the nth line, that buffer will hold any of the scanned lines
with equal probability of 1/n. The final contents of the buffer on EOF
are the result.

**Pros:**

* All lines, no matter how long or short, get the same chance.
* Works on streams that do not support random access.

**Cons:**

* Requires scanning through the whole file. Slower than the fseek method.
* Will never terminate when reading from a never-ending stream.

### Reservoir Sampling of file positions  (-mR)

**Choose a lines with uniform probability.** Same probablistic
algorithm as method `-mr` except rather than copying selected lines
into a temporary buffer, it copies an fpos\_t value obtained with
fgetpos(). Upon reaching EOF, it uses fsetpos() to go back to selected
line and prints it.

**Pros:**

* Same as `-mr`
* Plus does not require space in memory to hold lines. Great for long lines.

**Cons:**

* Same as `-mr`
* However does use fgetpos()/fsetpos() so cannot operate on streams that
  lack random access.

### Poisson  (-mp)

**Choose line with the Poisson distribution.** Scan through the file. At
the start of each line, pick a random number between 0 and 1. If the
number falls below a user-specified (with `-pfff` e.g. `-p0.00001`)
threshold then consume and print the line and quit.

**Pros:**

* Does not require a full scan through the file.
* Works with input streams that do not support fseek.
* Works on never-ending streams.

**Cons:**

* Requires tuning the threshold to match expected file length.
* What happens if you hit the end of the stream and hadn't chosen a line to
  print? It currently *tries* to rewind and continue, but if the stream does
  not support fseek then it quits without printing a line.
