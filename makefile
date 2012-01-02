
all: bin/pianoclient bin/pianoserver

bin/pianoserver: pianoserver.c
	mkdir -p bin
	gcc -o $@ $^

bin/pianoclient: pianoclient.pl
	mkdir -p bin
	cp -fr $^ $@

clean:
	rm bin/pianoserver bin/pianoclient
