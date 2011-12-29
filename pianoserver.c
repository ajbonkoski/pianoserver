#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define DEBUG 0
#if DEBUG
  #define DEBUG1(format)             printf((format))
  #define DEBUG2(format, arg1)       printf((format), (arg1))
  #define DEBUG3(format, arg1, arg2) printf((format), (arg1), (arg2))
#else
  #define DEBUG1(format) 
  #define DEBUG2(format, arg1) 
  #define DEBUG3(format, arg1, arg2) 
#endif


#define MAX_BUF_SZ 255
#define PIANO_PIPE_IN  "/tmp/piano_pipe_in"
#define PIANO_FILE_OUT "/tmp/piano_output"
#define PIANO_EXEC_NAME "pianobar"
#define DAEMON_CONTROL_CHAR '%'
int pianobar_pid = 0;
#define is_pianobar_running() (pianobar_pid)
int pianobar_stdin;
int pianobar_stdout;
char buf[MAX_BUF_SZ+1];


char *get_input(int fd){
  int numread = read(fd, buf, MAX_BUF_SZ);
  if(!numread)
    return NULL;
  if(buf[numread-1] == '\n')
    --numread;
  buf[numread] = 0;
  return buf;
}

void forward_pianobar_output(int fd_out){
  char buf[MAX_BUF_SZ+1];
  int numread;

  if(!is_pianobar_running())
    return;

  do {
    numread = read(pianobar_stdout, buf, MAX_BUF_SZ);
    if(numread > 0)
      write(fd_out, buf, numread);
  } while(numread > 0);

}

void start_pianobar(){
  if(is_pianobar_running()){
    fprintf(stderr, "Error: pianobar is already running...\n");
    return;
  }

  printf("starting pianobar...\n");

  int in[2];
  int out[2];
  if(pipe(in) == -1){
    fprintf(stderr, "Pipe Failed.\n"); 
    exit(1);
  }
  if(pipe(out) == -1){
    fprintf(stderr, "Pipe Failed.\n"); 
    exit(1);
  }
  
  pianobar_pid = fork();
  if(pianobar_pid == -1){
    fprintf(stderr, "Fork Failed.\n"); 
    exit(1);
  }

  if(pianobar_pid == 0){  /* new process */
    /* close unused ends */
    close(in[1]);
    close(out[0]);

    /* Attach stdin */
    close(0);
    dup(in[0]);

    /* Attach stdout */
    close(1);
    dup(out[1]);

    execlp(PIANO_EXEC_NAME, PIANO_EXEC_NAME, NULL);

  }else{ /* prev process */
    /* close unused ends */
    close(in[0]);
    close(out[1]);

    pianobar_stdin  = in[1];
    pianobar_stdout = out[0];
    /* configure the pianobar_stdout for non-blocking reads */
    fcntl(pianobar_stdout, F_SETFL, O_NONBLOCK);
  }

}


void stop_pianobar(){
  printf("stopping pianobar...\n");
  if(!is_pianobar_running()){
    fprintf(stderr, "Error: pianobar is not running...\n");
  }else{
    kill(pianobar_pid, SIGKILL);
    pianobar_pid = 0;
  }
}

void restart_pianobar(){
    printf("restarting pianobar...\n");
    stop_pianobar();
    start_pianobar();
}

void handle_daemon_control(char *in){
  if(strcmp(in, "start") == 0 || strcmp(in, "g") == 0)
    start_pianobar();
  else if(strcmp(in, "restart") == 0 || strcmp(in, "re") == 0)
    restart_pianobar();
  else if(strcmp(in, "stop") == 0 || strcmp(in, "q") == 0)
    stop_pianobar();
  else
    fprintf(stderr, "Error: could not recognize daemon control string: '%s'\n", in);
}

void handle_piano_control(char *in){
  if(strcmp(in, "q") == 0){
    stop_pianobar();
    return;
  }

  if(!is_pianobar_running()){
    fprintf(stderr, "Error: pianobar is not running... command will be dropped..\n");
    return;
  }
  
  /* write to pianobar's stdin */
  write(pianobar_stdin, in, strlen(in));

}

void handle_input(char *in){
  int len = strlen(in);
  if(!len) return;
  if(in[0] == DAEMON_CONTROL_CHAR)
    handle_daemon_control(in+1);
  else
    handle_piano_control(in);
}

void str_toupper(char *str){
  for (; *str; ++str)
      *str = toupper(*str);
}

int main(int argc, char *argv[]){
  int ret_val;

  ret_val = mkfifo(PIANO_PIPE_IN, 0666);
  if((ret_val == -1) && (errno != EEXIST)){
    perror("Error creating named pipe");
    exit(1);
  }

  
  /*
  ret_val = mkfifo(PIANO_PIPE_OUT, 0666);
  if((ret_val == -1) && (errno != EEXIST)){
    perror("Error creating named pipe");
    exit(1);
  }
  */

  /* Open pipes*/
  int fd_in, fd_out;
  if((fd_in= open(PIANO_PIPE_IN,  O_RDONLY | O_NONBLOCK)) == -1){
    fprintf(stderr, "Error: Could not open named input pipe\n");
    exit(1);
  }
  if((fd_out = creat(PIANO_FILE_OUT, 0644)) == -1){
    fprintf(stderr, "Error: Could not open named input pipe\n");
    exit(1);
  }
  
  
  while(1){
    forward_pianobar_output(fd_out);

    char *buf = get_input(fd_in);
    if(buf){
      DEBUG2("*** NOTE *** Piano Server : Read From the pipe : %s\n", buf);
      handle_input(buf);
    }
   
    usleep(100000);
  }

  return 0;
}
  
