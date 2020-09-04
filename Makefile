CC=g++

all: tiff2specdata ssftool

tiff2specdata: tiff2specdata.cpp
	$(CC) -ggdb -o tiff2specdata tiff2specdata.cpp -ltiff

ssftool: ssftool.cpp
	${CC} -ggdb -o ssftool ssftool.cpp

install:
	cp ssftool /usr/local/bin/.
	cp tiff2specdata /usr/local/bin/.

clean:
	rm -rf *.o ssftool tiff2specdata

distclean:
	rm -rf /usr/local/bin/ssftool
	rm -rf /usr/local/bin/tiff2specdata
	