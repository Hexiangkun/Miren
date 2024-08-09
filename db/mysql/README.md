安装mysql依赖
apt install mysql-server
apt install mysql-client
apt install libmysqlclient-dev

修改mysql-server配置
vim /etc/mysql/mysql.conf.d/mysqld.cnf
将bind-address = 127.0.0.1 改为 bind-address = 0.0.0.0

修改数据库root密码和登录
mysql -uroot -p
use mysql;
ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY 'very_strong_password';
update user set host = '%' where user = 'root';
FLUSH PRIVILEGES;


参考连接：https://blog.csdn.net/YM_1111/article/details/107555383