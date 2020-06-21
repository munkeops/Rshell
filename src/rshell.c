#include<stdio.h>
#include<stdlib.h>
#include <sys/wait.h> 
#include <unistd.h> 
#include <string.h> 
#include <limits.h>
#include <fcntl.h>

FILE * commandsfptr;
struct current_pids *head_pid;

struct current_pids{
	int pid;
	struct current_pids *next;
};


int insert_pid(int pid){
	struct current_pids *node = (struct current_pids *)malloc(
		sizeof(struct current_pids));
	struct current_pids *loop_node;

	node->pid = pid;
	node->next = NULL;

	/* Go through everything */
	loop_node = head_pid;

	while (loop_node->next != NULL){
		loop_node = loop_node->next;
	}

	loop_node->next = node;

	return 1;
}

int delete_pid(int pid){
	struct current_pids *loop_node;
	struct current_pids *prev;

	loop_node = head_pid;
	prev = head_pid;

	while(loop_node != NULL){

		if (loop_node->pid == pid){
			prev->next = loop_node->next;
			free(loop_node);
			break;
		}

		prev = loop_node;
		loop_node = loop_node->next;
	}

	return 1;
}



#define RSH_RL_BUFSIZE 1024
char *rsh_read_line(void)
{
  int bufsize = RSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "rsh: allocation error\n");
    exit(EXIT_FAILURE);
  }


  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += RSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "rsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}


#define RSH_TOK_BUFSIZE 64
char **rsh_split_line(char *line)
{
    #define RSH_TOK_DELIM " \t\r\n\a"

  int bufsize = RSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "rsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  commandsfptr = fopen("commandslist2.txt","a");
  
  fprintf(commandsfptr,"%s\n",line);
  fclose(commandsfptr);
  token = strtok(line, RSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += RSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "rsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, RSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int rsh_check_piping(char** command)
{
    char *pipe = strstr(command, "|");
    

    if(pipe != NULL)
    {
      return 1;
    }
    else
    {
        return 0;
    }
}

char ** rsh_piped(char* line)
{
    #define RSH_TOK_DELIM "|"

    int bufsize = RSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "rsh: allocation error\n");
        exit(EXIT_FAILURE);
    }
    token = strtok(line, RSH_TOK_DELIM);

    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
        bufsize += RSH_TOK_BUFSIZE;
        tokens = realloc(tokens, bufsize * sizeof(char*));
        if (!tokens) {
            fprintf(stderr, "rsh: allocation error\n");
            exit(EXIT_FAILURE);
        }
        }

        token = strtok(NULL, RSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}


int rsh_check_redirection(char** command)
{
    char *out = strstr(command, ">");
    char *in = strstr(command, "<");

    if((out != NULL) && (in != NULL))
    {
        //both inut and output redirection
        return 3;
    }
    else if(out != NULL)
    {
        //output redirection only
        return 2;
    }
    else if(in != NULL)
    {
        //input redirection only
        return 1;
    }
    else
    {
        return -1;
    }
}

char ** rsh_redirected(char* line,int type)
{
    #define RSH_TOK_DELIM "<>"

    int bufsize = RSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "rsh: allocation error\n");
        exit(EXIT_FAILURE);
    }
    token = strtok(line, RSH_TOK_DELIM);

    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
        bufsize += RSH_TOK_BUFSIZE;
        tokens = realloc(tokens, bufsize * sizeof(char*));
        if (!tokens) {
            fprintf(stderr, "rsh: allocation error\n");
            exit(EXIT_FAILURE);
        }
        }

        token = strtok(NULL, RSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void output_redirect(char *s,char ** args)
{
    //printf("%s\n",s);
    int out = open(s, O_RDWR|O_CREAT|O_APPEND, 0600);
    if (-1 == out) { perror("opening cout.log"); return 255; }

    int err = open("cerr.log", O_RDWR|O_CREAT|O_APPEND, 0600);
    if (-1 == err) { perror("opening cerr.log"); return 255; }

    int save_out = dup(fileno(stdout));
    int save_err = dup(fileno(stderr));

    if (-1 == dup2(out, fileno(stdout))) { perror("cannot redirect stdout"); return 255; }
    if (-1 == dup2(err, fileno(stderr))) { perror("cannot redirect stderr") ; return 255; }

    // puts("doing an ls or something now");
    if (execvp(args[0], args) == -1) {
      perror("rsh");
    }
    exit(EXIT_FAILURE);
    fflush(stdout); close(out);
    fflush(stderr); close(err);

    dup2(save_out, fileno(stdout));
    dup2(save_err, fileno(stderr));

    close(save_out);
    close(save_err);
}


int rsh_launch(char **args,char** args_redirected,int type)
{
  pid_t pid, wpid;
  int status;
  FILE *fptr;
  fptr =fopen("pidhist.txt","a");
  int i=0,background=0;
  while(args[i]!=NULL)
  {
     i++;
  }
  if(strncmp(args[i-1],"&",1)==0)
  {
    args[i-1]=NULL;
    background=1;
  }
  pid = fork();
  if (pid == 0) {
    // Child process
    //char * output=execvp(args[0], args);
    //printf("%s\n",output);
    if(type==3)
    {
      //printf("%s\n",args_redirected[2]);
      output_redirect(args_redirected[2],args);
    }
    else if(type == 2)
    {
      //printf("%s\n",args_redirected[1]);
      output_redirect(args_redirected[1],args);
    }
    else
    {
      if (execvp(args[0], args) == -1) {
      perror("rsh");
      }
      exit(EXIT_FAILURE);
    }
         
  } 
  else if (pid < 0)
  {
    // Error forking
    perror("rsh");
  }
  else
  {
    // Parent process
    fprintf(fptr,"%s %d\n",args[0],pid);
    fclose(fptr);
    if(background>0)
    {
      insert_pid(pid);
    }
    else
    {
      wait(NULL);
    }
    
  }

  return 1;
}

void sigchld_handler(int signum){
	pid_t pid;
	pid = waitpid(-1, NULL, WNOHANG);

	if ((pid != -1) && (pid != 0)){
		//printf("  [~%d]\n", pid);
		delete_pid(pid);
	}
	return;
}


/*
  Function Declarations for builtin shell commands:
 */
int rsh_cd(char **args);
int rsh_help(char **args);
int rsh_exit(char **args);
int rsh_hist(char **args);
int rsh_pid();



/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "hist",
  "pid"
};

int (*builtin_func[]) (char **) = {
  &rsh_cd,
  &rsh_help,
  &rsh_exit,
  &rsh_hist,
  &rsh_pid
};

int rsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int rsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "rsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("rsh");
    }
  }
  return 1;
}

int rsh_help(char **args)
{
  int i;
  printf("Rohan's RSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < rsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int rsh_exit(char **args)
{
  return 0;
}

int rsh_hist(char **args)
{
  pid_t pid, wpid;
  int status;
  FILE *fptr;
  fptr =fopen("pidhist.txt","a");
  pid =fork();
  if(pid==0)
  {
    int lines=0;
    
    FILE * fp;
    fp=fopen("commandslist2.txt","r");
    fseek(fp, 0, SEEK_SET);
  
    char str1[30]; 
    int counter=1; 
    while(fscanf(fp, "%[^\n]", str1)==1)
    {     
      //if(counter>(lines-(args[0][4]-'0')))
      printf("%d. %s \n",counter,str1);
      fseek(fp, SEEK_CUR, SEEK_SET+1);
      counter++;
                   
    }

    fclose(fp);
    exit(EXIT_SUCCESS);
  }
  else if(pid<0)
  {
    fprintf(stderr,"error in forking \n");
    return EXIT_FAILURE;

  }
  else
  {
    fprintf(fptr,"hist %d\n",pid);
    do
    {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
   

  }
  fclose(fptr);
  return 1;
}

int rsh_histn(char **args,int num)
{
  pid_t pid, wpid;
  int status;
  FILE *fptr;
  fptr =fopen("pidhist.txt","a");
  pid =fork();

  if(pid==0)
  {
    int lines=0;

    FILE * fp;
    fp=fopen("commandslist2.txt","r");
    fseek(fp, 0, SEEK_SET);


    
    char str1[30]; 
    int counter=1; 
    while(fscanf(fp, "%[^\n]", str1)==1)
      {     
        
        fseek(fp, SEEK_CUR, SEEK_SET+1);
        counter++;
                    
      }
      int bound=counter;
      counter=0;
    fseek(fp, 0, SEEK_SET);

    while(fscanf(fp, "%[^\n]", str1)==1)
      {     
        if(counter>=(bound-num-1))
        {
              //printf("%d %d \n",counter,num);
              printf("%d. %s \n",counter-bound+num+2,str1);
        }
        fseek(fp, SEEK_CUR, SEEK_SET+1);
        counter++;
                    
      }

      fclose(fp);
      exit(EXIT_FAILURE);
  }
  else if (pid<0)
  {
    fprintf(stderr,"error in forking \n");
    return EXIT_FAILURE;
  }
  else
  {
    fprintf(fptr,"histn %d\n",pid);
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
   

  }
  fclose(fptr);
  return 1;
}

int rsh_pid()
{
  printf("command name : ./a.out   process id : %d\n",getpid());
  return 1;
}

int rsh_pid_all()
{
  FILE * fptr=fopen("pidhist.txt","r");
  char str1[100];
  while(fscanf(fptr, "%[^\n]", str1)==1)
  {
    char *token = strtok(str1," ");
    char pname[20];
    strcpy(pname,token);
    token=strtok(NULL," ");
    char pid[20];
    strcpy(pid,token);


    printf("command name : %s   process id : %s\n",pname,pid);
    fseek(fptr, SEEK_CUR, SEEK_SET+1);
  }
  fclose(fptr);
  return 1;
}

int rsh_pid_current()
{
  struct current_pids *loop_node;

	loop_node = head_pid;

	while(loop_node != NULL)
  {
		printf("PID:\t%d\n", loop_node->pid);
		loop_node = loop_node->next;
	}
  return 1;
}



int rsh_execute(char **args,char ** args_redirected,int type)
{
  //printf("hi\n");
  int i;
  int num=0;
  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  if(strlen(args[0])==5)
  {
    if (args[0][0]=='h'&&args[0][1]=='i'&&args[0][2]=='s'&&args[0][3]=='t')
    {
      num=args[0][4]-'0';
      //strncpy(args[0],"hist",strlen("hist"));
    }
  }
  
  //printf("%d",num);
  if(num>0)
        return rsh_histn(args,num);
  else if((strncmp(args[0],"pid",strlen("pid"))==0 )&&(args[1]!=NULL)&& (strncmp(args[1],"all",strlen("all"))==0))
    return rsh_pid_all();
  else if((strncmp(args[0],"pid",strlen("pid"))==0 )&&(args[1]!=NULL)&& (strncmp(args[1],"current",strlen("current"))==0))
    return rsh_pid_current();
  else
  {
    for (i = 0; i < rsh_num_builtins(); i++) {
      if (strcmp(args[0], builtin_str[i]) == 0) {
        
          return (*builtin_func[i])(args);
      }
    }
  }
  return rsh_launch(args,args_redirected,type);
}
int rsh_execute_pipe(char**args_redirected);

int rsh_loop(void)
{

  int redirect_type,pipe_exist=0;
  char *line;
  char **args;
  char **args_redirected;
  int status;
  char cwd[PATH_MAX];
  char cwd2[PATH_MAX];

  char hostname[HOST_NAME_MAX];
  char username[LOGIN_NAME_MAX];
  gethostname(hostname, HOST_NAME_MAX);
  getlogin_r(username, LOGIN_NAME_MAX);
  if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
      perror("getcwd() error");
      return 1;
    }
    FILE *fptr;
    fptr =fopen("pidhist.txt","a");
    fprintf(fptr,"./a.out %d\n",getpid());
    fclose(fptr);



  do {
    if (getcwd(cwd2, sizeof(cwd2)) == NULL)
    {
      perror("getcwd() error");
      return 1;
    }
    printf("%s@%s:~%s> ",username,hostname,cwd2+strlen(cwd));
    line = rsh_read_line();
    pipe_exist=rsh_check_piping(line);
    
    if(pipe_exist==0)
    {
      redirect_type = rsh_check_redirection(line);
      args_redirected=rsh_redirected(line,redirect_type);
      if(redirect_type<0 || redirect_type==2)
      {
          args = rsh_split_line(args_redirected[0]);
      }
      else
      {  
          char str1[40];
          char str2[40];
          strncpy(str1,args_redirected[0],strlen(args_redirected[0]));
          strncpy(str2,args_redirected[1],strlen(args_redirected[1]));
          strcat(strcat(str1," " ),str2);
          args=rsh_split_line(str1);
      }  
      status = rsh_execute(args,args_redirected,redirect_type);
    }
    else
    {
      args_redirected=rsh_piped(line);
      status=rsh_execute_pipe(args_redirected);
    }
    
    

    free(line);
    free(args);
  } while (status);
  fclose(commandsfptr);
  remove("commandslist2.txt");
  remove("pidhist.txt");

}

int rsh_execute_pipe(char**args_redirected)
{
  pid_t pid1,pid2,wpid;
  int status;

  int mypipe[2];
  if (pipe(mypipe))
  {
      fprintf(stderr,"failed to create a pipe");
      return EXIT_FAILURE;
  }

  pid1=fork();
  
  if(pid1==0)
  {
     char* args1[10];
           
    char *token3=strtok(args_redirected[0]," ");
    
    int i=0;
    while (token3!=NULL)
    {
        args1[i]=token3;
        i++;
        token3=strtok(NULL," ");
    }
    args1[i]=NULL;
    close(mypipe[0]);
    dup2(mypipe[1],1);
    close(mypipe[1]);
    execvp(args1[0],args1); 
    exit(EXIT_SUCCESS);
  }
  else if(pid1<0)
  {
      fprintf(stderr,"error in forking \n");
      return EXIT_FAILURE;
  }
  else
  {
    do
      {
        wpid = waitpid(pid2, &status, WUNTRACED);
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  pid2=fork();
  
  if(pid2==0)
  {
    char* args1[10];
           
    char *token3=strtok(args_redirected[1]," ");
    
    int i=0;
    while (token3!=NULL)
    {
        args1[i]=token3;
        i++;
        token3=strtok(NULL," ");
    }
    args1[i]=NULL;
    close(mypipe[1]);
    dup2(mypipe[0],0);
    close(mypipe[0]);
    scanf("%s",&args1[i]);
    args1[i+1]=NULL;
    printf("%s %s",args1[0],args1[1]);
    execvp(args1[0],args1); 
    exit(EXIT_SUCCESS);
  }
  else if(pid2<0)
  {
      fprintf(stderr,"error in forking \n");
      return EXIT_FAILURE;
  }
  else
  {
    do
      {
        wpid = waitpid(pid2, &status, WUNTRACED);
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  
return 1;
}


int rsh_clean_up(){
	//printf("Cleaning up...\n");

	struct current_pids *loop_node;
	struct current_pids *next;

  remove("commandslist2.txt");


	loop_node = head_pid;

	while(loop_node != NULL){
		next = loop_node->next;
		free(loop_node);
		loop_node = next;
	}

	return 1;
}


int main(int argc, char **argv)
{
  // Load config files, if any.
  head_pid = (struct current_pids *)malloc(sizeof(struct current_pids));
	head_pid->pid = getpid();
	head_pid->next = NULL;


	signal(SIGCHLD, sigchld_handler);

  // Run command loop.
  rsh_loop();

  // Perform any shutdown/cleanup.
  rsh_clean_up();
  return EXIT_SUCCESS;
}