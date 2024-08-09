wget https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
unzip
cd 
mkdir build && cd build
cmake ..
make
make install

git clone https://github.com/protocolbuffers/protobuf.git
cd protobuf
git submodule update --init --recursive
mkdir build && cd build
cmake ..
cmake --build . --parallel 1
make VERBOSE=1 test
sudo make install


https://sourceware.org/gdb/wiki/STLSupport?action=AttachFile&do=view&target=stl-views-1.0.3.gdb



https://zeeklog.com/linux-xia-an-zhuang-google-protobuf-xiang-xi-/
sudo apt-get install autoconf
sudo apt-get install libtool
wget https://github.com/protocolbuffers/protobuf/releases/download/v21.0/protobuf-all-21.0.zip
unzip protobuf-all-21.0.zip
cd protobuf
./configure
make 
make install


protobuf默认安装在 /usr/local 目录
你可以修改安装目录通过 ./configure --prefix=命令
虽然我是root用户但觉得默认安装过于分散，所以统一安装在/usr/local/protobuf下

$./configure --prefix=/usr/local/protobuf or ./configure
$ make
$ make check
$ make install

到此步还没有安装完毕，在/etc/profile 或者用户目录 ~/.bash_profile
添加下面内容
####### add protobuf lib path ########
#(动态库搜索路径) 程序加载运行期间查找动态链接库时指定除了系统默认路径之外的其他路径
#(静态库搜索路径) 程序编译期间查找动态链接库时指定查找共享库的路径
#执行程序搜索路径
#c程序头文件搜索路径
#c++程序头文件搜索路径
#pkg-config 路径
export PATH=$PATH:/usr/local/google/protobuf/bin
export PKG_CONFIG_PATH=/usr/local/google/protobuf/lib/pkgconfig
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/google/protobuf/lib
export LIBRARY_PATH=$LIBRARY_PATH:/usr/local/google/protobuf/lib
export C_INCLUDE_PATH=$C_INCLUDE_PATH:/usr/local/google/protobuf/include
export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/usr/local/google/protobuf/include
######################################
ldconfig



echo 'core-%e-%p-%t' > /proc/sys/kernel/core_pattern


tar -zxf hiredis-0.14.1.tar.gz
cd ./hiredis-0.14.1/
make
make install


sudo apt-get update
sudo apt-get install libboost-all-dev
sudo apt-get install libjsoncpp-dev
sudo apt-get install redis-server



ragel安装参考/test/ragel_test



## 安装zookeeper

1

``` bash
zookeeper-3.4.10$ cd conf    //进入文件夹
zookeeper-3.4.10/conf$  mv zoo_sample.cfg zoo.cfg //将配置文件名称进行更改
```

2、进入bin目录，启动zkServer， ./zkServer.sh start

在进行上述两布操作前需要安装jdk环境，所以必须先安装jdk，这里给出jdk的包，安装过程自行上网搜索

> 链接：https://pan.baidu.com/s/1KL0vjUt3fbNCnfojoy_N2Q 
> 提取码：gggg 
> 复制这段内容后打开百度网盘手机App，操作更方便哦

进入上面解压目录src/c下面，zookeeper已经提供了原生的C/C++和Java API开发接口，需要通过源码
编译生成，过程如下：

``` bash
~/package/zookeeper-3.4.10/src/c$ sudo ./configure
~/package/zookeeper-3.4.10/src/c$ sudo make
可能会出现错误
src/zookeeper.c:3469:21: error: '%d' directive writing between 1 and 5 bytes into a region of size between 0 and 127 [-Werror=format-overflow=]
 3469 |     sprintf(buf,"%s:%d",addrstr,ntohs(port));
修改makefile文件中548行
AM_CFLAGS = -Wall -Werror 
为
AM_CFLAGS = -Wall 
~/package/zookeeper-3.4.10/src/c$ sudo make install
```



git clone https://github.com/jbeder/yaml-cpp.git
mkdir build && cd build
cmake ..
make 
make install



https://juniway.github.io/Programming/web-multipart-formdata/


echo 'core-%e-%p-%t' > /proc/sys/kernel/core_pattern


一，指定端口，例8080
     1，netstat -tunlp |grep  8080

     2，lsof  -i:8080

二、查看服务器所有端口
     1，netstat -ntlp

三、查看某进程端口占用，例Tomcat
     1，ps -ef |grep tomcat


删除指针之后要把指针滞空
不能返回临时变量的引用
不能返回临时变量作为指针指向


wget https://nginx.org/download/nginx-1.18.0.tar.gz
tar -zxvf nginx-1.26.1.tar.gz 
./configure --with-stream

is forbidden (13: Permission denied),
https://blog.csdn.net/lf_she/article/details/118566515