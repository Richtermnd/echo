all: echo echo_thread

echo: echo.c
	gcc -o echo echo.c

echo_thread: echo_thread.c
	gcc -o echo_thread echo_thread.c
