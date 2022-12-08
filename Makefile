m_clean:
	rm -f ./build/*.o

main:
	make m_clean
	gcc -Wall -g main.c -o ./build/main.o
	./build/main.o

VALGRIND_TOOL=helgrind
VALGRIND_TOOL=memcheck --track-origins=yes --leak-check=full --track-fds=yes -s
main_valgrind:
	make m_clean
	gcc -Wall -g main.c -o ./build/main.o
	valgrind --tool=$(VALGRIND_TOOL) ./build/main.o