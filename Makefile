all: shell.c
	gcc shell.c -o shell
clean:
	$(RM) shell
