PACKAGES=libgeoclue-2.0 json-c

main: CFLAGS=$(shell pkg-config --cflags $(PACKAGES)) -g -Wall
main: LDLIBS=$(shell pkg-config --libs $(PACKAGES)) -lmosquitto
main: owntracks.o battery.o

test: CFLAGS=$(shell pkg-config --cflags $(PACKAGES)) -g -Wall
test: LDLIBS=$(shell pkg-config --libs $(PACKAGES)) -lmosquitto
test: owntracks.o

all: main
