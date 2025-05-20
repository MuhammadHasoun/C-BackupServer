
# Daemon Server with MySQL and Email Notifications

## Overview

This project is a Linux daemon server written in C++ that connects to a MySQL database and sends email notifications. It logs activities and handles system signals for graceful shutdown.

## Features

- Runs as a daemon process
- Connects to a MySQL database to fetch data
- Sends emails using libcurl SMTP (Gmail example)
- Logs events to a log file (`/tmp/daemon_server_log.txt`)
- Handles UNIX signals to shutdown cleanly

## Requirements

- Linux environment
- MySQL client libraries and server
- libcurl development libraries
- Build tools (g++)

## Installation

1. Install dependencies (Debian/Ubuntu example):

```bash
sudo apt-get update
sudo apt-get install libmysqlclient-dev libcurl4-openssl-dev g++
```

2. Build the project:

```bash
g++ -o daemon_server daemon_server.cpp -lmysqlclient -lcurl
```

## Configuration

- Update MySQL connection details in the source code:

```cpp
char* server = "localhost";
char* user = "root";
char* password = "example"; // Your MySQL password
char* database = "users";
int mysql_port = 3306;
```

- Update SMTP email credentials in the `sendEmail` function:

```cpp
curl_easy_setopt(curl, CURLOPT_USERNAME, "your_email@gmail.com");
curl_easy_setopt(curl, CURLOPT_PASSWORD, "your_password");
curl_easy_setopt(curl, CURLOPT_MAIL_FROM, "<your_email@gmail.com>");
```

## Running

Run the compiled daemon server binary:

```bash
./daemon_server
```

Check the log file at `/tmp/daemon_server_log.txt` for activity.

## Signals

- Press `Ctrl+C` or send SIGINT to shutdown gracefully.

## Notes

- For production, use secure methods to store credentials.
- Consider using environment variables or config files.
- Gmail SMTP requires app passwords or OAuth2 setup.

## License

This project is provided as-is without warranty.
