#ifdef _cplusplus
extern "C" {
#endif
 
#include <stdio.h>
#include <hiredis/hiredis.h>
 
int main()
{
	redisContext * conn = redisConnect("127.0.0.1", 6379);
	if (conn->err)
	{
            printf("connection error\n");
            redisFree(conn);
            return 0;
     }
 
	redisReply * reply = (redisReply*)redisCommand(conn, "set foo '1234'");
	freeReplyObject(reply);
 
	reply = (redisReply*)redisCommand(conn, "get foo");
 
	printf("%s\n", reply->str);
	freeReplyObject(reply);
 
	redisFree(conn);
}
 
#ifdef _cplusplus
}
#endif
