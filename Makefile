CC = cc
SRC = $(wildcard src/*.c)
EXAMPLES = $(wildcard examples/*.c)

TARGET = libwcio.dylib
EXAMPLE_TARGETS = $(EXAMPLES:examples/%.c=bin/%)

INCPATH = /usr/local/include
INCPATH_WCIO = ./src/

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -fPIC -shared $(SRC) -o $(TARGET)

examples: $(EXAMPLE_TARGETS)

bin/%: examples/%.c $(TARGET)
	mkdir -p bin
	$(CC) -I$(INCPATH_WCIO) $< -L. -lwcio -o $@

install:
	cp $(TARGET) /usr/local/lib
	cp -r ./src/include/* $(INCPATH)

debug: $(SRC)
	$(CC) -g -fPIC -shared $(SRC) -o $(TARGET)

clean:
	rm -rf $(TARGET) bin
