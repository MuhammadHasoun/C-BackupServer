#include <stdio.h>
#include <stdlib.h>
#include <libssh/libssh.h>
#include <iostream>
#include <string.h>
class SSHHandler{

private:
	const char * host;
	const char * user;
	const char * passw;
	ssh_channel saved_ssh_channel;
	ssh_session saved_ssh_session;
public:
	SSHHandler(const char * hostname,const char * username,const char * password);
	int StartSession();
	char * ExecuteCommand(const char * command);
	void Disconnect();
};
