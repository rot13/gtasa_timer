all: gtasa_timer.exe

gtasa_timer.exe : font.S gtasa_timer.c
	i686-w64-mingw32-gcc -std=c99 font.S gtasa_timer.c -o gtasa_timer.exe -s -mwindows
clean:
	rm *.exe
