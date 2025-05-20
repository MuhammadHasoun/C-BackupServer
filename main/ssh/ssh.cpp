#include "ssh.h"

SSHHandler::SSHHandler(const char * hostname,const char * username,const char *password) : host(hostname),user(username),passw(password){}

// if error occur the ssh connection will automatily end 
// if connection successfull the disconnect function will execute full disconnect


int SSHHandler::StartSession(){
	// start ssh session
	ssh_session session = ssh_new();
	if (session == nullptr){
		std::cerr << "Creating session failed" << std::endl;
		return -1;
	}
	// set options user and host
	ssh_options_set(session,SSH_OPTIONS_HOST,host,allow_control=true);
	ssh_options_set(session,SSH_OPTIONS_USER,user,allow_control=true);

	// connect to target
	if (ssh_connect(session) != SSH_OK){
		std::cerr << "Error connecting to target " << ssh_get_error(session) << std::endl;
		ssh_free(session);
		return -1;
	}

	// authentiacte
	if (ssh_userauth_password(session,nullptr,passw) != SSH_AUTH_SUCCESS){
		std::cerr << "Error authenticating with the target host (" << host << "): " << ssh_get_error(session) << " " << passw << std::endl;
		ssh_disconnect(session);
		ssh_free(session);
		return -1;
	}

	saved_ssh_session = session;

	return 0;

}

char * SSHHandler::ExecuteCommand(const char * command){
	// start ssh channel
	ssh_channel channel = ssh_channel_new(saved_ssh_session);
	if (channel == nullptr){
		std::cerr << "Error creating a channel" << ssh_get_error(saved_ssh_session) << std::endl;
		ssh_disconnect(saved_ssh_session);
		ssh_free(saved_ssh_session);
		return nullptr;
	}
	// start open session on the channel
	if (ssh_channel_open_session(channel) != SSH_OK){
		std::cerr << "Error open session for the channel " << ssh_get_error(channel);
		ssh_channel_free(channel);
		ssh_disconnect(saved_ssh_session);
                ssh_free(saved_ssh_session);
		return nullptr;
	}

	// execute give command
	if (ssh_channel_request_exec(channel,command) != SSH_OK){
		std::cerr << "Error execute command "<< ssh_get_error(channel) << std::endl;
		ssh_channel_close(channel);
		ssh_channel_free(channel);
                ssh_disconnect(saved_ssh_session);
               	ssh_free(saved_ssh_session);
		return nullptr;
	}

	// recevie ssh server response
	char buffer[256];
	char * full_response = nullptr;
	int nbytes;
	int full_size = 0;
	nbytes = ssh_channel_read(channel,buffer,sizeof(buffer)-1,0);
        buffer[nbytes]='\0';
	full_size+=nbytes;
	full_response = (char *)malloc((full_size+1) * sizeof(char));
	strcpy(full_response,buffer);

	while(nbytes > 0){
		nbytes = ssh_channel_read(channel,buffer,sizeof(buffer)-1,0);
		buffer[nbytes] = '\0';
        	full_size+=nbytes;
		full_response = (char *)realloc(full_response,(full_size+1) * sizeof(char));
		strcat(full_response,buffer);


	}

	saved_ssh_channel = channel;


	return full_response;


}

















void SSHHandler::Disconnect(){
	// end ssh connection
	std::cout << "SSH Connection ended" << std::endl;
	ssh_channel_close(saved_ssh_channel);
        ssh_channel_free(saved_ssh_channel);
        ssh_disconnect(saved_ssh_session);
        ssh_free(saved_ssh_session);
}
