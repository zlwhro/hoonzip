hoonzip: main.o decomp.o
	gcc -o hoonzip main.o decomp.o

main.o: main.c
	gcc -c main.c hoonzip.h

decomp.o : decomp.c decomp.h
	gcc -c decomp.c decomp.h  -march=native

clean:
	rm *.o *.gch
	rm hoonzip