// The MIT License (MIT)
// 
// Copyright (c) 2016 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 15     // Mav shell only supports 15 arguments

//define history length 
#define MAX_HISTORY_LENGTH 15

char *hist[MAX_HISTORY_LENGTH];// A global array of pointers to store the command history
int commands = 0; //integer to store the number of commands in the history


int pid_array [MAX_HISTORY_LENGTH]; //global array to store the process IDs of executed commands
int pid_counter = 0; //integer variable to store the number of process IDs in the array

void add_to_hist(char *command);
void display_history();
void execute_command (char **token, int token_count);

// Function to add a command to the history

void add_to_hist(char *command)
{
  if (commands < MAX_HISTORY_LENGTH) // if there is space in the history array, add the command to the end of the array
  {
    hist [commands] = strdup(command); //duplicating the command 
    commands++;
  }
  else //if the history array is full, shift all the commands back by one and add the new command to the end
  {
    for (int i = 0; i < MAX_HISTORY_LENGTH - 1; i++)
    {
      free(hist[i]); //freeing the previous allocated memory
      hist[i] = strdup(hist[i+1]); //shifting the array
    }
    free (hist[(MAX_HISTORY_LENGTH) - 1]); //freeing the memory of the last element
    hist[(MAX_HISTORY_LENGTH) - 1] = strdup(command); //adding the new command
  }
}

// Function to display the command history
void display_history()
{
  for (int i = 0; i < commands; i++) //loop through the history array and print out each command
  {
    printf("%d: %s", i, hist[i]); 
  }
}

//Funtion to take array of tokens & token counter, and execute the stated commands
void execute_command (char **token, int token_count)
{
  if (token_count == 0) //if no tokens, return
  {
    return;
  }

  if (strcmp("cd", token[0]) == 0) //executing cd
  {
    if (chdir(token[1]) == -1) //if chdire =-1, directory not found
    {
      printf("%s: Directory not found. \n", token[1]);
    }
    pid_array[pid_counter] = -1; //add process id to the pid_array & ++ pid_counter
    pid_counter++;
  } 
  //if token is quit or exit, exit shell
  else if (strcmp("quit", token[0]) == 0 || strcmp("exit", token[0]) == 0) 
  {
    exit(0);
  }
  else if (strcmp("showpids", token[0]) == 0) //display list if pids
  {
    pid_array[pid_counter] = -1; //add pid to the pid_array & ++ pid_counter
    pid_counter++;

    for (int i = 0; i < commands; i++) //for loop for commands and pids
    {
      printf("%d: %d\n", i, pid_array[i]);
    }
   
  }
  else if (strcmp("history", token[0]) == 0) //execute history
  {
    pid_array[pid_counter] = -1;
    pid_counter++;
    display_history();
  }
  else if (strcmp("!", token[0]) == 0) //if "!", execute a command from command history 
  {
      /*if (token_count <2 )
      {
        printf("error. missing history index");
      }*/

    int index = atoi(token[1]); //index of command to execute from the 2nd token
    if (index <=0 || index > commands)//if index is invalid
    {
      printf("Command not found.\n");
    }
    else //if index is valid, get the command from history and execute it. 
    { 
      char *command = hist[index - 1]; //add command to history 
      add_to_hist(command);

      //tokenise command and execute it
      char *hist_token[MAX_NUM_ARGUMENTS]; 
      int hist_token_count = 0;
      char *hist_token_ptr = strtok(command, WHITESPACE);
      while (hist_token_ptr != NULL && hist_token_count < MAX_NUM_ARGUMENTS)
      {
        hist_token[hist_token_count] = hist_token_ptr;
        hist_token_count++;
        hist_token_ptr = strtok(NULL, WHITESPACE);
      }
      execute_command(hist_token, hist_token_count);
    }
    return;
  }
  else
  {
    //fork and execute command
    pid_t pid1 = fork();
    if (pid1 == -1)
    {
      perror("Fork Failed.");
      exit(EXIT_FAILURE);
    }
    else if (pid1 == 0)
    { 
      //child process
      if (execvp(token[0], token) == -1)
      {
        printf("%s: Command not found.\n", token[0]);
      }
      exit(0);
    }
    else
    {
      int status1;
      waitpid (pid1, &status1, 0);
      //parent process
      if (pid_counter < MAX_NUM_ARGUMENTS)
      {
        pid_array[pid_counter] = pid1;
        pid_counter++;
      }
      else
      {
        for (int i = 0; i < MAX_HISTORY_LENGTH - 1; i++)
        {
          pid_array[i] = pid_array[i+1];
        }
        //free (pid_array[(MAX_HISTORY_LENGTH) - 1]);
        pid_array[(MAX_HISTORY_LENGTH) - 1] = pid1;
      }
      //int status1;
      //waitpid (pid1, &status1, 0);
    }
  }
}

int main()
{

  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );

    add_to_hist(command_string);

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      token[i] = NULL;
    }

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr = NULL;                                         
                                                           
    char *working_string  = strdup( command_string );                

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }
    
    execute_command(token, token_count); //calling the function to execute the inputed/pre-existing commands

    // blank
    //if (strcmp("\0", token[0])==0 || strcmp(" ", token[0]) == 0)
    if(token[0] == NULL)
    {
      continue; 
    }

    //!
    /*int ex_counter = -1;
    if(strcmp("!", token[0]) == 0)
    {
      char *ex_str = token[1];
      ex_counter = atoi(ex_str);

      if((ex_counter > 0) && (ex_counter <= commands))
      {
        strcpy(*token, hist[(ex_counter) - 1]);
      }
      else
      {
        printf("Error: Invalid history index\n");
        continue; 
      }
    }*/
    
    // Cleanup allocated memory
    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      if( token[i] != NULL )
      {
        free( token[i] );
      }
    }

    free( head_ptr );

  }

  free( command_string );

  return 0;
  
}