#pragma once

#include <hiredis/hiredis.h>
#include <string>
#include <vector>
#include "db/redis/Common.h"

#pragma GCC diagnostic ignored "-Wconversion"


/* redis 接口注意事项
 * 1、使用get接口时，当返回结果为0（长度）时，该key有可能不存在，也有可能key对应的值为空，若想确定是否为空，需调用对应的 has接口，
      除非你的业务能保证不存在非空值的value.
 * 2、使用has接口时，当返回结果 大于等于1 时，才说明该Key存在，因为当出现网络错误时会返回负数，切记不要这样if(has())，得这样使用if（has() > 0）
 */
namespace RedisConn
{

	class RedisConnection
	{
	public:
		RedisConnection();
		virtual ~RedisConnection();

		//服务器连接
		bool connectSvr(const char *ip, int port, unsigned int timeout = 1500); //timeout 单位ms
		void disconnectSvr();

		//同步数据到磁盘
		int asynSave();//异步，会立即返回
		int save();//同步，耗时长，慎用

		//redis 原始接口
		void* command(const char *format, ...);
		void* commandArgv(const std::vector<std::string> &vstrOper, const std::vector<std::string> &vstrParam);

		//redis基本操作（string)（数值类型得格式化成string）
		int setKeyVal(const std::string& key, const std::string& value, unsigned int lifeTime = 0);//lifeTime,生存时间(秒)，0代表永不删除
		int setKeyRange(const std::string& key, const std::string& value, int offset);//offset为偏移量，下标从0开始
		int append(const std::string& key, const std::string& value);//在原有的key追加，若不存在则相当于setKey
		int setKeyLifeTime(const std::string& key, unsigned int time = 0); //time 单位秒，=0代表永不删除

		int getKey(const std::string& key, std::string& value);//结果存value,返回长度，0不存在
		int getLen(const std::string& key);//返回结果：0代表不存在
		int getKeyByRange(const std::string& key, int start, int end, std::string& value);//start,end, -1代表最后1个，-2代表倒数第2个
		int getKeyRemainLifeTime(const std::string& key);//返回结果：-2:key不存在，-1:永久，其它则返回剩余时间(秒)
		int getKeyType(const std::string& key, std::string& valueType);//返回结果在valueType

		int delKey(const std::string& key);//删除成功返回1，不存在返回0
		int hasKey(const std::string& key);//key存在返回1，不存在返回0
		int incrByFloat(const std::string& key, double addValue, double &retValue);//执行成功返回0
		int incrByInt(const std::string& key, int addValue, int &retValue);//执行成功返回0

		int setMultiKey(const std::vector<std::string> &vstrKeyValue);//返回0 成功
		int getMultiKey(const std::vector<std::string> &vstrKey, std::vector<std::string> &vstrValue);//返回0 成功,结果在vstrValue中
		int delMultiKey(const std::vector<std::string> &vstrKey);//返回删除成功的个数

		//hash表
		int setHField(const std::string& key, const std::string& field, const std::string& value);//返回0 成功
		int getHField(const std::string& key, const std::string& field, std::string& value);//返回长度
		int delHField(const std::string& key, const std::string& field);//删除成功返回1，不存在返回0

		int hasHField(const std::string& key, const std::string& field);//key存在返回1，不存在返回0
		int incrHByFloat(const std::string& key, const std::string& field, double addValue, double& retValue);//执行成功返回0
		int incrHByInt(const std::string& key, const std::string& field, int addValue, int& retValue);//执行成功返回0

		int getHAll(const std::string& key, std::vector<std::string> &vstrFieldValue);//执行成功返回0
		int getHFieldCount(const std::string& key);//返回field个数

		int setMultiHField(const std::string& key, const std::vector<std::string> &vstrFieldValue);//返回0 成功
		int getMultiHField(const std::string& key, const std::vector<std::string> &vstrField, std::vector<std::string> &vstrValue);//返回0 成功
		int delMultiHField(const std::string& key, const std::vector<std::string> &vstrField);//返回删除成功的个数
		
		//列表
		int lpushList(const std::string& key, const std::string& value);//返回列表长度
		int lpopList(const std::string& key, std::string& value);//返回value长度
		int rpushList(const std::string& key, const std::string& value);//返回列表长度
		int rpopList(const std::string& key, std::string& value);//返回value长度

		int indexList(const std::string& key, int index, std::string& value);//返回value长度
		int lenList(const std::string& key);//成功长度
		int rangeList(const std::string& key, int start, int end, std::vector<std::string> &vstrList);//成功返回0
		int setList(const std::string& key, int index, const std::string& value);//成功返回0，set效率为线性的，一般只用做首尾的修改（也就是index为0或-1）

	private:
		// 禁止拷贝、赋值
		RedisConnection(const RedisConnection&);
		RedisConnection& operator =(const RedisConnection&);
		bool reConnectSvr();
		void *execCommand(const char *cmd, int len);

	protected:
    bool connectSvr(redisContext** ctx, const char* ip, int port, unsigned int timeout = 1500);
    void disconnectSvr(redisContext** ctx);

		redisContext *ctx_;
	
		std::string host_;
		int port_;
		unsigned int timeoutMs;
	};

}



