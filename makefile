
all: pianoclient pianoserver

pianoserver: pianoserver.c
	gcc -o $@ $^

pianoclient: pianoclient.pl
	cp -fr $^ $@

clean:
	rm pianoserver pianoclient
