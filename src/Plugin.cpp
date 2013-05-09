//g++ -std=c++0x -pthread echosrv.cpp -o echosrv
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <unistd.h>
#include "localmutex.h"
using namespace std;

int gRunning = 1;
int gPaused = 1;
Mutex gRunningMutex;

class Worker
{
public:

    void operator()() const
    {
        cout << "#Type RUN and press enter to start the algorithm" << endl;

		int lastPaused = 1;
        float progress = 0.f;
		gRunningMutex.lock();
		int running = gRunning;
		int paused = gPaused;
		gRunningMutex.unlock();
		while(running)
		{
			if(paused != lastPaused)
			{
				if(paused)
					cout << "NOW_PAUSED" << endl;
				else
					cout << "NOW_RUNNING" << endl;
			}
			lastPaused = paused;

			if(!paused)
			{
                usleep(100000);
                progress += 0.01;
			}
			else
			{
				usleep(1000);
			}

            //Check if finished
            if(progress >= 1.f)
            {
                progress = 1.f;
                gRunningMutex.lock();
                gRunning = 0;
                gRunningMutex.unlock();
            }

            if(!paused)
                cout << "PROGRESS=" << progress << endl;

			gRunningMutex.lock();
			running = gRunning;
			paused = gPaused;
			gRunningMutex.unlock();
		}
		
        cout << "FINISHED" << endl;
        exit(0);
    }


};

static void* StartWorker(void *data)
{
	class Worker worker;
	worker();
}

int main(int argc, char *argv[])
{
    pthread_attr_t workerSettings;
    pthread_t worker;
	pthread_attr_init(&workerSettings);
	pthread_attr_setdetachstate(&workerSettings, PTHREAD_CREATE_JOINABLE);
	pthread_create(&worker, &workerSettings, StartWorker, NULL);

	gRunningMutex.lock();
	int running = gRunning;
	gRunningMutex.unlock();

    ofstream log("log.txt");

	while(running)
	{
        char buff[1024*100];

		//Read standard input
        cin.getline(buff, 1024*100);
        std::string mystring(buff);
        if(mystring.length()>0)
        {
            log << mystring << endl;
            log.flush();
        }

		//Process commands
		if(mystring == "QUIT")
		{
			gRunningMutex.lock();
			gRunning = 0;
			gRunningMutex.unlock();
		}

		if(mystring == "PAUSE")
		{
			gRunningMutex.lock();
			gPaused = 1;
			gRunningMutex.unlock();
		}

		if(mystring == "RUN")
		{
			gRunningMutex.lock();
			gPaused = 0;
			gRunningMutex.unlock();
		}

        if(mystring.substr(0,11) == "DATA_BLOCK=")
        {
            //Read text arguments
            char tmp[1024*100];
            cin.getline(tmp, 1024*100);
            log << tmp << endl;

            int len = atoi(mystring.substr(11).c_str());
            log << "expect " << len << endl;
            char *dataBlock = new char[len];

            cin.read(dataBlock, len);
            delete [] dataBlock;
            log << "final buffer len " << len << endl;
            //log << buff;
        }

		//if(mystring == "GET_PROGRESS")
		//	cout << "PROGRESS=0.5" << endl;

		gRunningMutex.lock();
		running = gRunning;
		gRunningMutex.unlock();
	}
	
	pthread_join(worker, NULL); //This waits until worker thread finishes
}

