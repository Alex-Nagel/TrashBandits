.phony: all clean

all: text2vol5 nsf2data5

clean:
	-rm text2vol5 nsf2data5

text2vol5: text2vol5.cpp
	$(CC) -o $@ $<

nsf2data5: nsf2data5.cpp
	$(CC) -o $@ $<
