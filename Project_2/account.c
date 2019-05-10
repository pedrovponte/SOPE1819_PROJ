#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "account.h"

void salt_generator(char* salt){
	char string[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	for(int i = 0; i < SALT_LEN; i++){
		salt[i] = string[rand() % (strlen(string))];
	}

	salt[SALT_LEN] = '\0';
}

bank_account_t* create_client_account(client_inf* client_information){
	usleep(client_information->operation_delay); //TODO Probably change this
	bank_account_t* account = (bank_account_t*)malloc(sizeof(bank_account_t));

	account->account_id = client_information->account_id;
	salt_generator(account->salt);

	//Creating hash
	char* password = malloc(strlen(client_information->account_password)+strlen(account->salt));
	password = memcpy(password, client_information->account_password);
	int fd[2], fork_value;

	if(pipe(fd)){
		perror("pipe");
		exit(-1);
	}

	if((fork_value = fork()) == -1){
		perror("fork");
		exit(-1);
	}
	else if(fork_value == 0){
		if(dup2(fd[WRITE], STDOUT_FILENO) == -1){
			perror("dup2");
			exit(-1);
		}

		int new_fd[2];

		if(pipe(new_fd)){
			perror("pipe");
			exit(-1);
		} 

		if((fork_value = fork()) == -1){
			perror("fork");
			exit(-1);
		}
		else if(fork_value == 0){
			close(new_fd[READ]);
			if(dup2(new_fd[WRITE], STDOUT_FILENO) == -1){
				perror("dup2");
				exit(-1);
			}
			execlp("echo", "echo", strcat(password, account->salt), NULL);
			printf("ERROR!!!\n");
			close(new_fd[WRITE]);
			exit(-1);
		}

		close(new_fd[WRITE]);
		waitpid(fork_value, NULL, 0);
		
		dup2(new_fd[READ], STDIN_FILENO);

		execlp("sha256sum", "sha256sum", NULL);
		printf("ERROR!!!\n");
        close(fd[WRITE]);
        exit(-1);
	}
	else{
		close(fd[WRITE]);
		waitpid(fork_value, NULL, 0);
		while(read(fd[READ], account->hash, HASH_LEN) == 0);
		close(fd[READ]);
	}

	account->balance = 0;

	return account; //Probably this don't works
}

void money_transfer(uint32_t account_id, char* password, uint32_t new_account_id, uint32_t balance){
	if(pthread_mutex_lock(save_account_mutex)){
		perror("pthread_mutex_lock");
		exit(-1);
	}

	int fd = open("accounts.txt", O_RDONLY);
	if(fd < 0){
		perror("open");
		exit(-1);
	}

	char* string = malloc(sizeof(uint32_t)+sizeof(char)); //The biggest string stored in the file has this length
	bank_account_t tmp_account, account, new_account;

	account.id = 0; new_account.id = 0; //Initialize variables to check if they exist later

	while(read(fd, string, sizeof(uint32_t)+sizeof(char)) > 0){
		tmp_account.account_id = stoi(string);
		read(fd, tmp_account.hash, HASH_LEN+1);
		read(fd, tmp_account.salt, HASH_LEN+1);
		memset(string, '\0', sizeof(uint32_t)+sizeof(char));
		tmp_account.balance = stoi(string);

		if(tmp_account.id == account_id){
			tmp_account.hash[HASH_LEN] = '\0';
			tmp_account.salt[HASH_LEN] = '\0';
			
			//Creating hash
			char* hash = malloc(HASH_LEN+1);

			int fd[2], fork_value;

			if(pipe(fd)){
				perror("pipe");
				exit(-1);
			}

			if((fork_value = fork()) == -1){
				perror("fork");
				exit(-1);
			}
			else if(fork_value == 0){
				if(dup2(fd[WRITE], STDOUT_FILENO) == -1){
					perror("dup2");
					exit(-1);
				}

				int new_fd[2];

				if(pipe(new_fd)){
					perror("pipe");
					exit(-1);
				} 

				if((fork_value = fork()) == -1){
					perror("fork");
					exit(-1);
				}
				else if(fork_value == 0){
					close(new_fd[READ]);
					if(dup2(new_fd[WRITE], STDOUT_FILENO) == -1){
						perror("dup2");
						exit(-1);
					}
					execlp("echo", "echo", strcat(password, tmp_account->salt), NULL);
					printf("ERROR!!!\n");
					close(new_fd[WRITE]);
					exit(-1);
				}

				close(new_fd[WRITE]);
				waitpid(fork_value, NULL, 0);
				
				dup2(new_fd[READ], STDIN_FILENO);

				execlp("sha256sum", "sha256sum", NULL);
				printf("ERROR!!!\n");
		        close(fd[WRITE]);
		        exit(-1);
			}
			else{
				close(fd[WRITE]);
				waitpid(fork_value, NULL, 0);
				while(read(fd[READ], hash, HASH_LEN) == 0);
				close(fd[READ]);
			}
			
			if(strcmp(hash, tmp_account->hash) == 0){ //Found first account
				account = tmp_account;
				if(account.balance < balance){
					free(hash);
					free(string);
					write(STDOUT_FILENO, "OPERATION FAILED: Insufficient Money!!!\n", 40);
					return;
				}
			}
			else{
				free(hash);
				free(string);
				write(STDOUT_FILENO, "OPERATION FAILED: Invalid Password!!!\n", 38);
				return;				
			}

			free(hash);
		}
		else if(tmp_account.id == new_account_id){
			new_account = tmp_account;
		}

		if(account.id != 0 && new_account.id != 0)
			break;
	}

	free(string);

	if(account.id == 0 || new_account.id == 0){
		write(STDOUT_FILENO, "OPERATION FAILED: Account Does Not Exist!!!\n", 44);
	}

	if(close(fd)){
		perror("close");
		exit(-1);
	}	


	//FIND ACCOUNTS AND TRANSFER THE MONEY

	if(pthread_mutex_unlock(save_account_mutex)){
		perror("pthread_mutex_unlock");
		exit(-1);
	}
}

void save_account(bank_account_t* account){
	if(pthread_mutex_lock(save_account_mutex)){
		perror("pthread_mutex_lock");
		exit(-1);
	}
	//Critical Section
	int fd = open("accounts.txt", O_WRONLY | O_APPEND);
	if(fd < 0){
		perror("open");
		exit(-1);
	}
	
	char* uint32_string = malloc(sizeof(uint32_t)+sizeof(char));
	sprintf(uint32_string, "%d\n", account->account_id);
	write(fd, uint32_string, strlen(uint32_string));
	
	char* string = subst_null_by_endline(account->hash);
	write(fd, string, HASH_LEN+1);
	free(string);

	string = subst_null_by_endline(account->salt);
	write(fd, string, HASH_LEN+1);
	free(string);

	memset(uint32_string, '\0', sizeof(uint32_t)+sizeof(char));
	sprintf(uint32_string, "%d\n", account->balance);
	write(fd, uint32_string, strlen(uint32_string));

	free(uint32_string);

	if(close(fd)){
		perror("close");
		exit(-1);
	}

	if(pthread_mutex_unlock(save_account_mutex)){
		perror("pthread_mutex_unlock");
		exit(-1);
	}
}

char* subst_null_by_endline(char* string){ //This function will only be called by the account saver
	char* new_string = malloc(HASH_LEN+1);
	new_string = memcpy(new_string, string, HASH_LEN);
	new_string[HASH_LEN] = '\n';
	return new_string;
}
