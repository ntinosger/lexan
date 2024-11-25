# Use shell
SHELL = /bin/sh

# Compiler flags
CFLAGS = -Wall -g  # Enables warnings and debugging info
CC = gcc           # Compiler to be used

# Targets
LEXAN_TARGET = lexan
SPLITTER_TARGET = splitter
BUILDER_TARGET = builder

# Object files for each executable
LEXAN_OBJS = lexan.o hashTable.o globals.o
SPLITTER_OBJS = splitter.o hashTable.o globals.o
BUILDER_OBJS = builder.o hashTable.o globals.o

# Build both executables
all: $(LEXAN_TARGET) $(SPLITTER_TARGET) $(BUILDER_TARGET)

# Build lexan executable
$(LEXAN_TARGET): $(LEXAN_OBJS)
	$(CC) $(CFLAGS) -o $@ $(LEXAN_OBJS)

# Build splitter executable
$(SPLITTER_TARGET): $(SPLITTER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SPLITTER_OBJS)

# Build builder executable
$(BUILDER_TARGET): $(BUILDER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(BUILDER_OBJS)

# Clean up object files and executables
clean:
	-rm -f *.o core *.core $(LEXAN_TARGET) $(SPLITTER_TARGET) $(BUILDER_TARGET)

# Compile .c files into .o files
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
lexan.o: lexan.c lexan.h hashTable.h globals.h
splitter.o: splitter.c lexan.h hashTable.h globals.h
builder.o: builder.c lexan.h hashTable.h globals.h
hashTable.o: hashTable.c hashTable.h globals.h
globals.o: globals.c globals.h