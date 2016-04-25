#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 

// MACROS
#define MAX_CHARACTERS_PER_COMMAND 256 // Maximum number of characters in individual command.
#define MAX_CHARACTERS_PER_TOKEN 20 // Maximum number of characters in individual term (token) in command.
#define MAX_TOKENS_PER_COMMAND 20 // Maximum number of terms (tokens) in individual command.
#define COMMAND_HISTORY_SIZE 10 // Number of commands saved in history.

// GLOBAL VARIABLES
char input_command[MAX_CHARACTERS_PER_COMMAND]; // String containing command which will be initialized using input from user. Array rather than pointer so we can use as input to fgets.
char command_history[COMMAND_HISTORY_SIZE][MAX_CHARACTERS_PER_COMMAND]; // Array of pointers to commands in history.
int command_history_position_counter=0; // Index of current command in history (after 10 commands, remains at 9);

// FUNCTION PROTOTYPES (indented if function is used within previous function. For example, display_command_promt(), save_user_input(), and replace_newline_with_null() are all used within take_input_from_user()).
void exit_if_command_is_ctrl_C();
	void ctrl_C(int);
void take_input_from_user();
	void display_command_prompt();
	void save_user_input();
	void replace_newline_with_null(); // 
void put_command_in_history(); // Executed every time user inputs command
	int command_history_full();
	void shift_history(); // Executed when attempting to put command in history when history is full. Removes oldest command and shifts more recent commands back in history, in order to make space for new command.
	void put_command_in_next_empty_slot_in_history();
void exit_if_command_is_exit();
void print_history_if_command_is_history();
	void print_history(); // Prints current command history
void tokenize_and_execute_input_command();
	pid_t fork_process();
		void exit_process_if_fork_error(pid_t);
	void make_parent_wait_for_child_to_complete(pid_t);
	int break_into_IO_terms(char *, char **);
		int current_char_of_command_is_symbol(char *, int);
		void save_token_to_the_left_of_current_character(char *, char *command_tokenized[MAX_TOKENS_PER_COMMAND+1], int, int, int);
			char *substring(char *, int, int); // For input string, returns substring with specified start and end indices.
		void save_current_symbol(char *, char *command_tokenized[MAX_TOKENS_PER_COMMAND+1], int, int);
		void set_next_token_to_start_after_current_character(int, int *);
		int reached_end_of_command(char *, int);
		void save_last_token_in_command(char *, char *command_tokenized[MAX_TOKENS_PER_COMMAND+1], int, int, int);
		int current_char_of_command_is_space(char *, int);
		void add_null_token_to_end_of_command(char **, int);
	void determine_IO_type_and_execute(char *command_tokenized[MAX_TOKENS_PER_COMMAND+1], int number_of_IO_terms);
		void extract_commands_only(char *IO_commands[MAX_CHARACTERS_PER_COMMAND+1], char *IO_terms[MAX_CHARACTERS_PER_COMMAND+1], int);
		void extract_symbols_only(char *IO_symbols[MAX_CHARACTERS_PER_COMMAND+1], char *IO_terms[MAX_CHARACTERS_PER_COMMAND+1], int);
		int compute_number_pipelines(char *IO_symbols[MAX_CHARACTERS_PER_COMMAND+1], int);
		void execute(char *IO_commands[MAX_TOKENS_PER_COMMAND], int);
			int tokenize(char *, char **); // Breaks string into tokens, returning number of tokens


int main()
{
	while(1){ // Until loop is broken by either 'exit' or ctrl-c...
		exit_if_command_is_ctrl_C();
		take_input_from_user();
		put_command_in_history();
		exit_if_command_is_exit();
		print_history_if_command_is_history();
		tokenize_and_execute_input_command();
	}
}

// Sets up signal handler at every iteration of shell prompt.
void exit_if_command_is_ctrl_C(){
	signal(SIGINT, ctrl_C);
}

// Signal handler called whenever ctrl-C is pressed
void ctrl_C(int a){
	printf("\n");
	exit(0);
}

// Executed at every iteration of shell prompt
void take_input_from_user(){
	display_command_prompt();
	save_user_input();
	replace_newline_with_null();
}

// Display command prompt in preparation for user input
void display_command_prompt(){
	printf("Gabe> ");
}

// Save user input as input_command
void save_user_input(){
	fgets(input_command,MAX_CHARACTERS_PER_COMMAND,stdin);
}

// input_command contains newline character at the end, so we remove it
void replace_newline_with_null(){
	input_command[strlen(input_command)-1]='\0';
}

// Save command in history array
void put_command_in_history(){
	if (command_history_full()){
		shift_history();
	}
	put_command_in_next_empty_slot_in_history();
}

// Returns 1 if history contains 10 commands, and 0 if it contains fewer than 10
int command_history_full(){
	return (command_history_position_counter==(COMMAND_HISTORY_SIZE));
}

// Remove oldest command in history and shift others back to make room for new command. Executed only if history is full.
void shift_history(){
	for (int i=0; i<(COMMAND_HISTORY_SIZE-1); i++)
		strcpy(command_history[i], command_history[i+1]);
	command_history_position_counter--;
}

// Called only when there is at least one empty slot. If history is full, shift_history() called first, and empty slot will be 10th.
void put_command_in_next_empty_slot_in_history(){
	strcpy(command_history[command_history_position_counter], input_command);
	command_history_position_counter++;
}

// Called before command would be saved in history
void exit_if_command_is_exit(){
	if (strncmp(input_command, "exit", 4)==0)
		exit(0);
}

// Called after command saved in history, so history command itself will appear in history display
void print_history_if_command_is_history(){
	if (strncmp(input_command, "history", 7)==0)
		print_history();
}

// Display contents of history array, from oldest to newest
void print_history(){
	printf("\n");
	for (int i=0; i<command_history_position_counter; i++){
		printf("%s\n", command_history[i]);
	}
	printf("\n");
}

// Fork child process that will execute command
void tokenize_and_execute_input_command(){
	pid_t pid=fork_process();
	make_parent_wait_for_child_to_complete(pid);
	if (pid==0){ // For child process...
		char *IO_terms[MAX_CHARACTERS_PER_COMMAND+1]; // Create array to hold IO terms
		int number_of_IO_terms=break_into_IO_terms(input_command, IO_terms); // Break command into IO terms
		determine_IO_type_and_execute(IO_terms, number_of_IO_terms); // Execute command based on these terms
	}
}

// Forks process and checks for forking error
pid_t fork_process(){
	pid_t pid=fork();
	exit_process_if_fork_error(pid);
	return pid;
}

// Called whenever process is forked
void exit_process_if_fork_error(pid_t pid){
	if (pid < 0){
		perror("fork");
		printf("main function: errno number is %d\n",errno);
		exit(pid);
	}
}

void make_parent_wait_for_child_to_complete(pid_t pid){
	if (pid>0)
		wait(NULL);
}

// Break input command into tokens, each of which is either a command that does not contain IO symbols, or an IO symbol
int break_into_IO_terms(char *command, char **command_tokenized){
	int current_index_in_command, starting_index_of_current_token=0, number_of_tokens=0;
	for (current_index_in_command=0; current_index_in_command<strlen(command); current_index_in_command++){ // For each character in command...
		if (current_char_of_command_is_symbol(command, current_index_in_command)){ // If current character is IO symbol (< > |)...
			save_token_to_the_left_of_current_character(command, command_tokenized, starting_index_of_current_token, current_index_in_command, number_of_tokens); // Save command preceding this symbol
			number_of_tokens++; // Increment number of tokens because we have added command preceding current symbol
			save_current_symbol(command, command_tokenized, current_index_in_command, number_of_tokens); // Save current IO symbol
			set_next_token_to_start_after_current_character(current_index_in_command, &starting_index_of_current_token); // Re-set starting point of next command in preparation to save command PROceeding current IO symbol
			number_of_tokens++; // Increment number of tokens because we have added current symbol
		}
		if (reached_end_of_command(command, current_index_in_command)){ // If we have reached the end of the command...
			save_last_token_in_command(command, command_tokenized, starting_index_of_current_token, current_index_in_command, number_of_tokens); // Save IO command preceding end of full command, ie last IO command
			number_of_tokens++; // Increment number of tokens because we have added final IO command
		}
	}
	int token_index, character_index;
	for (int token_index=0; token_index<number_of_tokens; token_index++){ // For each token (ie each IO terms, command or symbol)...
		char token_with_end_spaces_removed[50]; // Create string that will hold this token with any extraneous spaces at either end removed.
		strcpy(token_with_end_spaces_removed, command_tokenized[token_index]); // Copy token into temporary string
		if (current_char_of_command_is_space(command_tokenized[token_index], 0)){ // If leftmost character of token is space...
			strcpy(token_with_end_spaces_removed, substring(token_with_end_spaces_removed, 1, (strlen(token_with_end_spaces_removed)-1))); // Remove this space
		}
		if (current_char_of_command_is_space(token_with_end_spaces_removed, (strlen(token_with_end_spaces_removed)-1))){ // If rightmost character of token is space...
			strcpy(token_with_end_spaces_removed, substring(token_with_end_spaces_removed, 0, (strlen(token_with_end_spaces_removed)-2))); // Remove this space
		}
		strcpy(command_tokenized[token_index], token_with_end_spaces_removed); // Save token with spaces removed back in token array
	}
	add_null_token_to_end_of_command(command_tokenized, number_of_tokens); // Add a null string to the end of list of commands, because execvp() requires this
	return number_of_tokens;
}

// Called while scanning through command to identify IO command and symbols, returns 1 if current character is <, >, or |
int current_char_of_command_is_symbol(char * command, int current_index_in_command){
	return ((command[current_index_in_command]=='<')||(command[current_index_in_command]=='>')||(command[current_index_in_command]=='|'));
}

// Called while scanning through command to identify IO command and symbols, when IO symbol (< > |) is found, save command preceding this symbol
void save_token_to_the_left_of_current_character(char *command, char *command_tokenized[MAX_TOKENS_PER_COMMAND+1], int starting_index_of_current_token, int current_index_in_command, int number_of_tokens){
	command_tokenized[number_of_tokens]=substring(command, starting_index_of_current_token, current_index_in_command-1);
}

// Returns substring of command between indices a and b
char *substring(char *command, int a, int b){
	char *substr=malloc(MAX_CHARACTERS_PER_COMMAND*sizeof(char));
	for (int i=0; i<=(b-a); i++){
		substr[i]=command[a+i];
	}
	return substr;
}

// Called while scanning through command to identify IO command and symbols, when IO symbol (< > |) is found, save this symbol in list of IO symbols.
void save_current_symbol(char *command, char *command_tokenized[MAX_TOKENS_PER_COMMAND+1], int current_index_in_command, int number_of_tokens){
	command_tokenized[number_of_tokens]=substring(command, current_index_in_command, current_index_in_command);
}

// Called while scanning through command to identify IO command and symbols, after IO symbol (< > |) is found, we will save preceding command and prepare to search for command PROceeding symbol.
void set_next_token_to_start_after_current_character(int current_index_in_command, int *starting_index_of_current_token){
	*starting_index_of_current_token=current_index_in_command+1;
}

// Called while scanning through command to identify IO command and symbols, returns 1 when we have reached end of command, and 0 otherwise
int reached_end_of_command(char *command, int current_index_in_command){
	return (current_index_in_command==(strlen(command)-1));
}

// Called while scanning through command to identify IO command and symbols, if we have reached the end of the command, save last IO command
void save_last_token_in_command(char *command, char *command_tokenized[MAX_TOKENS_PER_COMMAND+1], int starting_index_of_current_token, int current_index_in_command, int number_of_tokens){
	command_tokenized[number_of_tokens]=substring(command, starting_index_of_current_token, current_index_in_command);
}

// Called while scanning through command to identify IO command and symbols, return 1 if current command is space, 0 otherwise
int current_char_of_command_is_space(char * command, int current_index_in_command){
	return (command[current_index_in_command]==' ');
}

// Called while scanning through command to identify IO command and symbols, when we have saved all IO commands, adds a NULL token to the end of this array since execvp requires this
void add_null_token_to_end_of_command(char **command_tokenized, int number_of_tokens){
	command_tokenized[number_of_tokens]=NULL;
}

// Analyze the type of IO command (IO redirection, pipe, etc) based on IO symbols, and execute command accordingly
void determine_IO_type_and_execute(char *IO_terms[MAX_CHARACTERS_PER_COMMAND+1], int number_of_IO_terms){ // a>b>c>D
	int number_of_IO_commands=(number_of_IO_terms+1)/2, number_of_IO_symbols=number_of_IO_commands-1;
	char *IO_commands[MAX_TOKENS_PER_COMMAND];
	extract_commands_only(IO_commands, IO_terms, number_of_IO_commands);
	char *IO_symbols[MAX_TOKENS_PER_COMMAND];
	extract_symbols_only(IO_symbols, IO_terms, number_of_IO_symbols);
	int number_of_pipelines=compute_number_pipelines(IO_symbols, number_of_IO_symbols);

	if (number_of_pipelines==0){ // If command contains no pipes or IO redirection...
		execute(IO_commands, 0); // Simply execute command as is
	}

	// Create and pipe file descriptors
	int file_descriptors[number_of_IO_symbols][2]; // Create a pair of file descriptors for each pipeline
	for (int file_descriptor_index=0; file_descriptor_index<number_of_IO_symbols; file_descriptor_index++){ // For each pipeline...
		if (pipe(file_descriptors[file_descriptor_index]) < 0){ // Pipe this pipeline
			perror("fatal error");
		    exit(1);
		}
	}

	int status; // Int which will be used by child processes to signal a change in their state for their parent.

	for (int command_index=0; command_index<number_of_IO_commands; command_index++){ // For each IO command...
		if (fork_process()==0){ // Fork process
			if (command_index>0){ // If current command is not first IO command (ie there is an IO symbol to its left)...
				if (IO_symbols[command_index-1][0]=='|') // If command is preceded by pipe...
					dup2(file_descriptors[command_index-1][0], STDIN_FILENO); // Redirect input to be from lefthand pipe
			}
			if (command_index<(number_of_IO_commands-1)){ // If current command is not last IO command (ie there is an IO symbol to its right)...
				if (IO_symbols[command_index][0]=='|') // If command is proceeded by pipe...
					dup2(file_descriptors[command_index][1], STDOUT_FILENO); // Redirect output to be from righthand pipe
				if (IO_symbols[command_index][0]=='>'){ // If command is proceeded by output redirection symbol...
					int output_file = open(IO_commands[command_index+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
					dup2(output_file, STDOUT_FILENO); // Redirect output to be to file specified after > symbol
				}
				if (IO_symbols[command_index][0]=='<'){ // If command is proceeded by input redirection symbol...
					int input_file=open(IO_commands[command_index+1], O_RDONLY);
					dup2(input_file, STDIN_FILENO); // Redirect input to be from file specified after > symbol
					if (number_of_IO_symbols>1){ // If there is also output redirection...
						if (IO_symbols[command_index+1][0]=='>'){
							int output_file = open(IO_commands[command_index+2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
							dup2(output_file, STDOUT_FILENO); // Redirect output to be to file specified after > symbol
						}
					}
				}
			}
			// Close file descriptors
			for (int j=0; j<number_of_pipelines; j++){
				close(file_descriptors[j][0]);
				close(file_descriptors[j][1]);
			}

			execute(IO_commands, command_index); // Execute current command, given that its input and output have been redirected appropriately	
		}
	}
	// Close file descriptors
	for (int j=0; j<number_of_pipelines; j++){
		close(file_descriptors[j][0]);
		close(file_descriptors[j][1]);
	}
	for (int i = 0; i < number_of_IO_commands; i++) // For each child process executing an IO command...
    	wait(&status); // Make parent wait for change in child's state

	exit(0); // Exit main child process running all other processes
}

// Take IO_terms, which contains all IO commands and symbols, and extract commands only
void extract_commands_only(char *IO_commands[MAX_TOKENS_PER_COMMAND], char *IO_terms[MAX_TOKENS_PER_COMMAND], int number_of_IO_commands){
	IO_commands[0]=IO_terms[0];
	if (number_of_IO_commands>1){
		for (int i=0; i<number_of_IO_commands; i++){
			IO_commands[i+1]=IO_terms[2*i+2];
		}
	}
}

// Take IO_terms, which contains all IO commands and symbols, and extract symbols only
void extract_symbols_only(char *IO_symbols[MAX_TOKENS_PER_COMMAND], char *IO_terms[MAX_TOKENS_PER_COMMAND], int number_of_IO_symbols){
	if (number_of_IO_symbols>0){
		for (int i=0; i<number_of_IO_symbols; i++){
			IO_symbols[i]=IO_terms[2*i+1];
		}
	}
}

// Take IO_symbols, which contains list of all IO symbols in order, and compute number of pipelines needed (1 for each |, and only 1 if there is any number of < and/or >)
int compute_number_pipelines(char *IO_symbols[MAX_CHARACTERS_PER_COMMAND+1], int number_of_IO_symbols){
	int pipes=0;
	for (int i=0; i<number_of_IO_symbols; i++){
		if ((IO_symbols[i][0]=='<')||(IO_symbols[i][0]=='>')){
			return 1;
		} else{
			if (IO_symbols[i][0]=='|')
				pipes++;
		}
	}
	return pipes;
}

// Execute given command, where command will not contain any IO symbols
void execute(char *IO_commands[MAX_TOKENS_PER_COMMAND], int index_of_command_to_execute){
	char *command[MAX_TOKENS_PER_COMMAND+1];
	int number_of_tokens=tokenize(IO_commands[index_of_command_to_execute], command);
	execvp(command[0], command);
}

// Tokenize a given command that does not contain IO symbols. This will simply break a command into its command and arguments
int tokenize(char *command, char **command_tokenized){
	int current_index_in_command, starting_index_of_current_token=0, number_of_tokens=0;
	for (current_index_in_command=0; current_index_in_command<strlen(command); current_index_in_command++){ // For each character in command...
		if (current_char_of_command_is_space(command, current_index_in_command)){
			save_token_to_the_left_of_current_character(command, command_tokenized, starting_index_of_current_token, current_index_in_command, number_of_tokens);
			set_next_token_to_start_after_current_character(current_index_in_command, &starting_index_of_current_token);
			number_of_tokens++;
		} else if (current_char_of_command_is_symbol(command, current_index_in_command)){
			if (command[current_index_in_command+1]!=' '){	// if form is a|b
				save_token_to_the_left_of_current_character(command, command_tokenized, starting_index_of_current_token, current_index_in_command, number_of_tokens);
				number_of_tokens++;
				command_tokenized[number_of_tokens]=substring(command, current_index_in_command, current_index_in_command);
				set_next_token_to_start_after_current_character(current_index_in_command, &starting_index_of_current_token);
				number_of_tokens++;
			}
		}
		if (reached_end_of_command(command, current_index_in_command)){
			save_last_token_in_command(command, command_tokenized, starting_index_of_current_token, current_index_in_command, number_of_tokens);
			number_of_tokens++;
		}
	}
	add_null_token_to_end_of_command(command_tokenized, number_of_tokens);
	return number_of_tokens;
}