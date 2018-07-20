CFLAGS =	-I/usr/include -g -Wall -std=c++11
LIBS = -pthread -lboost_system -ljsoncpp -ldlib -lcurl
chatserver: chat_server.o
	g++ $(CFLAGS) $^ -o $@ $(LIBS)

chat_server.o : chat_server.cpp
	g++ -std=c++11 -c $^

clean:
	rm -f chatserver *.o *~ 
