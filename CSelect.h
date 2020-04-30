#ifndef CSelect_H
#define CSelect_H


#include <sys/select.h>
#include <sys/time.h>
#include "SocketOps.h"
#include <vector>

#define CLIENT_SIZE 50

class CSelect
{
private:
	int maxfd;
	fd_set read_set;  
    fd_set ready_set;
    int nready;
    int listenfd_;
    std::vector<int> clientfd;
public:
	CSelect():clientfd(FD_SETSIZE, -1)
	{}

	void init(int listenfd)
	{
		FD_ZERO(&read_set);

		listenfd_ = listenfd;
		maxfd = listenfd;
		FD_SET(listenfd, &read_set);
	}

	std::vector<int> run()
	{
		struct sockaddr_storage clientaddr;
		socklen_t clientlen;
		int connfd;
		char hostname[MAXLINE], port[MAXLINE];

		ready_set = read_set;
    	nready = Select(maxfd+1, &ready_set, NULL, NULL, NULL);

	    if (FD_ISSET(listenfd_, &ready_set)){
	    	nready--;
	        clientlen = sizeof(clientaddr);
	        connfd = Accept(listenfd_, (SA *)&clientaddr, &clientlen);

	     //    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
		    // printf("Accepted connection from (%s, %s)\n", hostname, port);
	        addClient(connfd);
	    }
	
    	return checkClient();
	}

private:
	void addClient(int connfd)
	{
		FD_SET(connfd, &read_set);
		int i;
		for(i = 0; i < FD_SETSIZE; i++)
		{
			if(clientfd[i] == -1)
			{
				clientfd[i] = connfd;
				break;
			}
		}

		if(i == FD_SETSIZE)
			app_error("add_client error: Too many clients");

		if(connfd > maxfd)
			maxfd = connfd;
	}

	std::vector<int> checkClient()
	{

		std::vector<int> tmp;

		for (int i = 0; (i < FD_SETSIZE) && (nready > 0); i++) 
		{
			int connfd = clientfd[i];

			if ((connfd > 0) && (FD_ISSET(connfd, &ready_set))) 
			{ 
			    nready--;
			   
			    tmp.push_back(connfd);
			    
				FD_CLR(connfd, &read_set); 
				clientfd[i] = -1;          
		    }
		}

		return tmp;
	}

};

#endif