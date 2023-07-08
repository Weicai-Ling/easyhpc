SRC = easyhpc.c pages.c posts.c sessions.c files.c exec.c common.c
OBJ = $(SRC:.c=.o)
PROG = easyhpc

$(PROG): $(OBJ)
	gcc $(OBJ) -o $(PROG) -I/usr/local/include -L/usr/local/lib -lmicrohttpd -Wl,-rpath,/usr/local/lib
$(OBJ): $(SRC)

clean:
	rm -f $(OBJ) $(PROG)