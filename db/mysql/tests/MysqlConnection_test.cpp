#include <iostream>
#include "db/mysql/MysqlConnection.h"

using namespace SqlConn;

int main()
{
	MySqlConnection conn;
	bool ret = conn.connect("127.0.0.1", "root", "root", "chat");
	std::cout << ret << std::endl;
	const char *sql = "DROP table if exists users; ";
	conn.ExecuteNonQuery(sql);
	sql =	"CREATE TABLE users (id INT AUTO_INCREMENT PRIMARY KEY, username VARCHAR(50) NOT NULL, email VARCHAR(100) NOT NULL, birthdate DATE, is_active BOOLEAN DEFAULT TRUE) ENGINE=InnoDB DEFAULT CHARSET=utf8; ";
	conn.ExecuteNonQuery(sql);
	sql = "INSERT INTO users (username, email, birthdate, is_active) VALUES ('test1', 'test1@runoob.com', '1985-07-10', true);";
	conn.ExecuteNonQuery(sql);
	const char *uesername = "Bob";
	const char* email = "374961015@qq.com";
	TmDateTime date(2024, 7, 30, 22, 36);
	bool isActive = false;

	conn.ExecuteNonQuery("INSERT INTO users (username, email, birthdate, is_active) values(?,?,?,?)", uesername, email, date.ToString(), isActive);

	int id = 2;
	std::string un;
	std::string em;

	char sql1[1024];
  sprintf(sql1, "select username,email from users where id >=  %d", id);

	MySqlDataReader *rd = conn.ExecuteReader(sql1);
	while (rd->Read())
	{
		rd->GetValues(un, em);
		std::cout << un << " " << em << std::endl;
	}
	delete rd;
}