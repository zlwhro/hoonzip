hoonzip: main.o decomp.o
	gcc -fsanitize=address -ggdb -o hoonzip main.o decomp.o

main.o: main.c
	gcc -fsanitize=address -ggdb -c main.c hoonzip.h

decomp.o : decomp.c decomp.h
	gcc -fsanitize=address -ggdb -c decomp.c decomp.h  -march=native

clean:
	rm *.o *.gch
	rm hoonzip
