CC = g++
NVCC = nvcc

ADDFLAGS = -L/usr/local/cuda/cuda-8.0/lib64 -L/usr/local/lib
CFLAGS =	-I/usr/include -I/usr/local/cuda/cuda-8.0/include -g -Wall -std=c++11
NCFLAGS = -std=c++11 -arch=sm_61
LIBS = -pthread -lboost_system -ljsoncpp -ldlib -lcurl
LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs
chatserver: utils.o chat_server.o base64.o
	g++ $(ADDFLAGS) $(CFLAGS) $^ -o $@ $(LIBS)

chat_server.o : chat_server.cpp
	g++ -std=c++11 -c $^

utils.o : ../utils.cpp
	g++ -std=c++11 -c $^

base64.o : ../base64.cpp
	g++ -std=c++11 -c $^

# bucket_sort2d.o : bucket_sort2d.cu
# 	$(NVCC) $(NCFLAGS) -c $^

clean:
	rm -f chatserver *.o *~ 
