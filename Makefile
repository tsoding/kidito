GL_PKGS=glfw3 glew
CFLAGS=-Wall -Wextra
SRC=main.c geo.c

main: $(SRC)
	$(CC) $(CFLAGS) `pkg-config --cflags $(GL_PKGS)` -o main $(SRC) `pkg-config --libs $(GL_PKGS)`

