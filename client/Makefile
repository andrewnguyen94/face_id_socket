ADDFLAGS = -L/usr/local/lib
CFLAGS =	-I/usr/include -g -Wall -std=c++11
LIBS = -pthread -lboost_system -ljsoncpp -lboost_thread
LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs
chatclient: chat_client.o base64.o
	g++ $(ADDFLAGS) $(CFLAGS) $^ -o $@ $(LIBS)

chat_client.o: chat_client.cpp
	g++ -std=c++11 -c $^

base64.o : ../base64.cpp
	g++ -std=c++11 -c $^

clean:
	rm -f chatclient *.o *~ 
