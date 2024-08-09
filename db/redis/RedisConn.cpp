#include "db/redis/RedisConn.h"

#include <sys/time.h>
#include <math.h>
#include <queue>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "base/log/Logging.h"


#define REDIS_NORMAL_JUDGE(reply) 					    \
if (nullptr == reply)									            \
{													                      \
	return RedisConn::RedisCode::kNetworkError;		\
}													                      \
if (REDIS_REPLY_ERROR == reply->type)				    \
{													                      \
	freeReplyObject(reply);							          \
	return RedisConn::RedisCode::kNormalError;    \
}													                      \


#ifdef __cplusplus
extern "C" {
#endif
int __redisAppendCommand(redisContext *c, const char *cmd, size_t len);
#ifdef __cplusplus
}
#endif



namespace RedisConn
{
  namespace detail
  {
    /* Calculate the number of bytes needed to represent an integer as string. */
    static int intlen(int i) {
      int len = 0;
      if (i < 0) {
        len++;
        i = -i;
      }
      do {
        len++;
        i /= 10;
      } while (i);
      return len;
    }

    /* Helper that calculates the bulk length given a certain string length. */
    static size_t bulklen(size_t len) {
      return 1 + intlen(len) + 2 + len + 2;
    }
  } // namespace detail
  

	RedisConnection::RedisConnection()
	{
		port_ = 0;
		timeoutMs = 0;
		ctx_ = nullptr;
	}

	RedisConnection::~RedisConnection()
	{
		if(ctx_ != nullptr) {
			redisFree(ctx_);
			ctx_ = nullptr;
		}
	}

	/*************************************************
	Function:  connectSvr
	Description: 连接redis服务
	Input: 
		ip：例 "192.168.1.2"
		port: 端口号 例 6379
		timeout:设置超时时间，单位 ms
	Output: 无
	Return: 成功返回true
	Others: 无
	*************************************************/
	bool RedisConnection::connectSvr(const char *ip, int port, unsigned int timeout)
	{
		host_ = ip;
		port_ = port;
		timeoutMs = timeout;
		if(ctx_ == nullptr) {
			return connectSvr(&ctx_, ip, port, timeout);
		}
		return true;
	}

	/*************************************************
	Function:  disconnectSvr
	Description: 断开原有的redis连接
	Input:无
	Output: 无
	Return: 无
	Others: 无
	*************************************************/
	void RedisConnection::disconnectSvr()
	{
		redisFree(ctx_);
		ctx_ = nullptr;
	}

	/*************************************************
	Function:  asynSave
	Description: fork一个子进程，同步数据到磁盘，立即返回
	Input:无
	Output: 无
	Return: 成功返回0
	Others: 需使用 LASTSAVE 命令查看同步结果
	*************************************************/
	int RedisConnection::asynSave()
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("BGSAVE");
		REDIS_NORMAL_JUDGE(reply);

		freeReplyObject(reply);
		return 0;
	}

	/*************************************************
	Function:  save
	Description: 阻塞方式 同步数据到磁盘
	Input:无
	Output: 无
	Return: 成功返回0
	Others: 当key较多时，很慢
	*************************************************/
	int RedisConnection::save()
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("SAVE");
		REDIS_NORMAL_JUDGE(reply);

		if (0 == strncasecmp("ok", reply->str, 3))
		{
			freeReplyObject(reply);
			return 0;
		}

		freeReplyObject(reply);
		
		return RedisCode::kNormalError;
	}

	/*************************************************
	Function:  command
	Description: redis原始接口,若失败，会尝试一次重连
	Input:格式化参数，接受可变参数
	Output: 无
	Return: 执行结果，一般为 redisReply*
	Others: 无
	*************************************************/
	void* RedisConnection::command(const char *format, ...)
	{
		va_list ap;
		void *reply = nullptr;
		LOG_INFO << ctx_ ;
		if (ctx_)
		{
			va_start(ap, format);
			reply = redisvCommand(ctx_, format, ap);
			va_end(ap);
		}

		if (nullptr == reply)
		{
			if (reConnectSvr())
			{
				va_start(ap, format);
				reply = redisvCommand(ctx_, format, ap);
				va_end(ap);
			}
		}

		return reply;
	}

	void* RedisConnection::commandArgv(const std::vector<std::string> &vstrOper, const std::vector<std::string> &vstrParam)
	{
		char *cmd = nullptr;
		int len = 0;
		void *reply = nullptr;
		int pos;
		int totlen = 0;

		//格式化命令
		/* Calculate number of bytes needed for the command */
		totlen = 1 + detail::intlen(vstrOper.size() + vstrParam.size()) + 2;
		for (unsigned int j = 0; j < vstrOper.size(); j++)
		{
			totlen += detail::bulklen(vstrOper[j].length());
		}
		for (unsigned int j = 0; j < vstrParam.size(); j++)
		{
			totlen += detail::bulklen(vstrParam[j].length());
		}

		/* Build the command at protocol level */
		cmd = (char *)malloc(totlen + 1);
		if (cmd == nullptr)
			return nullptr;
		
		pos = sprintf(cmd, "*%zu\r\n", vstrOper.size() + vstrParam.size());
		//push cmd
		for (unsigned int j = 0; j < vstrOper.size(); j++) {
			pos += sprintf(cmd + pos, "$%zu\r\n", vstrOper[j].length());
			memcpy(cmd + pos, vstrOper[j].c_str(), vstrOper[j].length());
			pos += vstrOper[j].length();
			cmd[pos++] = '\r';
			cmd[pos++] = '\n';
		}

		//push param
		for (unsigned int j = 0; j < vstrParam.size(); j++) {
			pos += sprintf(cmd + pos, "$%zu\r\n", vstrParam[j].length());
			memcpy(cmd + pos, vstrParam[j].c_str(), vstrParam[j].length());
			pos += vstrParam[j].length();
			cmd[pos++] = '\r';
			cmd[pos++] = '\n';
		}
		assert(pos == totlen);
		cmd[pos] = '\0';

		len = totlen;
		//执行命令
		reply = execCommand(cmd, len);
		if (nullptr == reply)
		{
			if (reConnectSvr())
			{
				reply = execCommand(cmd, len);
			}
		}

		free(cmd);
		return reply;
	}

	void* RedisConnection::execCommand(const char *cmd, int len)
	{
		void *reply = nullptr;
		if (nullptr == ctx_)
		{
			return nullptr;
		}

		//执行命令
		if (__redisAppendCommand(ctx_, cmd, len) != REDIS_OK)
		{
			return nullptr;
		}

		//获取执行结果
		if (ctx_->flags & REDIS_BLOCK)
		{
			if (redisGetReply(ctx_, &reply) != REDIS_OK)
			{
				return nullptr;
			}
		}

		return reply;
	}

	/*************************************************
	Function:  setKey
	Description: 设置一对key value,若已存在直接覆盖
	Input:
		lifeTime:生存时间(秒)，0代表永不删除 
	Output: 无
	Return: 成功返回0
	Others: 无
	*************************************************/
	int RedisConnection::setKeyVal(const std::string& key, const std::string& value, unsigned int lifeTime)
	{
		redisReply *reply = nullptr;
		if (0 == lifeTime)
		{
			reply = (redisReply *)command("SET %s %b", key.c_str(), value.c_str(), value.size());
			// reply = (redisReply *)command("SET %s %s", key, value);

		}
		else
		{
			reply = (redisReply *)command("SET %s %b EX %u", key.c_str(), value, value.size(), lifeTime);
		}
		REDIS_NORMAL_JUDGE(reply);

		freeReplyObject(reply);
		return 0;
	}

	/*************************************************
	Function:  setKeyRange
	Description: 用 value 参数覆写(overwrite)给定 key 所储存的字符串值，从偏移量 offset 开始。
	Input:
		offset: 偏移量，下标从0开始
	Output: 无
	Return: 修改后value的长度
	Others: 如果给定 key 原来储存的字符串长度比偏移量小，那么原字符和偏移量之间的空白将用零字节("\x00" )来填充。
	*************************************************/
	int RedisConnection::setKeyRange(const std::string& key, const std::string& value, int offset)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("SETRANGE %s %u %b", key.c_str(), offset, value, value.size());
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->integer;
		freeReplyObject(reply);

		return len;
	}

	/*************************************************
	Function:  append
	Description: 在原有的key追加，若key不存在则相当于setKey
	Input:
	offset: 偏移量，下标从0开始
	Output: 无
	Return: 修改后value的长度
	Others: 如果给定 key 原来储存的字符串长度比偏移量小，那么原字符和偏移量之间的空白将用零字节("\x00" )来填充。
	*************************************************/
	int RedisConnection::append(const std::string& key, const std::string& value)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("APPEND %s %b", key.c_str(), value, value.size());
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->integer;
		freeReplyObject(reply);

		return len;
	}

	/*************************************************
	Function:  setKeyLifeTime
	Description: 设置key的生存时间
	Input:
		time:单位秒，0代表永不删除
	Output: 无
	Return: 成功返回0
	Others: 无
	*************************************************/
	int RedisConnection::setKeyLifeTime(const std::string& key, unsigned int time)
	{
		redisReply *reply = nullptr;
		if (0 == time)
		{
			reply = (redisReply *)command("PERSIST %s", key.c_str());
		}
		else
		{
			reply = (redisReply *)command("EXPIRE %s %u", key.c_str(), time);
		}
		REDIS_NORMAL_JUDGE(reply);

		int ret = 0;
		freeReplyObject(reply);

		return ret;
	}


	/*************************************************
	Function:  getKey
	Description: 获取key对应的value
	Input:
		valueLen:value的size
	Output: 
		value:用于接收 key对应的value，若存储空间不够，返回失败
	Return: 返回长度，0不存在
	Others: 无
	*************************************************/
	int RedisConnection::getKey(const std::string& key, std::string& value)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("GET %s", key.c_str());
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->len;

		value.append(reply->str, len);
		// memcpy(value, reply->str, len);
		freeReplyObject(reply);

		return len;
	}

	/*************************************************
	Function:  getLen
	Description: 获取key对应的value的长度
	Input:
	Output:
	Return: 返回长度，0代表不存在
	Others: 无
	*************************************************/
	int RedisConnection::getLen(const std::string& key)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("STRLEN %s", key.c_str());
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->integer;
		freeReplyObject(reply);

		return len;
	}


	/*************************************************
	Function:  getKeyByRange
	Description: 获取key对应start end区间的数据
	Input:
		start:-1代表最后1个，-2代表倒数第2个.
		end: -1代表最后1个，-2代表倒数第2个
		valueLen:value的size
	Output:
		value：获取的数据
	Return: 返回长度，0代表key不存在或end在start前
	Others: 通过保证子字符串的值域(range)不超过实际字符串的值域来处理超出范围的值域请求。
	*************************************************/
	int RedisConnection::getKeyByRange(const std::string& key, int start, int end, std::string& value)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("GETRANGE %s %d %d", key.c_str(), start, end);
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->len;

		// if (len > valueLen)
		// {
		// 	len = RedisCode::kSpaceNotEnough;
		// }
		// else
		// {
		// 	memcpy(value, reply->str, len);
		// }
		value.append(reply->str, len);
		freeReplyObject(reply);

		return len;
	}

	/*************************************************
	Function:  getKeyRemainLifeTime
	Description: 获取key的剩余时间
	Input:
	Output:
	Return: -2:key不存在，-1:永久，其它则返回剩余时间(秒)
	Others: 无
	*************************************************/
	int RedisConnection::getKeyRemainLifeTime(const std::string& key)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("TTL %s", key.c_str());
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->integer;
		freeReplyObject(reply);

		return len;
	}

	/*************************************************
	Function:  getKeyType
	Description: 获取key的类型
	Input:
		valueLen:valueType的size
	Output:
		valueType: none(key不存在) string(字符串)	list(列表)	set(集合) zset(有序集) hash(哈希表)
	Return: valueType size
	Others: 无
	*************************************************/
	int RedisConnection::getKeyType(const std::string& key, std::string& valueType)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("TYPE %s", key.c_str());
		REDIS_NORMAL_JUDGE(reply);

		int len = reply->len;
		// if (reply->len + 1 > valueLen)
		// {
		// 	len = RedisCode::kSpaceNotEnough;
		// }
		// else
		// {
		// 	memcpy(valueType, reply->str, reply->len + 1);
		// }
		valueType.append(reply->str, len + 1);		//????
		freeReplyObject(reply);

		return len;
	}

	/*************************************************
	Function:  delKey
	Description: 删除key
	Input:
	Output:
	Return: 删除成功返回1，不存在返回0
	Others: 无
	*************************************************/
	int RedisConnection::delKey(const std::string& key)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("DEL %s", key.c_str());
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	/*************************************************
	Function:  hasKey
	Description: 判断key是否存在
	Input:
	Output:
	Return: key存在返回1，不存在返回0
	Others: 无
	*************************************************/
	int RedisConnection::hasKey(const std::string& key)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("EXISTS %s", key.c_str());
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	int RedisConnection::incrByInt(const std::string& key, int addValue, int &retValue)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("INCR %s %d", key.c_str(), addValue);
		REDIS_NORMAL_JUDGE(reply);

		retValue = atoi(reply->str);
		freeReplyObject(reply);

		return 0;
	}

	/*************************************************
	Function:  incrByFloat
	Description: 对key做加法
	Input:
		addValue：需要相加的值。（可为负数）
	Output:
		retValue：用于接收相加后的结果
	Return: 执行成功返回0
	Others: 如果 key 不存在，那么会先将 key 的值设为 0 ，再执行加法操作。
	*************************************************/
	int RedisConnection::incrByFloat(const std::string& key, double addValue, double &retValue)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("INCRBYFLOAT %s %f", key.c_str(), addValue);
		REDIS_NORMAL_JUDGE(reply);

		retValue = atof(reply->str);
		freeReplyObject(reply);

		return 0;
	}

	//返回0 成功
	int  RedisConnection::setMultiKey(const std::vector<std::string> &vstrKeyValue)
	{
		redisReply *reply = nullptr;
		std::vector<std::string> vstrOper;
		vstrOper.push_back(std::string("MSET"));
		reply = (redisReply *)commandArgv(vstrOper, vstrKeyValue);
		REDIS_NORMAL_JUDGE(reply);

		freeReplyObject(reply);
		return 0;
	}

	int RedisConnection::getMultiKey(const std::vector<std::string> &vstrKey, std::vector<std::string> &vstrValue)
	{
		redisReply *reply = nullptr;
		std::vector<std::string> vstrOper;
		vstrOper.push_back(std::string("MGET"));
		reply = (redisReply *)commandArgv(vstrOper, vstrKey);
		REDIS_NORMAL_JUDGE(reply);
		//获取结果
		vstrValue.clear();
		for (unsigned int i = 0; i < reply->elements; i++)
		{
			vstrValue.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
		}

		freeReplyObject(reply);
		return 0;
	}

	int RedisConnection::delMultiKey(const std::vector<std::string> &vstrKey)
	{
		redisReply *reply = nullptr;
		std::vector<std::string> vstrOper;
		vstrOper.push_back(std::string("DEL"));
		reply = (redisReply *)commandArgv(vstrOper, vstrKey);
		REDIS_NORMAL_JUDGE(reply);

		int len = reply->integer;
		freeReplyObject(reply);
		return len;
	}

	int RedisConnection::setHField(const std::string& key, const std::string& field, const std::string& value)
	{    
		redisReply *reply = nullptr;
		reply = (redisReply *)command("HSET %b %b %b", key, key.size(), field, field.size(), value, value.size());
		REDIS_NORMAL_JUDGE(reply);

		freeReplyObject(reply);

		return 0;
	}

	int RedisConnection::getHField(const std::string& key, const std::string& field, std::string& value)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("HGET %b %b", key, key.size(), field, field.size());
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->len;
		// if (len > valueLen) //空间不够
		// {
		// 	len = RedisCode::kSpaceNotEnough;
		// }
		// else
		// {
		// 	memcpy(value, reply->str, len);
		// }
		value.append(std::string(reply->str, len));
		freeReplyObject(reply);

		return len;
	}

	int RedisConnection::delHField(const std::string& key, const std::string& field)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("HDEL %b %b", key, key.size(), field, field.size());
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}


	int RedisConnection::hasHField(const std::string& key, const std::string& field)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("HEXISTS %b %b", key, key.size(), field, field.size());
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	int RedisConnection::incrHByFloat(const std::string& key, const std::string& field, double addValue, double& retValue)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("HINCRBYFLOAT %b %b %f", key, key.size(), field, field.size(), addValue);
		REDIS_NORMAL_JUDGE(reply);

		retValue = atof(reply->str);
		freeReplyObject(reply);

		return 0;
	}

	int RedisConnection::incrHByInt(const std::string& key, const std::string& field, int addValue, int& retValue)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("HINCRBY %b %b %d", key, key.size(), field, field.size(), addValue);
		REDIS_NORMAL_JUDGE(reply);

		retValue = atoi(reply->str);
		freeReplyObject(reply);

		return 0;
	}

	int RedisConnection::getHAll(const std::string& key, std::vector<std::string> &vstrFieldValue)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("HGETALL %b", key, key.size());
		REDIS_NORMAL_JUDGE(reply);
		//获取结果
		vstrFieldValue.clear();
		for (unsigned int i = 0; i < reply->elements; i++)
		{
			vstrFieldValue.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
		}

		freeReplyObject(reply);
		return 0;
	}

	int RedisConnection::getHFieldCount(const std::string& key)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("HLEN %b", key, key.size());
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	int RedisConnection::setMultiHField(const std::string& key, const std::vector<std::string> &vstrFieldValue)
	{
		redisReply *reply = nullptr;
		std::vector<std::string> vstrOper;
		vstrOper.push_back(std::string("HMSET"));
		vstrOper.push_back(key);
		reply = (redisReply *)commandArgv(vstrOper, vstrFieldValue);
		REDIS_NORMAL_JUDGE(reply);

		freeReplyObject(reply);
		return 0;
	}

	int RedisConnection::getMultiHField(const std::string& key, const std::vector<std::string> &vstrField, std::vector<std::string> &vstrValue)
	{
		redisReply *reply = nullptr;
		std::vector<std::string> vstrOper;
		vstrOper.push_back(std::string("HMGET"));
		vstrOper.push_back(key);
		reply = (redisReply *)commandArgv(vstrOper, vstrField);
		REDIS_NORMAL_JUDGE(reply);
		//获取结果
		vstrValue.clear();
		for (unsigned int i = 0; i < reply->elements; i++)
		{
			vstrValue.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
		}

		freeReplyObject(reply);
		return 0;
	}

	int RedisConnection::delMultiHField(const std::string& key, const std::vector<std::string> &vstrField)
	{
		redisReply *reply = nullptr;
		std::vector<std::string> vstrOper;
		vstrOper.push_back(std::string("HDEL"));
		vstrOper.push_back(key);
		reply = (redisReply *)commandArgv(vstrOper, vstrField);
		REDIS_NORMAL_JUDGE(reply);

		int len = reply->integer;
		freeReplyObject(reply);
		return len;
	}


	int RedisConnection::lpushList(const std::string& key, const std::string& value)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("LPUSH %b %b", key, key.size(), value, value.size());
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	//返回value长度
	int RedisConnection::lpopList(const std::string& key, std::string& value)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("LPOP %b", key, key.size());
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->len;
		// if (len > valueLen) //空间不够
		// {
		// 	len = RedisCode::kSpaceNotEnough;
		// }
		// else
		// {
		// 	memcpy(value, reply->str, len);
		// }
		value.append(std::string(reply->str, len));
		freeReplyObject(reply);

		return len;
	}

	int RedisConnection::rpushList(const std::string& key, const std::string& value)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("RPUSH %b %b", key, key.size(), value, value.size());
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	//返回value长度
	int RedisConnection::rpopList(const std::string& key, std::string& value)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("RPOP %b", key, key.size());
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->len;
		// if (len > valueLen) //空间不够
		// {
		// 	len = RedisCode::kSpaceNotEnough;
		// }
		// else
		// {
		// 	memcpy(value, reply->str, len);
		// }
		value.append(std::string(reply->str, len));
		freeReplyObject(reply);

		return len;
	}


	int RedisConnection::indexList(const std::string& key, int index, std::string& value)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("LINDEX %b %d", key, key.size(), index);
		REDIS_NORMAL_JUDGE(reply);

		int len = (int)reply->len;
		// if (len > valueLen) //空间不够
		// {
		// 	len = RedisCode::kSpaceNotEnough;
		// }
		// else
		// {
		// 	memcpy(value, reply->str, len);
		// }
		value.append(std::string(reply->str, len));
		freeReplyObject(reply);

		return len;
	}

	int RedisConnection::lenList(const std::string& key)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("LLEN %b", key, key.size());
		REDIS_NORMAL_JUDGE(reply);

		int ret = reply->integer;
		freeReplyObject(reply);

		return ret;
	}

	int RedisConnection::rangeList(const std::string& key, int start, int end, std::vector<std::string> &vstrList)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("LRANGE %b %d %d", key, key.size(), start, end);
		REDIS_NORMAL_JUDGE(reply);
		//获取结果
		vstrList.clear();
		for (unsigned int i = 0; i < reply->elements; i++)
		{
			vstrList.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
		}
		freeReplyObject(reply);

		return 0;
	}

	int RedisConnection::setList(const std::string& key, int index, const std::string& value)
	{
		redisReply *reply = nullptr;
		reply = (redisReply *)command("LSET %b %d %b", key, key.size(), index, value, value.size());
		REDIS_NORMAL_JUDGE(reply);

		return 0;
	}

	bool RedisConnection::reConnectSvr()
	{
		disconnectSvr();
		return connectSvr(host_.c_str(), port_, timeoutMs);
	}


	bool RedisConnection::connectSvr(redisContext** ctx, const char* ip, int port, unsigned int timeout)
	{
		struct timeval tvTimeout;
		tvTimeout.tv_sec = timeout / 1000;
		tvTimeout.tv_usec = (timeout % 1000) * 1000;
		*ctx = redisConnectWithTimeout(ip, port, tvTimeout);

		if(*ctx == nullptr || (*ctx)->err) {
				if(*ctx) {
						disconnectSvr(ctx);
						LOG_ERROR << "RedisConnection::connect error. ip:" << ip << " , port: " << port << ", error: " << ctx_->errstr;
				}
				else {
						LOG_ERROR << "RedisConnection::connect failed! can't allocate redis context";
				}
				return false;
		}
		LOG_INFO << "RedisConnection::connect success. ip:" << ip << " , port: " << port;

		return true;
  }

	void RedisConnection::disconnectSvr(redisContext** ctx)
	{
		redisFree(*ctx);
		(*ctx) = nullptr;
	}

}

