#include "sql.h"
#include <cstring>
#include <stdlib.h>
#include <string>



// initialize all required values
SQLHandler::SQLHandler(const char * serv, const char * us, const char * pw,const char * db, unsigned int pt) : server(serv),user(us),password(pw),database(db),port(pt){}

SQLHandler::~SQLHandler(){
	if (current_connection){
		mysql_close(current_connection);
	}
}

// function connect to the data
// return mysql connection
MYSQL * SQLHandler::Connector(){
	MYSQL  * conn;
	conn = mysql_init(NULL);

	// connect to the database
	if (!mysql_real_connect(conn,server,user,password,database,port,NULL,0)){
		// if fail close connection and return nullptr;
		std::cerr << "Connection Failed "<< mysql_error(conn) << std::endl;
		mysql_close(conn);
		return nullptr;
	}

	// if connected return the connection

	current_connection = conn;
	return conn;

}



// excute query to a exisiting connected server 
int SQLHandler::ExecuteQuery(MYSQL * conn, const char * query){

	// execute the given query
	if (mysql_query(conn,query)){
		// execution failed
		std::cerr << "Query Execution Failed " << mysql_error(conn) << std::endl;
		mysql_close(conn);
		return 1;
	}

	return 0;
}

// fetch all data or return
// data_status =  0 - fetch , print
// data status =  1 - 1 return all data;

MYSQL_RES * SQLHandler::GetResults(MYSQL * conn){
	MYSQL_RES * res =  mysql_store_result(conn);
	if (res == NULL){
	// fetching results failed
		std::cerr << "Fetching results failed " << mysql_error(conn) << std::endl;
		mysql_close(conn);
		return nullptr;

	}

	return res;
}

void SQLHandler::ShowResults(MYSQL_RES * res){

	MYSQL_ROW  row;
	int total_rows = mysql_num_rows(res);
	int fields_ammount = mysql_num_fields(res);
	while ((row = mysql_fetch_row(res))) {
        	for (int i = 0; i < fields_ammount; i++) {
            		printf("%s\t", row[i] ? row[i] : "NULL");
        	}
        	printf("\n");
    	}

}

void SQLHandler::ShowTables(MYSQL_RES * res){
	MYSQL_ROW row;
	while((row = mysql_fetch_row(res)) != NULL ){
		std::cout << row[0] << std::endl;
	}
}

struct tables_struct SQLHandler::FetchTables(MYSQL * conn,MYSQL_RES * res){
	char ** tables = NULL;
	int total_tables = 0;
	struct tables_struct tbs;
	MYSQL_ROW row;
	while((row = mysql_fetch_row(res)) != NULL){
		tables = (char **) realloc(tables,(total_tables+1) * sizeof(char*));
		tables[total_tables] = (char*) malloc((strlen(row[0])+1) * sizeof(char));
		strncpy(tables[total_tables],row[0],strlen(row[0])+1);
		total_tables++;

	}
	tbs.tables = tables;
	tbs.length = total_tables;

	// return array of tables
	return tbs;
}

