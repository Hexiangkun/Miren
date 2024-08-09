#include "http/web/HttpWeb.h"
#include "http/web/HttpContext.h"
#include "net/EventLoop.h"
#include "base/log/Logging.h"

#include <iostream>

using namespace Miren;
using namespace Miren::http;
using namespace Miren::base;


int main() {
    //环境初始化

    net::EventLoop loop;
    http::HttpWeb c(&loop,net::InetAddress(8000), "httpWeb");

    //全局中间件
    c.Use([](std::shared_ptr<HttpContext> ctx) {
        LOG_INFO << "cweb 这是一个全局中间件";
        ctx->Next();
    });
    //普通get请求
    c.GET("/api/sayhi", [](std::shared_ptr<HttpContext> c){
        c->STRING(HttpStatusCode::OK, "hi, welcome to cweb");
    });
    //表单数据请求
    c.POST("api/sayhi", [](std::shared_ptr<HttpContext> c){
        c->STRING(HttpStatusCode::OK, "hi, " + c->PostForm("name") + "/" + c->PostForm("age") + "/" + c->PostForm("hobby") + ",welcome to cweb");
    });
    
    // 上传multipart
    c.POST("api/multipart", [](std::shared_ptr<HttpContext> c){
        std::shared_ptr<MultipartPart> part1 = c->MultipartForm("file");
        if(part1) {
            BinaryData data(part1->data().c_str(), part1->data().size());
            std::string filename = part1->filename();
            c->SaveUploadedFile(data, "/root/hxk/server/resources/", "tmp"+filename);
            c->STRING(HttpStatusCode::OK, "I received your data : " + part1->name());
        }
        else {
            c->STRING(HttpStatusCode::OK, "Your request is error");
        }
    });
    
    //json
    c.GET("api/info", [](std::shared_ptr<HttpContext> c){
        Json::Value root;
        root["name"] = "lemon";
        root["sex"] = "man";
        root["age"] = 26;
        root["school"] = "xjtu";
        
        Json::Value hobby;
        hobby["sport"] = "football";
        hobby["else"] = "sing";
        
        root["hobby"] = hobby;
        
        std::stringstream body;
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["emitUTF8"] = true;
        std::unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());
        jsonWriter->write(root, &body);
        
        c->JSON(HttpStatusCode::OK, body.str());
    });
    
    //带参数请求 ?key=value
    c.GET("api/echo", [](std::shared_ptr<HttpContext> c){
        c->STRING(HttpStatusCode::OK, "hi, " + c->Query("name") + ",welcome to cweb");
    });
    
    //动态路由
    c.GET("/api/dynamic/:param", [](std::shared_ptr<HttpContext> c) {
        c->STRING(HttpStatusCode::OK, "I got your param: " + c->RouterParam("param"));
    });
    
    //文件下载
    c.GET("/api/download/city", [](std::shared_ptr<HttpContext> c) {
        c->FILE(HttpStatusCode::OK, "/root/hxk/server/resources/city1.jpg");
    });
    
    c.GET("/api/download/bluesky", [](std::shared_ptr<HttpContext> c) {
        c->FILE(HttpStatusCode::OK, "/root/hxk/server/resources/bluesky.mp4");
    });
    
    //multipart数据
    c.GET("/api/multipart/data", [](std::shared_ptr<HttpContext> c) {
        MultipartPart* part1 = new MultipartPart();
        std::string text = "this is a text";
        part1->setName("text");
        part1->setContentType("text/plain");
        part1->setData(text);
        
        MultipartPart* part2 = new MultipartPart();
        std::string json = "{\"name\": \"John\",\"age\": 30,\"city\": \"New York\"}";
        part2->setName("json");
        part2->setContentType("application/json");
        part2->setData(json);
        
        MultipartPart* part3 = new MultipartPart();
        part3->setName("image");
        part3->setFilename("image.jpg");
        part3->setContentType("image/jpeg");
        part3->setImage("/root/hxk/server/resources/city1.jpg");
        
        c->MULTIPART(HttpStatusCode::OK, std::vector<MultipartPart *>{part1, part2, part3});
        delete part1;
        delete part2;
        delete part3;
    });


    c.Run(0);
    loop.loop();
    
    //mysql
    // c.GET("/api/mysql/data", [](std::shared_ptr<HttpContext> c) {
    //     RedisReplyPtr rr = c->Redis()->Cmd("GET /api/mysql/data");
    //     std::stringstream data;
    //     Json::Value res;
    //     Json::StreamWriterBuilder writerBuilder;
    //     writerBuilder["emitUTF8"] = true;
    //     std::unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());
    //     if(rr && rr->str) {
    //         res["type"] = "redis cache";
    //         res["data"] = rr->str;
    //         jsonWriter->write(res, &data);
    //         c->JSON(HttpStatusCode::OK, data.str().c_str());
    //         return;
    //     }
        
    //     MySQLReplyPtr mr = c->MySQL()->Cmd("SELECT * FROM person");
    //     int rows = mr->Rows();
    //     res["count"] = rows;
    //     for(int i = 0; i < rows; ++i) {
    //         mr->Next();
    //         Json::Value p;
    //         p["id"] = mr->IntValue(0);
    //         p["name"] = mr->StringValue(1);
    //         p["age"] = mr->IntValue(2);
    //         res["people"].append(p);
    //     }
        
    //     Json::Value res1;
    //     res1["type"] = "mysql";
    //     res1["data"] = res;
        
    //     jsonWriter->write(res1, &data);
    //     c->Redis()->Cmd("SET /api/mysql/data %s PX %d", data.str().c_str(), 10 * 1000);

    //     c->JSON(HttpStatusCode::OK, data.str());
    // });
    
    //redis
    // c.GET("/api/redis/data", [](std::shared_ptr<HttpContext> c) {
    //     RedisReplyPtr r = c->Redis()->Cmd("GET key_htl_hlea_tlyyyy_ghhtl_aseghlea");
    //     c->STRING(HttpStatusCode::OK, r->str);
    // });
    
    //redis lock
    // c.GET("api/redis/lock", [](std::shared_ptr<HttpContext> c) {
    //     struct timespec time;
    //     clock_gettime(CLOCK_REALTIME, &time);
    //     static std::string key = "lockkey-" + std::to_string(time.tv_nsec);
    //     std::string value = c->Redis()->Lock(key, 600 * 1000);
    //     if(value.size() > 0) {
    //         c->STRING(HttpStatusCode::OK, "lock success key:" + key + "value:" + value);
    //     }else {
    //         c->STRING(HttpStatusCode::OK, "lock failed key:" + key);
    //     }
    // });
    
    // c.GET("api/redis/unlock/:key/:value", [](std::shared_ptr<HttpContext> c) {
    //     LOG(LOGLEVEL_DEBUG, CWEB_MODULE, "redis", "key: %s value: %s", c->Param("key").c_str(), c->Param("value").c_str());
    //     bool res = c->Redis()->Unlock(c->Param("key"), c->Param("value"));
    //     if(res) {
    //         c->STRING(HttpStatusCode::OK, "unlock success");
    //     }else {
    //         c->STRING(HttpStatusCode::OK, "unlock failed");
    //     }
    // });
    
    // 分组路由
    // Group* g1 = c.Group("/group");
    // //分组中间件
    // g1->Use([](std::shared_ptr<HttpContext> c){
    //     LOG(LOGLEVEL_INFO, CWEB_MODULE, "cweb", "这是一个分组中间件");
    //     c->Next();
    // });
    
    // g1->GET("/sayhi", [](std::shared_ptr<HttpContext> c){
    //     c->STRING(HttpStatusCode::OK, "hi, welcome to cweb group");
    // });
}

