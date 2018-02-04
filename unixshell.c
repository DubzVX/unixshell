/*
Shell create for a practical work in germany erasmus
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

#define STOPPED 0
#define BACKGROUND 1
#define NO_REMOVE 0
#define REMOVE 1

pid_t fpid=0; // global variable to store the foreground process ID (used in the handlers functions)

//	structures
//-----------------------------------------------------

typedef struct Process Process;
struct Process
{
	pid_t pid;
	char *name;
	int status; // 0 if stopped, 1 if in background
	pid_t pgid;
	Process *next_process;
};

typedef struct Lifo_process Lifo_process;
struct Lifo_process
{
	Process *first_process;
};

Lifo_process *lifo; // global variable lifo to store stopped and background process (used in handlers functions)


//	lifo functions
//-----------------------------------------------------

Lifo_process *create_lifo()
{
	Lifo_process *lifo=malloc(sizeof(Lifo_process));
	lifo->first_process=NULL;

	return lifo;
}

void push_process_to_lifo(int pid, char *name, int status)
{
    Process *process = malloc(sizeof(Process));

    if (lifo==NULL || process==NULL)
    {
        exit(EXIT_FAILURE);
    }

    process->pid = pid;
    process->name = name;
    process->status = status;
    process->next_process = lifo->first_process;
    lifo->first_process = process;
}

Process *pop_last_process()
{
    if (lifo!=NULL && lifo->first_process!=NULL)
    {
    	Process *process = lifo->first_process;

        lifo->first_process=process->next_process;

        return process;
    }
    else
    {
    	return NULL;
    }
}

Process *get_last_process(int status, int remove)
{
	if(lifo!=NULL && lifo->first_process!=NULL)
	{
		Process *process_1=lifo->first_process;

		if(process_1->next_process==NULL)
		{
			if(process_1->status==status)
			{
	    		if(remove)
	    		{
	    			lifo->first_process=NULL;
	    		}
    			return process_1;
    		}
	    	else
	    	{
	    		return NULL;
	    	}
	    }
		else
		{
			if(process_1->status==status)
			{
	    		if(remove)
	    		{
	    			lifo->first_process=process_1->next_process;
	    		}
	    		return process_1;
			}
			else
			{
	   			Process *process_2=process_1;

		    	while(process_2!=NULL && process_2->status!=status)
		    	{
		    		process_1=process_2;
		    		process_2=process_2->next_process;
		    	}

		    	if(process_2!=NULL && process_2->status==status)
		    	{
		    		if(remove)
		    		{
		    			process_1->next_process=process_2->next_process;
		    		}
		    		return process_2;
		    	}
		    	else
		    	{
		    		return NULL;
		    	}
		    }
	    }
	}
	else
	{
		return NULL;
	}
}

void print_jobs()
{
	char *status;

	if(lifo!=NULL && lifo->first_process!=NULL)
	{
		Process *process=lifo->first_process;
		printf("\tPID        Command     Status\n");
		
		while(process!=NULL)
		{
			status = process->status ? "Background" : "Stopped";
			printf("\t[%d]     %-10s  %s\n", process->pid, process->name, status);
			process=process->next_process;
		}
	}
}

void print_lifo()
{
	char *status;

	if(lifo!=NULL)
	{
		if(lifo->first_process!=NULL)
		{
			Process *process=lifo->first_process;

			while(process!=NULL)
			{
				status = process->status ? "Background" : "Stopped";
				printf("[%d] %s %s -> ", process->pid, process->name, status);
				process=process->next_process;
			}
		}
		printf("NULL\n\n");
	}
	else
	{
		printf("LISTE VIDE\n\n");
	}
}


// handlers functions
//-----------------------------------------------------

void handle_SIGINT(int signum)
{
	if(fpid!=0)
	{
		kill(fpid, SIGINT);
		fpid=0;
	}
	printf("  [caught SIGINT]\n");
}


void handle_SIGTSTP(int signum)
{
	if(fpid!=0)
	{
		kill(fpid, SIGTSTP);
		fpid=0;
	}
	printf("  [caught SIGTSTP]\n");
}

void handle_SIGCHLD(int signal)
{
	pid_t pid;

	if(lifo!=NULL && lifo->first_process!=NULL)
	{
		Process *process_1 = lifo->first_process;

		if(process_1->next_process==NULL)
		{
			if(process_1->status==BACKGROUND)
			{
				pid = process_1->pid;
				pid = waitpid(pid, 0, WNOHANG);
				if(pid!=-1 && pid!=0)
				{
					lifo->first_process = NULL;
					free(process_1);
				}
			}
		}
		else
		{			
			if(process_1->status==BACKGROUND)
			{
				pid = process_1->pid;
				pid = waitpid(pid, 0, WNOHANG);
				if(pid!=-1 && pid!=0)
				{
					lifo->first_process = process_1->next_process;
					free(process_1);
				}
			}
			else
			{
				Process *process_2 = process_1->next_process;

				while(process_2!=NULL)
				{
					if(process_2->status==BACKGROUND)
					{
						pid = process_2->pid;
						pid = waitpid(pid, 0, WNOHANG);
						if(pid!=-1 && pid!=0)
						{
							process_1->next_process = process_2->next_process;
							free(process_2);
							break;
						}
					}
					process_1 = process_2;
					process_2 = process_2->next_process;
				}
			}
		}
	}
}


//	shell functions
//-----------------------------------------------------

char *get_command(void)
{
    char *command;
    ssize_t command_size=0;
    
    printf("> ");
    getline(&command, &command_size, stdin);

    return command;
}


char **split_command(char* command)
{
	int parameters_size = 32;
	int index = 0;
	char **parameters = malloc(parameters_size * sizeof(char *));
	char *parameter;
	
	parameter=strtok(command, " \n");

	while(parameter!=NULL)
	{
		parameters[index]=parameter;
		index++;

		if(index>=parameters_size)
		{
			parameters_size+=32;
			parameters=realloc(parameters, parameters_size * sizeof(char *));
		}
		parameter=strtok(NULL, " \n");
	}
	parameters[index]=NULL;

	return parameters;
}

int get_array_last_item_index(char** array) 
{
    int i;
 
    for(i = 0; array[i] != '\0'; i++);
 
    return --i;
}

void launch_command(char** command_parameters)
{
	int last_index, status;
	pid_t pid, wait_pid;


	last_index=get_array_last_item_index(command_parameters);

    pid=fork(); //Duplicate process

    if(!strcmp(command_parameters[last_index], "&"))
    {
    	//Background command

    	command_parameters[last_index]='\0'; // get rid of "&"

    	if(pid==0)
		{
			//Child process

			if(execvp(command_parameters[0], command_parameters)==-1)
			{
				printf("  Unknown command!\n> ");
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			//Parent process

			if(setpgid(pid, pid)==-1)
			{
				perror("setpgid");
			}

			fpid=0;

			push_process_to_lifo(pid, command_parameters[0], BACKGROUND);
			printf("  [%d] %s  now in background!\n", pid, command_parameters[0]);
		}
    }
    else
    {
    	//Usual command

		if(pid==0)
		{
			//Child process

			if(execvp(command_parameters[0], command_parameters)==-1)
			{
				printf("  Unknown command!\n");
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			//Parent process

			if(setpgid(pid, pid)==-1)
			{
				perror("setpgid");
			}

			fpid=pid;
			
      		waitpid(pid, &status, WUNTRACED); // wait for the child process to finish or being stopped

    		if(WIFSTOPPED(status)) // if CTRL-Z hit (waitpid function stopped)
    		{
    			push_process_to_lifo(pid, command_parameters[0], STOPPED);
    		}
		}
	}
}


//	main
//-----------------------------------------------------

int main(int argc, char *argv[])
{
	char *command=NULL;
	char **command_parameters=NULL;
	int logout=0;
	int status;
	Process *process=malloc(sizeof(Process));

	Lifo_process *lifo_2 = create_lifo(); // create a lifo to store the stopped or backgroung process
	lifo = lifo_2; // set this lifo as global (will be usedin the handlers functions)

	// set up the signal handlers 
	signal(SIGINT, handle_SIGINT);
	signal(SIGTSTP, handle_SIGTSTP);
	signal(SIGCHLD, handle_SIGCHLD);

	while(!logout)
	{
		command=get_command();
	    command_parameters = split_command(command);

	    if(command_parameters[0]!=NULL) //
	    {
	    	if(!strcmp(command_parameters[0], "exit")) // force the shell to exit even if there are unfinished jobs (used for debugging)
	    	{
	    		logout=1;
	    		break;
	    	}
	    	if(!strcmp(command_parameters[0],"pl")) // command to print the content of lifo
	    	{
	    		print_lifo();
	    	}
	    	else if(!strcmp(command_parameters[0], "jobs"))
	    	{
	    		print_jobs(); // print stopped and background process
	    	}
		    else if(!strcmp(command_parameters[0],"logout"))
		    {
		    	if(lifo->first_process==NULL)
		    	{
		    		logout=1; // logout if no process stopped or running in background
		    	}
		    	else
		    	{
		    		printf("  Unfinished Jobs :\n");
		    		print_jobs(); // else print stopped and background process
		    		printf("  Cannot logout!\n");
		    	}
		    }
		    else if(!strcmp(command_parameters[0],"fg"))
		    {
		    	process=pop_last_process(); //get last process stopped or put in background process

		    	if(process!=NULL)
		    	{
		    		fpid=process->pid;

			    	kill(process->pid, SIGCONT); // resume this process
			    	
			    	printf("  [%d] %s  now in foreground!\n", process->pid, process->name);

	      		    waitpid(process->pid, &status, WUNTRACED); // wait for the child process to finish or being stopped

		    		if(WIFSTOPPED(status)) // If CTRL-Z hit (waitpid function stopped)
		    		{
		    			push_process_to_lifo(process->pid, process->name, STOPPED); // add this stopped process to lifo
		    		}

		    		free(process);
		    	}
		    	else
		    	{
		    		printf("  No process to put in foreground!\n");
		    	}
		    }
		    else if(!strcmp(command_parameters[0],"bg"))
		    {
		    	process=get_last_process(STOPPED, REMOVE); // get the last stopped process and remove it from lifo 

		    	if(process!=NULL)
		    	{
		    		fpid=0;

		    		push_process_to_lifo(process->pid, process->name, BACKGROUND); // add this process running in background to lifo

		    		kill(process->pid, SIGCONT); // resume the (stopped) process 

		    		printf("  [%d] %s  now in background!\n", process->pid, process->name);

			    	free(process);
			    }
			    else
			    {
			    	printf("  No process to put in background!\n");	
			    }
		    }
		    else
		    {
		    	launch_command(command_parameters); // execute the typed command
		    }
		}
	}
	
	exit(EXIT_SUCCESS);

	return 0; 
}
