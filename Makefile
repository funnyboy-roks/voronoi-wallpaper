main: main.c
	gcc -o main main.c -lraylib -lm

delaunay: delaunay.c
	gcc -o delaunay delaunay.c -lraylib -lm
