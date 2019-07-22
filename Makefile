all:
	gcc -Wall -g main.c utils.c cmd_line.c -o holc
release:
	gcc -Wall -O2 main.c utils.c cmd_line.c -o holc