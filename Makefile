export CC := gcc

SRC = easyhpc.c pages.c posts.c sessions.c easyhpc.h
OBJ = easyhpc.o pages.o posts.o sessions.o 
PROG = easyhpc

$(PROG): $(OBJ)
	gcc $(OBJ) -o $(PROG) -I/usr/local/include -L/usr/local/lib -lmicrohttpd
$(OBJ): $(SRC)

clean:
	rm -f $(OBJ) $(PROG)