bitrepl : main.o adthashmap.o
	gcc -o bitrepl main.o adthashmap.o

main.o : main.c adthashmap.h
	gcc -c main.c
adthashmap.o : adthashmap.c
	gcc -c adthashmap.c

clean :
	rm bitrepl main.o adthashmap.o
