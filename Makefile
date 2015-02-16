FLAGS=-std=c++11 -g -L/usr/lib/x86_64-linux-gnu -lSDL2 -I/usr/include/SDL2 -D_REENTRANT -lSDL2_image
game: assets.h main.cpp
	clang++ -o game main.cpp $(FLAGS)
