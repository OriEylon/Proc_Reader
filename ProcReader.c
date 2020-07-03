#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_PATH 200


struct Sample_args
{
    int sample_every;
    char path[MAX_PATH];
};

/* function declaration */
char* getFilename(char* path);
void* sample_loop(void* args);
void sampleCmd();
void printHelp();
void fdCmd();
void memCmd();
void trdCmd();


int main(int argc, char const *argv[])
{
    char action[MAX_PATH];

    printf("welcome to /proc reader :)\n");

    do{
        printf("choose an action: (choose help for manual/quit to exit)\n");
        memset(action,'\0',MAX_PATH);
        scanf("%s",action);
        
        // menu:
        if(strcmp(action,"quit")==0){printf("goodbye\n");exit(0);}
        else if(strcmp(action,"proc")==0){
            int retVal;
            if((retVal=system("ls /proc"))<0){
                printf("failed ls cmd\n%s\n",strerror(errno));exit(1);
            }
        }
        else if(strcmp(action,"help")==0){printHelp();}
        else if(strcmp(action,"sample")==0){sampleCmd();}
        else if(strcmp(action,"fd")==0){fdCmd();}
        else if(strcmp(action,"memory")==0){memCmd();}
        else if(strcmp(action,"threads")==0){trdCmd();}
        else {printf("command not valid; try again\n");}
    }
    while(1);

    return 0;
}

void* sample_loop(void* args)
{
    // gets the path, opens a file based on path and writes value of "cat PATH" and time to file, each SAMPLE_EVERY

    struct Sample_args *sArgs = (struct Sample_args*)args;
    int fd;
    char filename[MAX_PATH];
    char timeCmd[MAX_PATH] = "cat /proc/driver/rtc | grep rtc_time >> ";
    char cmd[MAX_PATH];
    char *path = malloc(MAX_PATH * sizeof(char));
    strcpy(path, sArgs->path);

    memset(filename,'\0',MAX_PATH);
    strcat(filename,getFilename(path));
    
    strcat(timeCmd, filename);
    memset(cmd,'\0',MAX_PATH);

    if((fd=open(filename,O_CREAT|O_WRONLY,S_IRWXU))<0){
        printf("couldn't open file: %s\n",strerror(errno));
        free(path);
        exit(1);
    }

    strcat(cmd,"cat ");
    strcat(cmd,sArgs->path);
    strcat(cmd,">>");
    strcat(cmd,filename);
    
    printf("sampling every %d seconds until you quit, data goes into %s\n",sArgs->sample_every,filename);

    while(1){
        if(system(timeCmd) != 0){
            printf("problem printing time: %s\n",strerror(errno));
            unlink(filename);
            free(path);
            exit(1);
        }

        if(system(cmd) != 0){
            printf("problem executing cmd: %s\n",strerror(errno));
            unlink(filename);
            free(path);
            exit(1);
        }
        sleep(sArgs->sample_every);
    }

}

void sampleCmd()
{
    // get path to sample & how much to sample from user, then open new thread and pass info to thread.

    char path[MAX_PATH];
    int sample_every=0;

    memset(path,'\0',MAX_PATH);
    printf("please enter path to sample:\n");
    scanf("%s",path);
    // add tests for path
    do{
        printf("how much samples a second?\n");
        scanf("%d",&sample_every);
        if(sample_every<=0){
            printf("invalid entry; try again:\n");
        }
    }while(sample_every<=0);
    
    struct Sample_args *s1 = malloc(sizeof(struct Sample_args));
    s1->sample_every=sample_every;
    strcpy(s1->path,path);
    
    pthread_t smpltrd;
    int retVal;
    if((retVal = pthread_create(&smpltrd, NULL, sample_loop, (void*) s1)<0)){
        printf("couldn't create thread: %s",strerror(errno));exit(1);
    }
}

char* getFilename(char* path){
    const char delim[] = "/";
    char *fname = malloc(MAX_PATH * sizeof(char));
    memset(fname,'\0',MAX_PATH);
    char* token;

    token = strtok(path, delim);
    strcat(fname, token);

    //read whole file path, excluding "/"
    while((token = strtok(NULL, delim)) != NULL){
        strcat(fname, "_");
        strcat(fname, token);
    }
    //add file extension
    strcat(fname,".csv");
    return fname;
}

void printHelp()
{
    //prints the menu
    printf(" 1.\"proc\" - display the /proc filesystem\n");
    printf(" 2.\"sample\" - sample data from a path you provide into a corresponding file\n");
    printf(" 3.\"fd\" - display all procceses which have more than X fd's allocated\n");
    printf(" 4.\"memory\" - display all procceses which have more than X memory allocated\n");
    printf(" 5.\"threads\" - display all procceses which have more than X threads open\n");
    printf(" 6.\"quit\" to quit\n");
}

void fdCmd()
{
    // loop through procs, if # of fd's available > than limit , echo pid

    // for pid in /proc/[1-9]*; do if (($(sudo ls $pid/fd | wc -l) > 100 )); then echo $pid; fi; done
    char cmd[MAX_PATH];
    int limit;
    printf("display any procces that has more than this much fd's allocated:");
    scanf("%d",&limit);
    sprintf(cmd,"for pid in /proc/[1-9]*; do if [ $(sudo ls $pid/fd | wc -l) -gt %d ]; then echo $pid; fi; done",limit);
    system(cmd);
}

void memCmd()
{
    // loop through /proc . for each proccess running, get the virtual memory allocated, and if its more than "limit" (user input) display pid
    char cmd[MAX_PATH];
    int limit;
    printf("display any procces that has more than this much memory allocated (in kB):");
    scanf("%d",&limit);
    sprintf(cmd,"for pid in /proc/[1-9]*;do val=$(grep -i vmsize $pid/status | grep -o '[0-9]*') ; if [ $? -eq 0 ]; then if [ $val -ge %d ]; then echo $pid; fi; fi; done",limit);
    system(cmd);
}

void trdCmd()
{
    // loop through /proc . for each proccess running, get # of threads, and if its more than "limit" (user input) display pid
    char cmd[MAX_PATH];
    int limit;
    printf("display any procces that has more than this much threads:");
    scanf("%d",&limit);
    sprintf(cmd,"for pid in /proc/[1-9]*;do val=$(grep -i threads $pid/status | grep -o '[0-9]*') ; if [ $? -eq 0 ]; then if [ $val -ge %d ]; then echo $pid; fi; fi; done",limit);
    system(cmd);    

}