include=-I./depend/include \
		-I./depend/include/json-c

link=-L./depend/lib \
	 -ljson-c \
	 -lcharles_log \
	 -pthread

cc=g++
cppflags=-std=c++11 -fPIC -O3
cflags=-O3 -fPIC

src=charles_server.o main.o threadpool.o
so=./lib/libcharles_server.so.1
lnso=./lib/libcharles_server.so
bin=server

all:$(so) $(bin)

$(so):$(src)
	$(cc) -shared -fPIC -Wl,-soname,libcharles_server.so.1 $^ -o $@ $(link)
	cp -rd depend/include/* include/
	cp -rd depend/lib/* lib/
	cp charles_server.h include/
	cp config.h include/
	-cd lib && ln -s libcharles_server.so.1 libcharles_server.so

$(bin):$(src)
	g++ -I./include -L./lib -std=c++11 main.cpp -o $@ -ljson-c -lcharles_log -lcharles_server

%.o:%.cpp
	$(cc) $(include) $(cppflags) $^ -c -o $@ 

%.o:%.c
	$(cc) $(include) $(cflags) $^ -c -o $@

clean:
	-rm -f $(src) $(bin) $(so) $(lnso)
