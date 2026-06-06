CC = cc
SRC = $(wildcard src/*.c)
EXAMPLES = $(wildcard examples/*.c)

TARGET = libwcio.dylib
EXAMPLE_TARGETS = $(EXAMPLES:examples/%.c=bin/%)

INCPATH = /usr/local/include
OPENSSL_INCPATH = /usr/local/opt/openssl@3/include
INCPATH_WCIO = ./src/
OPENSSL_LIBPATH = /usr/local/opt/openssl@3/lib

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -I$(OPENSSL_INCPATH) -fPIC -g -shared -L$(OPENSSL_LIBPATH) $(SRC) -o $(TARGET) -lssl -lcrypto

examples: $(EXAMPLE_TARGETS)

bin/%: examples/%.c $(TARGET)
	mkdir -p bin
	$(CC) -I$(INCPATH_WCIO) -g $< -L. -lwcio -o $@

install:
	cp $(TARGET) /usr/local/lib
	cp -r ./src/include/* $(INCPATH)

debug: $(SRC)
	$(CC) -g -fPIC -shared $(SRC) -o $(TARGET)

clean:
	rm -rf $(TARGET) bin
