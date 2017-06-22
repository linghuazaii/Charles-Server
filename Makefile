include=-I./depend/include \
		-I./depend/include/json-c

link=-L./depend/lib \
	 -ljson-c \
	 -lcharles_log \
	 -pthread

cc=g++
cppflags=-g -O0 -std=c++11
cflags=-O3

src=charles_server.o main.o threadpool.o
bin=server

all:$(bin)

$(bin):$(src)
	$(cc) $^ -o $@ $(link)

%.o:%.cpp
	$(cc) $(include) $(cppflags) $^ -c -o $@ 

%.o:%.c
	$(cc) $(include) $(cflags) $^ -c -o $@

clean:
	-rm -f $(src) $(bin)
