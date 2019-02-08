hello3d: hello3d.cc
	g++ -m32 hello3d.cc -I/mingw32/include/SDL2 -I/mingw32/include/freetype2 -I/mingw32/include/chipmunk -L/mingw32/lib -Wl,-subsystem,windows -lmingw32 -lSDL2main -lSDL2 -lchipmunk -lglew32 -lopengl32 -lfreetype -mwindows -o hello3d.exe
