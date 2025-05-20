#include <mysql/mysql.h>
#include <iostream>

using namespace std;
struct tables_struct{
        char ** tables;
        int length;

};


class SQLHandler{

private:
	const char * server;
	const char * user;
	const char * password;
	const char * database;
	unsigned int port;
	MYSQL * current_connection;
public:
	SQLHandler(const char * serv, const char * us, const char * pw, const char * db, unsigned int pt);
	MYSQL  * Connector();
	int ExecuteQuery(MYSQL * conn,const char * query);
	MYSQL_RES * GetResults(MYSQL * conn);
	void ShowResults(MYSQL_RES * res);
	struct tables_struct FetchTables(MYSQL * conn ,MYSQL_RES * res);
	void ShowTables(MYSQL_RES * res);

	~SQLHandler();
};
