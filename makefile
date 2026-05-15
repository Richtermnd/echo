all: echo echo_thread echo_poll

echo: echo.c
	gcc -o echo echo.c

echo_thread: echo_thread.c
	gcc -o echo_thread echo_thread.c

echo_poll: echo_poll.c
	gcc -o echo_poll echo_poll.c

clean:
	rm echo echo_thread echo_poll
