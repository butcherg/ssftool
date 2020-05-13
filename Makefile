CC=g++

ssftool: ssftool.cpp
	${CC} -ggdb -o ssftool ssftool.cpp

install:
	cp ssftool /usr/local/bin/.

clean:
	rm -rf *.o ssftool

distclean:
	rm -rf /usr/local/bin/ssftool