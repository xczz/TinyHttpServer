#ifndef TASK_H
#define TASK_H

#include "SocketOps.h"

class Task
{
private:
	int connd_;

public:
	Task(int connd):connd_(connd){}


	void doit();

	int getConnd();
private:
	int read_requesthdrs(rio_t *rp, char *method);
	int parse_uri(char *uri, char *filename, char *cgiargs);
	void serve_static(char *filename, int filesize, char *method);
	void get_filetype(char *filename, char *filetype);
	void serve_dynamic(char *filename, char *cgiargs, char *method);
	void clienterror(char *cause, const char *errnum, 
			 const char *shortmsg, const char *longmsg);
};


#endif