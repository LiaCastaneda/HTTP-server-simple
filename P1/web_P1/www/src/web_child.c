#define MAXN 16384 /* max # bytes client can request */
3 void
4 web_child(int sockfd)
5 {
6 int ntowrite;
7 ssize_t nread;
8 char line[MAXLINE], result[MAXN];
9 for ( ; ; ) {
10 if ( (nread = Readline(sockfd, line, MAXLINE)) == 0)
11 return; /* connection closed by other end */
12 /* line from client specifies #bytes to write back */
13 ntowrite = atol(line);
14 if ((ntowrite <= 0) || (ntowrite > MAXN))
15 err_quit("client request for %d bytes", ntowrite);
16 Writen(sockfd, result, ntowrite);
17 }
18 }