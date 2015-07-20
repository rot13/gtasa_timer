all: timer.exe

timer.exe : timer.c
	i686-w64-mingw32-gcc -std=c99 timer.c -o timer.exe -s -mwindows
clean:
	rm *.exe
