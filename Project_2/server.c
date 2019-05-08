#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//Given header files
#include "types.h"
#include "constants.h"

//Header files created by us
#include "server.h"
#include "parse.h"

int main(int argc, char* argv[]){
	setbuf(stdout, NULL);
	srand(time(NULL));

	if(argc == 2 && strcmp(argv[1], "--help") == 0){
		server_help();
		exit(0);
	}

	if(argc != 3){
		printf("Usage: server number_of_bank_offices \"admin_password\"\n");
		printf("Try server --help for more information\n.");
		exit(-1);
	}

	if(strlen(argv[2]) > 20 || strlen(argv[2]) < 8){
		printf("Password length must be between 8 to 20 characters.\n");
		exit(-2);
	}

	check_number(argv[1]);

	if(atoi(argv[1]) > 99 || atoi(argv[1]) < 1 || strlen(argv[1]) > 9){
		printf("Number of bank offices must be between 1 and 99.\n");
		exit(-4);
	}

	server_inf* server_information = malloc(sizeof(server_inf));

	parse_server_inf(argv, server_information);

	free_server_information(server_information);

	return 0;
}

void server_help(){
	printf("Usage: server number_of_bank_offices \"admin_password\"\n");
	printf("Creates a homebanking server in your pc\n");
	printf("Example: server 10 \"bad_password\"\n\n");

	printf("number_of_bank_offices:\n"); 
	printf("	This argument is an integer that specifies the number of bank offices to create in the server.\n");
	printf("	Max number of bank offices: 99\n\n");

	printf("admin_password:\n");
	printf("	This argument represents the administrator password. This have to be between quotation marks.\n");
}