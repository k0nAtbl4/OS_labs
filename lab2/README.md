gcc -c help.c -o help.o
gcc -o lab2   lab2.c   help.o
gcc -o child1 child1.c help.o
gcc -o child2 child2.c help.o
./lab2