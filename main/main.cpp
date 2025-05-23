#include "sql/sql.h"
#include "ssh/ssh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <time.h>
#include <chrono>
#include <curl/curl.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <unistd.h>   // For daemon(), sleep(), fork()
#include <sys/types.h>
#include <sys/stat.h> // For umask()
#include <signal.h>   // For handling termination signals
#include <stdlib.h>   // For exit()
#include <errno.h>
// 0 == sucess
// 1 ==  fail
//CREATE TABLE backup (id INT AUTO_INCREMENT PRIMARY KEY,name VARCHAR(255) NOT NULL,created_date DATE NOT NULL,created_time TIME NOT NULL,version_number INT NOT NULL,data LONGTEXT);

const char * hostname = "localhost";
const char * user_db = "username";
const char * user_pass = "password";
const char * data_db_name = "db";
const int db_port = 3306;


char * email_payload;

const char* LOG_FILE = "/tmp/daemon_server_log.txt";
struct routers_info{
	char ** ip;
	char ** name;
	char ** usernames;
	char ** passwords;
	int * is_router;
	int * routers_only_index;
	int total_routers;
	int all_devices;
};

char *checkfornewline(const char *body) {
    if (body == NULL) return NULL;

    size_t len = strlen(body);
    // Worst case: every char is '\n', so output length could be up to 2*len
    size_t max_size = len * 2 + 1;
    char *new_body = (char *)malloc(max_size);
    if (!new_body) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (body[i] == '\n') {
            new_body[j++] = '\n';
            new_body[j++] = '\r';
        } else {
            new_body[j++] = body[i];
        }
    }

    new_body[j] = '\0';  // Null-terminate
    // Optionally shrink allocation to actual size:
    char *shrunk = realloc(new_body, j + 1);
    if (shrunk) {
        return shrunk;
    } else {
        // If realloc fails, just return original (still valid)
        return new_body;
    }
}

size_t payload_source(void *ptr, size_t size, size_t nmemb, void *data) {
    size_t room = size * nmemb;
    if (email_payload) {
        size_t len = strlen(email_payload);
        if (len > 0) {
            memcpy(ptr, email_payload, len);
            email_payload = NULL;  // Markeer als verbruikt
            return len;
        }
    }
    return 0;
}

char * GetReceiver(){
    SQLHandler sq(hostname, user_db, user_pass, data_db_name, db_port);
    MYSQL *conn = sq.Connector();
    sq.ExecuteQuery(conn, "SELECT * FROM receiver");
    MYSQL_RES *result = sq.GetResults(conn);
    MYSQL_ROW row;
    row = mysql_fetch_row(result);

    return row[1];

}

int SendEmail(const char * new_body) {
    SQLHandler sq(hostname, user_db, user_pass, data_db_name, db_port);
    MYSQL *conn = sq.Connector();
    sq.ExecuteQuery(conn, "SELECT * FROM smtp_info");
    MYSQL_RES *result = sq.GetResults(conn);
    MYSQL_ROW row;
    row = mysql_fetch_row(result);


    std::string host = "smtp://" + std::string(row[1]) + ":" + std::string(row[2]);  // Construct SMTP URL
    char *username = row[3];
    char *password = row[4];
    char * from = (char*)malloc(sizeof(char) * (strlen(username)+1));
    strcpy(from,username);


    char  *to = GetReceiver();
    const char * subject = "New Security Threat";
    //std::string body = "hello n\nits my test email\nend of the message\n\n\n\n\nya";

    //char * new_body = checkfornewline(body);


    email_payload =(char*)malloc(5000 * sizeof(char));

    snprintf(email_payload, 5000,
             "To: %s\r\n"
             "From: %s\r\n"
             "Subject: %s\r\n"
             "\r\n"  // Lege regel om headers van de body te scheiden
             "%s",
             to, from, subject, new_body);

    CURL *curl;
    CURLcode res = CURLE_OK;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        // Set the SMTP URL
        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());

        // Enable SSL/TLS
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

        // Set SMTP authentication
        curl_easy_setopt(curl, CURLOPT_USERNAME, username);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password);

        // Set the sender's email address
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, username);

        // SSL verification (consider enabling SSL verification in production)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Disable SSL verification (not recommended in production)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // Recipients
        struct curl_slist *recipients = NULL;
        recipients = curl_slist_append(recipients, to);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        // Payload
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, NULL);  // NULL is okay if payload_source handles data
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        // Perform the email sending
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            //printf("Email sent successfully.\n");
        }

        // Cleanup
        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}


void CopyToMemory(char *** data,char * info,int index){
	*data = (char**)realloc(*data,(index+1) * sizeof(char*));
        (*data)[index] = (char *)malloc((strlen(info)+1) * sizeof(char));
        strcpy((*data)[index],info);

}

void CollectTables(SQLHandler * sq, MYSQL * conn,struct tables_struct * tbs){
	sq->ExecuteQuery(conn,"SHOW TABLES");


        MYSQL_RES * tables_results = sq->GetResults(conn);

        //sq->ShowTables(tables_results);
        if (tables_results != nullptr){
                *tbs = sq->FetchTables(conn,tables_results);
	}
}



// Signal handler for clean termination
void signalHandler(int signum) {
    std::ofstream log(LOG_FILE, std::ios::app);
    log << "Daemon received signal " << signum << ", terminating.\n";
    log.close();
    exit(0);
}

// Function to create the daemon
void createDaemon() {
    // Step 1: Fork the process
    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Fork failed!" << std::endl;
        exit(1);
    }

    if (pid > 0) {
        // Exit the parent process
        std::cout << "Daemon started with PID: " << pid << std::endl;
        exit(0);
    }

    // Step 2: Create a new session
    if (setsid() < 0) {
        std::cerr << "Failed to create a new session!" << std::endl;
        exit(1);
    }

    // Step 3: Fork again to ensure the daemon is not a session leader
    pid = fork();
    if (pid < 0) {
        std::cerr << "Second fork failed!" << std::endl;
        exit(1);
    }

    if (pid > 0) {
        // Exit the intermediate parent process
        exit(0);
    }

    // Step 4: Set file permissions to ensure the daemon can access files properly
    umask(0);

    // Step 5: Change the working directory to root to avoid directory locking issues
    if (chdir("/") < 0) {
        std::cerr << "Failed to change directory to /" << std::endl;
        exit(1);
    }

    // Step 6: Close all open file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Step 7: Register signal handlers to handle termination
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
}

void CreateQuery(MYSQL* conn, const char* name_str, const char* data,int version_number) {
    time_t now = time(NULL);
    struct tm* local = localtime(&now);

    char date_str[11], time_str[9];
    sprintf(date_str, "%04d-%02d-%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday);
    sprintf(time_str, "%02d:%02d:%02d", local->tm_hour, local->tm_min, local->tm_sec);

    const char* query =
        "INSERT INTO backup (name, created_date, created_time, version_number, data) "
        "VALUES (?, ?, ?, ?, ?)";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        std::cerr << "mysql_stmt_init() failed\n";
        return;
    }

    if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
        std::cerr << "mysql_stmt_prepare() failed: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt);
        return;
    }

    unsigned long data_length = strlen(data);  // Length of the multi-line BLOB data
    // Bind parameters
    MYSQL_BIND bind[5] = {};

    bind[0].buffer_type = MYSQL_TYPE_STRING;  // Name
    bind[0].buffer = (void*)name_str;
    bind[0].buffer_length = strlen(name_str);

    bind[1].buffer_type = MYSQL_TYPE_STRING;  // Date
    bind[1].buffer = (void*)date_str;
    bind[1].buffer_length = sizeof(date_str) - 1;

    bind[2].buffer_type = MYSQL_TYPE_STRING;  // Time
    bind[2].buffer = (void*)time_str;
    bind[2].buffer_length = sizeof(time_str) - 1;

    bind[3].buffer_type = MYSQL_TYPE_LONG;    // Version number
    bind[3].buffer = (void*)&version_number;
    bind[3].is_null = 0;

    bind[4].buffer_type = MYSQL_TYPE_STRING;    // Data (BLOB)
    bind[4].buffer = (void*)data;
    bind[4].buffer_length = data_length;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "mysql_stmt_bind_param() failed: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt);
        return;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::cerr << "mysql_stmt_execute() failed: " << mysql_stmt_error(stmt) << "\n";
    } else {
       std::cout << "Data inserted successfully!\n";
    }

    mysql_stmt_close(stmt);
}

void GetDevicesInfo(SQLHandler * sq,char * target_table,struct routers_info * router_struct,MYSQL * conn){
	std::string routers_query = "SELECT * FROM "+std::string(target_table);
        sq->ExecuteQuery(conn,routers_query.c_str());

        MYSQL_RES * result = sq->GetResults(conn);
        int fields_num = mysql_num_fields(result);
        int rows_num = mysql_num_rows(result);

        MYSQL_ROW row;
        int i = 0;
        int router_index = 0;

	if (result != nullptr){
                while((row = mysql_fetch_row(result)) != NULL){
                        // get all ips
                        CopyToMemory(&router_struct->ip,row[1],i);
                        // get all names
                        CopyToMemory(&router_struct->name,row[2],i);
                        // get all usernames
                        CopyToMemory(&router_struct->usernames,row[4],i);
                        // get all passwords
                        CopyToMemory(&router_struct->passwords,row[5],i);
                        if (!strcmp(row[7],"0")){

                                router_struct->is_router = (int *)realloc(router_struct->is_router, (i+1) * sizeof(int));
                                router_struct->is_router[i] = 0;
                        }
                        else{
                                router_struct->is_router = (int *)realloc(router_struct->is_router, (i+1) * sizeof(int));
                                router_struct->is_router[i] = 1;


                                router_struct->routers_only_index = (int *)realloc(router_struct->routers_only_index,(router_index+1) * sizeof(int));
                                router_struct->routers_only_index[router_index] = i;
                                router_index++;

                        }

                        i++;
                }
                router_struct->total_routers = router_index;
                router_struct->all_devices = i;
        }

}

int GetTargetTable(struct tables_struct tbs){
	for (int i = 0; i <tbs.length ; i++){
		if(!strcmp(tbs.tables[i],"macFilterSwitch")){
			return i;
		}
	}
}

void ClearTablesStructs(struct tables_struct &tbs){
	memset(&tbs,0,sizeof(tbs));
}

void ClearRoutersStructs(struct routers_info &router_struct){
        memset(&router_struct,0,sizeof(router_struct));
}


void ClearDeviceStatus(){
   SQLHandler sq(hostname,user_db,user_pass,data_db_name,db_port);
    MYSQL * conn = sq.Connector();
    if (conn == nullptr){
	return ;
    }

    sq.ExecuteQuery(conn,"DELETE FROM switchesStatus");

}
void DeviceStatus(struct routers_info *router_struct, int index_number, char *device_full_info, char *location) {
    SQLHandler sq(hostname,user_db,user_pass,data_db_name,db_port);
    MYSQL * conn = sq.Connector();
    if (conn == nullptr){
	return ;
    }

    // Ensure index_number is valid
    if (index_number < 0 || index_number >= router_struct->all_devices) {
        std::cerr << "Invalid index number: " << index_number << std::endl;
        return;
    }

    // Check if device_full_info is not null
    if (device_full_info == nullptr) {
        std::cerr << "device_full_info is null" << std::endl;
        return;
    }

    // Same for location
    if (location == nullptr) {
        std::cerr << "location is null" << std::endl;
        return;
    }

    // Initialize character buffers
    char uptime[50] = {0}, version[20] = {0}, buildTime[30] = {0};
    char freeMemory[20] = {0}, totalMemory[20] = {0}, cpu[10] = {0};
    char cpuCount[5] = {0}, cpuLoad[10] = {0}, freeHddSpace[20] = {0};
    char totalHddSpace[20] = {0}, badBlocks[10] = {0}, architecture[10] = {0};
    char boardName[30] = {0}, platform[20] = {0};

    char *router_name = router_struct->name[index_number];
    char *router_ip = router_struct->ip[index_number];
    int running = 1;

    // Extracting values from device_full_info
    int scanned = sscanf(device_full_info,
                         "%*s %49[^\n]\n%*s %19[^\n]\n%*s %29[^\n]\n%*s %19[^\n]\n%*s %19[^\n]\n%*s %9[^\n]\n%*s %4[^\n]\n%*s %9[^\n]\n%*s %19[^\n]\n%*s %19[^\n]\n%*s %9[^\n]\n%*s %9[^\n]\n%*s %29[^\n]\n%*s %19[^\n]\n%*s %19[^\n]",
                         uptime, version, buildTime, freeMemory, totalMemory, cpu, cpuCount,
                         cpuLoad, freeHddSpace, totalHddSpace, badBlocks, architecture, boardName, platform);

    // Check if all expected values were scanned
    if (scanned < 13) {
        std::cerr << "Error: Not all values were scanned from device_full_info\n";
        return; // Handle error as needed
    }

    // Prepare the INSERT statement
    const char *query = "INSERT INTO switchesStatus (switch_name, ip_address, running, uptime, version, build_time, "
                        "board_name, free_hdd_space, free_memory, cpu_load) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"; 
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt) {
        std::cerr << "mysql_stmt_init() failed\n";
        return;
    }

    if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
        std::cerr << "mysql_stmt_prepare() failed: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt);
        return;
    }
    
    // Bind parameters
    MYSQL_BIND bind[10] = {};
    memset(bind, 0, sizeof(bind)); // Initialize binding structure

    // Bind switch_name
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char *)router_name;
    bind[0].buffer_length = strlen(router_name); // Ensure this is valid

    // Bind ip_address
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (char *)router_ip;
    bind[1].buffer_length = strlen(router_ip); // Ensure this is valid

    // Bind running
    bind[2].buffer_type = MYSQL_TYPE_TINY; // Assuming tiny int for running
    bind[2].buffer = (void *)&running;
    bind[2].is_null = 0;

    // Bind uptime
    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = (char *)uptime;
    bind[3].buffer_length = strlen(uptime);

    // Bind version
    bind[4].buffer_type = MYSQL_TYPE_STRING;
    bind[4].buffer = (char *)version;
    bind[4].buffer_length = strlen(version);

    // Bind build_time
    bind[5].buffer_type = MYSQL_TYPE_STRING;
    bind[5].buffer = (char *)buildTime;
    bind[5].buffer_length = strlen(buildTime);

    // Bind board_name
    bind[6].buffer_type = MYSQL_TYPE_STRING;
    bind[6].buffer = (char *)boardName;
    bind[6].buffer_length = strlen(boardName);

    // Bind free_hdd_space
    bind[7].buffer_type = MYSQL_TYPE_STRING;
    bind[7].buffer = (char *)freeHddSpace;
    bind[7].buffer_length = strlen(freeHddSpace);

    // Bind free_memory
    bind[8].buffer_type = MYSQL_TYPE_STRING;
    bind[8].buffer = (char *)freeMemory;
    bind[8].buffer_length = strlen(freeMemory);

    // Bind cpu_load
    bind[9].buffer_type = MYSQL_TYPE_STRING;
    bind[9].buffer = (char *)cpuLoad;
    bind[9].buffer_length = strlen(cpuLoad);

    // Execute the prepared statement
    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        std::cerr << "mysql_stmt_bind_param() failed: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt);
        return;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::cerr << "mysql_stmt_execute() failed: " << mysql_stmt_error(stmt) << "\n";
    } else {
        std::cout << "Devices inserted succesfully!\n";
    }

    // Close the statement
    mysql_stmt_close(stmt);
}
void WriteCurrentVersion(int version) {
    char str[20];
    snprintf(str, sizeof(str), "%d", version);  // Safer than sprintf

    FILE *file = fopen("version.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    fprintf(file, "%s", str);
    fflush(file);  // Force the buffer to flush
    fclose(file);
}

int ReadVersion(){
        FILE *file;
        int num;

        // Open the file in read mode
        file = fopen("version.txt", "r");
         if (file == NULL) {
                printf("Error opening the file!\n");
                return 1;  // Exit if file could not be opened
        }

        // Read integers from the file
        while (fscanf(file, "%d", &num) == 1) {
                //printf("Read number: %d\n", num);  // Print each integer
        }
        //printf("%d\n",num+499);
        // Close the file
        fclose(file);

        return num;
}

int UpdateDaysCount(int days_count) {
    SQLHandler sq(hostname, user_db, user_pass, data_db_name, db_port);
    MYSQL* conn = sq.Connector();

    // Proper string concatenation
    std::string query = "UPDATE backup SET clear_contact_days = " + std::to_string(days_count);

    // Execute the query
    sq.ExecuteQuery(conn, query.c_str());

    return 0;  // Return value for function (as it's expected to return int)
}

int GetExpireIndays(){
	SQLHandler sq(hostname,user_db,user_pass,data_db_name,db_port);
        MYSQL * conn = sq.Connector();
	sq.ExecuteQuery(conn,"SELECT clear_day FROM backup ORDER BY id DESC LIMIT 1");
        MYSQL_RES * result = sq.GetResults(conn);
        MYSQL_ROW row;
        row = mysql_fetch_row(result);
        int expiry_days = atoi(row[0]);
	return expiry_days;

}

void CheckIfExpired(int day_count){
     int expiry_date = GetExpireIndays();
     if(day_count > expiry_date){
	std::string expiry_query = "DELETE FROM backup WHERE created_date < NOW() - INTERVAL "+std::to_string(expiry_date)+" DAY LIMIT 1000";
	SQLHandler sq(hostname,user_db,user_pass,data_db_name,db_port);
    	MYSQL * conn = sq.Connector();
    	sq.ExecuteQuery(conn,expiry_query.c_str());
	UpdateDaysCount(0);
    }


}

int GetCurrentVersion(){
	SQLHandler sq(hostname,user_db,user_pass,data_db_name,db_port);
        MYSQL * conn = sq.Connector();
        sq.ExecuteQuery(conn,"SELECT version_number FROM backup ORDER BY id DESC LIMIT 1");
	MYSQL_RES * result = sq.GetResults(conn);
        MYSQL_ROW row;
        row = mysql_fetch_row(result);
	int version_number = atoi(row[0]);
	return version_number;

}
int GetDaysCount(){
	SQLHandler sq(hostname,user_db,user_pass,data_db_name,db_port);
        MYSQL * conn = sq.Connector();
        sq.ExecuteQuery(conn,"SELECT clear_contact_days FROM backup ORDER BY id DESC LIMIT 1");
        MYSQL_RES * result = sq.GetResults(conn);
        MYSQL_ROW row;
        row = mysql_fetch_row(result);
        int days = atoi(row[0]);
	return days;
}
int CompareBackup(char * router_name,char * data1){
	int status = 2;
        SQLHandler sq(hostname,user_db,user_pass,data_db_name,db_port);
        MYSQL * conn = sq.Connector();
        std::string query = "SELECT * FROM backup WHERE version_number = " 
                   + std::to_string(GetCurrentVersion() - 1) 
                   + " AND name = '" 
                   + std::string(router_name)
		   + "'";
        sq.ExecuteQuery(conn,query.c_str());
        MYSQL_RES * result = sq.GetResults(conn);
        int fields_num = mysql_num_fields(result);
        int rows_num = mysql_num_rows(result);

        MYSQL_ROW row;
	row = mysql_fetch_row(result);
        int i = 0;
        int router_index = 0;
	if (strlen(row[5]) == strlen(data1)){
		int data_length = strlen(row[5]);
		for (int i = 0 ; i < data_length ; i++){
			if (row[5][i] == '#'){
				do{
					i++;
				}while(row[5][i] != '\n');
			}
			else if (row[5][i] == data1[i]){
				status = 1;
			}
			else{
				// not equal
				status = 0;
				break;
			}
		}
	}


	return status;
}

void CheckBackupStatus() {
    SQLHandler sq(hostname, user_db, user_pass, data_db_name, db_port);
    MYSQL* conn = sq.Connector();
    if (conn == nullptr) {
        // Connection failed
        return;
    }

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return;

    const char* query = "SELECT * FROM backup WHERE version_number = ?";
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        mysql_stmt_close(stmt);
        return;
    }

    int currentVersion = GetCurrentVersion();

    MYSQL_BIND bind_param[1];
    memset(bind_param, 0, sizeof(bind_param));

    bind_param[0].buffer_type = MYSQL_TYPE_LONG;
    bind_param[0].buffer = (char*)&currentVersion;
    bind_param[0].is_null = 0;
    bind_param[0].length = 0;

    if (mysql_stmt_bind_param(stmt, bind_param)) {
        mysql_stmt_close(stmt);
        return;
    }

    if (mysql_stmt_execute(stmt)) {
        mysql_stmt_close(stmt);
        return;
    }

    // Verkrijg metadata van resultaat
    MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
    if (!meta) {
        mysql_stmt_close(stmt);
        return;
    }

    int field_count = mysql_num_fields(meta);

    // Maak bind structuren voor resultaat
    MYSQL_BIND* bind_result = new MYSQL_BIND[field_count];
    memset(bind_result, 0, sizeof(MYSQL_BIND) * field_count);

    // Voor eenvoud: we gaan ervan uit dat alle velden strings zijn, max 1024 chars
    char** buffers = new char*[field_count];
    unsigned long* lengths = new unsigned long[field_count];
    my_bool* is_null = new my_bool[field_count];

    for (int i = 0; i < field_count; ++i) {
        buffers[i] = new char[1024];
        memset(buffers[i], 0, 1024);
        bind_result[i].buffer_type = MYSQL_TYPE_STRING;
        bind_result[i].buffer = buffers[i];
        bind_result[i].buffer_length = 1024;
        bind_result[i].length = &lengths[i];
        bind_result[i].is_null = &is_null[i];
    }

    if (mysql_stmt_bind_result(stmt, bind_result)) {
        // cleanup
        for (int i = 0; i < field_count; ++i) delete[] buffers[i];
        delete[] buffers;
        delete[] lengths;
        delete[] is_null;
        delete[] bind_result;
        mysql_free_result(meta);
        mysql_stmt_close(stmt);
        return;
    }

    std::string body;

    // Loop over resultaten
    while (mysql_stmt_fetch(stmt) == 0) {
        // row data zit nu in buffers[i], let op nulls
        const char* router_name = is_null[1] ? "" : buffers[1];
        const char* data = is_null[5] ? "" : buffers[5];
        const char* version = is_null[4] ? "" : buffers[4];

        if (CompareBackup((char*)router_name, (char*)data) == 0) {
            body = std::string("Hello Aron\nRouterName: ") + router_name + "\nVersion: " + version + "\nNew Threat";
            SendEmail(body.c_str());
        }
    }

    // Cleanup geheugen
    for (int i = 0; i < field_count; ++i) delete[] buffers[i];
    delete[] buffers;
    delete[] lengths;
    delete[] is_null;
    delete[] bind_result;
    mysql_free_result(meta);
    mysql_stmt_close(stmt);
}


int main() {
    int versie_nummer = GetCurrentVersion();
    int days_count = GetDaysCount();

    // Start de daemon
    createDaemon();

    while (true) {
        try {
            // Initialiseer structen
            struct tables_struct tbs;
            struct routers_info router_struct = {nullptr, nullptr, nullptr, nullptr, 0, 0};

            // Maak databaseverbinding
            SQLHandler sq(hostname, user_db, user_pass, data_db_name, db_port);
            MYSQL* conn = sq.Connector();
            if (conn == nullptr) {
                std::cerr << "[FOUT] Geen verbinding met database." << std::endl;
                return 1;
            }

            // Haal tabellen op en kies target
            CollectTables(&sq, conn, &tbs);
            int target_table = GetTargetTable(tbs);

            // Laad routergegevens
            GetDevicesInfo(&sq, tbs.tables[target_table], &router_struct, conn);

            versie_nummer++;

            // Verwerk elk apparaat
            for (int i = 0; i < router_struct.all_devices; ++i) {
                try {
                    const std::string host = router_struct.ip[i];
                    const std::string username = router_struct.usernames[i];
                    const std::string password = router_struct.passwords[i];
                    char* device_name = router_struct.name[i];
                    int is_router = router_struct.is_router[i];

                    std::cout << "[INFO] Verwerken van apparaat: " << device_name << std::endl;

                    // Start SSH sessie
                    SSHHandler sh(host.c_str(), username.c_str(), password.c_str());
                    int session_status = sh.StartSession();

                    if (session_status == 0) {
                        char* response = sh.ExecuteCommand("export");
                        char* device_full_info = sh.ExecuteCommand("/system resource print");
                        char* location = sh.ExecuteCommand("/system identity print");

                        if (response && device_full_info && location) {
                            CreateQuery(conn, device_name, response, versie_nummer);
                            DeviceStatus(&router_struct, i, device_full_info, location);
                        }

                        sh.Disconnect();
                    }

                } catch (const std::exception& e) {
                    std::cerr << "[FOUT] Apparaat fout: " << e.what() << std::endl;
                }
            }

            // Opruimen
            ClearTablesStructs(tbs);
            ClearRoutersStructs(router_struct);

            // Wacht 24 uur
            const auto SLEEP_DURATION = std::chrono::hours(24);
            std::this_thread::sleep_for(SLEEP_DURATION);

            // Back-up en statuscontroles
            CheckBackupStatus();
            int updated_days = GetDaysCount() + 1;
            UpdateDaysCount(updated_days);
            CheckIfExpired(updated_days);

            // Verwijder oude statussen
            ClearDeviceStatus();

        } catch (const std::exception& e) {
            std::cerr << "[FOUT] Hoofdlus: " << e.what() << std::endl;
        }
    }

    return 0;
}

