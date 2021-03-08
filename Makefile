GL_PKGS=glfw3 glew
CFLAGS=-Wall -Wextra
SRC=src/main.c src/geo.c src/sv.c

kidito: $(SRC)
	$(CC) $(CFLAGS) `pkg-config --cflags $(GL_PKGS)` -o kidito $(SRC) `pkg-config --libs $(GL_PKGS)` -lm

