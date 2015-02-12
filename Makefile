FLAGS=-L/usr/local/lib -lcygwin -lSDL2_image -lSDL2main -lSDL2 -I/usr/local/include/SDL2 -I/usr/include/mingw -Dmain=SDL_main -std=c++11 -g
game: assets.h main.cpp
	clang++ -o game main.cpp $(FLAGS)
