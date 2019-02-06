scheduler:scheduler.o
	g++ scheduler.o -o scheduler

scheduler.o:scheduler.cpp
	g++ -std=c++11 scheduler.cpp -c -o scheduler.o

clean:
	rm scheduler
	rm *.o
