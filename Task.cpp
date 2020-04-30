#include "Task.h"



void Task::doit()
{
	int is_static, cont_len;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, connd_);
    if (!Rio_readlineb(&rio, buf, MAXLINE))
    {
        return;
    }
    // printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       
    if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD") && strcasecmp(method, "POST")) {                     //line:netp:doit:beginrequesterr
        clienterror(method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }                                                    //line:netp:doit:endrequesterr
    cont_len = read_requesthdrs(&rio, method);                              //line:netp:doit:readrequesthdrs

    if(strcasecmp(method, "POST") == 0){
        Rio_readlineb(&rio, buf, cont_len+1);
        printf("%s\n", buf);
    }

    /* Parse URI from GET request */
    is_static = parse_uri(uri, filename, cgiargs);       //line:netp:doit:staticcheck
    if (stat(filename, &sbuf) < 0) {                     //line:netp:doit:beginnotfound
	clienterror(filename, "404", "Not found",
		    "Tiny couldn't find this file");

    // printf("hh\n");
	return;
    }                                                    //line:netp:doit:endnotfound

    if (is_static) { /* Serve static content */          
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { //line:netp:doit:readable
	    clienterror(filename, "403", "Forbidden",
			"Tiny couldn't read the file");
	    return;
	}
	serve_static(filename, sbuf.st_size, method);        //line:netp:doit:servestatic
    }
    else { /* Serve dynamic content */
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { //line:netp:doit:executable
	    clienterror(filename, "403", "Forbidden",
			"Tiny couldn't run the CGI program");
	    return;
	}
    if(strcasecmp(method, "POST")){
        serve_dynamic(filename, cgiargs, method); 
    }
    else{
        serve_dynamic(filename, buf, method); 
    }
	
    }

}



int Task::getConnd()
{
    return connd_;
}
/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
int Task::read_requesthdrs(rio_t *rp, char *method) 
{
    char buf[MAXLINE];
    int len = 0;

    Rio_readlineb(rp, buf, MAXLINE);
    // printf("%s", buf);
    
    while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
        Rio_readlineb(rp, buf, MAXLINE);
        // printf("%s", buf);
        if((strncasecmp(buf, "Content-Length:", 15) == 0) && (strcasecmp(method, "POST") == 0)){
            sscanf(buf, "Content-Length: %d", &len);
        }
    }

    return len;
}


/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int Task::parse_uri(char *uri, char *filename, char *cgiargs) 
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */ //line:netp:parseuri:isstatic
	strcpy(cgiargs, "");                             //line:netp:parseuri:clearcgi
	strcpy(filename, ".");                           //line:netp:parseuri:beginconvert1
	strcat(filename, uri);                           //line:netp:parseuri:endconvert1
	if (uri[strlen(uri)-1] == '/')                   //line:netp:parseuri:slashcheck
	    strcat(filename, "home.html");               //line:netp:parseuri:appenddefault
	return 1;
    }
    else {  /* Dynamic content */                        //line:netp:parseuri:isdynamic
	ptr = index(uri, '?');                           //line:netp:parseuri:beginextract
	if (ptr) {
	    strcpy(cgiargs, ptr+1);
	    *ptr = '\0';
	}
	else 
	    strcpy(cgiargs, "");                         //line:netp:parseuri:endextract
	strcpy(filename, ".");                           //line:netp:parseuri:beginconvert2
	strcat(filename, uri);                           //line:netp:parseuri:endconvert2
	return 0;
    }
}
/* $end parse_uri */

/*
 * serve_static - copy a file back to the client 
 */
/* $begin serve_static */
void Task::serve_static(char *filename, int filesize, char *method)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename, filetype);    //line:netp:servestatic:getfiletype
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); //line:netp:servestatic:beginserve
    Rio_writen(connd_, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(connd_, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n", filesize);
    Rio_writen(connd_, buf, strlen(buf));
    sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(connd_, buf, strlen(buf));    //line:netp:servestatic:endserve

    if(strcasecmp(method, "HEAD") == 0)
        return;
    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0); //line:netp:servestatic:open
    srcp = (char*)Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); //line:netp:servestatic:mmap
    Close(srcfd);                       //line:netp:servestatic:close
    Rio_writen(connd_, srcp, filesize);     //line:netp:servestatic:write
    Munmap(srcp, filesize);             //line:netp:servestatic:munmap


}

/*
 * get_filetype - derive file type from file name
 */
void Task::get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
	strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mpeg"))
    strcpy(filetype, "video/mpg");
    else
	strcpy(filetype, "text/plain");
}  
/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void Task::serve_dynamic(char *filename, char *cgiargs, char *method) 
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    Rio_writen(connd_, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(connd_, buf, strlen(buf));
  
    if (Fork() == 0) { /* Child */ //line:netp:servedynamic:fork
	/* Real server would set all CGI vars here */
	setenv("QUERY_STRING", cgiargs, 1); //line:netp:servedynamic:setenv
    setenv("method", method, 1);
	Dup2(connd_, STDOUT_FILENO);         /* Redirect stdout to client */ //line:netp:servedynamic:dup2
	Execve(filename, emptylist, environ); /* Run CGI program */ //line:netp:servedynamic:execve
    }
    Wait(NULL); /* Parent waits for and reaps child */ //line:netp:servedynamic:wait

}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void Task::clienterror(char *cause, const char *errnum, 
		 const char *shortmsg, const char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(connd_, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(connd_, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(connd_, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(connd_, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(connd_, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(connd_, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(connd_, buf, strlen(buf));
}
/* $end clienterror */