all: gtasa_timer.exe

gtasa_timer.exe : gtasa_timer.c
	i686-w64-mingw32-gcc -std=c99 gtasa_timer.c -o gtasa_timer.exe -s -mwindows
clean:
	rm *.exe
