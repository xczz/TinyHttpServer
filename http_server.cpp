// #include "ThreadPool.h"
#include "Task.h"
#include "ThreadPool.h"
#include <iostream>
#include "CSelect.h"

using namespace std;


int main(int argc, char const *argv[])
{
	
	int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;


    ThreadPool<Task> pool(4);
    pool.start();


    CSelect sel;

    
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }


    listenfd = Open_listenfd(argv[1]);

    sel.init(listenfd);

    while (1) 
    {
        vector<int> tmp;

        tmp = sel.run();

        if(!tmp.empty())
        {
            for(int i = 0; i < tmp.size(); ++i)
            {
                Task *task = new Task(tmp[i]);
                pool.addTask(task);
            }
        }                                        //line:netp:tiny:close

	}
}