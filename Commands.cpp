#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include "stdlib.h"
#include <list>
#include <fcntl.h>

using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";
#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif


string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}


int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = nullptr;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}
void remove_extra_mid_spaces_1(char* old_cmd, char* new_cmd)

{
    int input_index = 0;
    int output_index = 0;
    while(old_cmd[input_index] != '\0')
    {
        new_cmd[output_index] = old_cmd[input_index];
        if(old_cmd[input_index] == ' ')
        {
            while(old_cmd[input_index + 1] == ' ')
            {
                input_index++;
            }
        }
        input_index++;
        output_index++;
    }
    new_cmd[output_index] = '\0';
}
void remove_extra_mid_spaces_2(char* old_cmd, char* new_cmd)
{
    int in_index = 0;
    int out_index = 0;
    while(old_cmd[in_index] != '\0')
    {

       if(old_cmd[in_index] == ' ')
        {
            while(old_cmd[in_index+1] == ' ')
            {
                in_index++;
            }
            in_index++;
        }
        new_cmd[out_index]=old_cmd[in_index];
        in_index++;
        out_index++;
    }
    new_cmd[out_index] = '\0';
}

Command * SmallShell::CreateCommand(const char* cmd_line,int* is_external,char** last_pwd) {

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    char *args[COMMAND_MAX_ARGS];
    // todo init to null
    int temp;
    temp = cmd_s.length() + 1;
     char* in = strdup(cmd_s.c_str());
    int args_num_first=0;
    char* out=new char[cmd_s.length()+1];
    remove_extra_mid_spaces_1(in,out);
    int args_num = _parseCommandLine(out, args);
    this->redirected=false;
    this->piped=false;
    signal_piped=false;
    int redirect_index=-1;
   // bool is_back_ground=_isBackgroundComamnd(cmd_line);

    if(((redirect_index=cmd_s.find(">"))!=string::npos)||(cmd_s.find(">>")!=string::npos)) {
        this->redirected=true;
        return new RedirectionCommand(cmd_line);
    }
    if ((cmd_s.find("|") == -1) &&

        (cmd_s.find("|&") == -1)) {

        if (firstWord.compare("pwd") == 0 || firstWord.compare("pwd&") == 0) {




            return new GetCurrDirCommand(cmd_line);
        } else if (firstWord.compare("showpid") == 0 || firstWord.compare("showpid&") == 0) {



            return new ShowPidCommand(cmd_line);
        } else if (firstWord.compare("chprompt") == 0 || firstWord.compare("chprompt&") == 0) {
            if (args[1]) {


                return new chpromptCommand(cmd_line, args[1]);
            } else {


                return new chpromptCommand(cmd_line, "smash");
            }
        } else if (firstWord.compare("kill") == 0 || firstWord.compare("kill&") == 0) {


            return new KillCommand(cmd_line, (this->jobs_list));
        } else if (firstWord.compare("cd") == 0 || firstWord.compare("cd&") == 0) {


            return new ChangeDirCommand(cmd_line, last_pwd);
        } else if (firstWord.compare("bg") == 0 || firstWord.compare("bg&") == 0) {


            return new BackgroundCommand(cmd_line, getInstance().jobs_list);
        } else if (firstWord.compare("fg") == 0 || firstWord.compare("fg&") == 0) {


            return new ForegroundCommand(cmd_line, (this->jobs_list));
        } else if (firstWord.compare("quit") == 0 || firstWord.compare("quit&") == 0) {


            return new QuitCommand(cmd_line, (this->jobs_list));
        } else if (firstWord.compare("jobs") == 0 || firstWord.compare("jobs&") == 0) {


            return new JobsCommand(cmd_line, getInstance().jobs_list);
        }

        else if (firstWord.compare("cat") == 0 || firstWord.compare("cat&") == 0) {

            return new CatCommand(cmd_line);

        }
        else if(firstWord.compare("timeout")==0)
        {
            return new TimeOutCommand(cmd_line);
        }

        else {
            *is_external = 1;
            return new ExternalCommand(cmd_line);
        }
    }

    else {

        if ((cmd_s.find("|") != -1) ||

            (cmd_s.find("|&") != -1)) {
            signal_piped=true;
            args_num_first=0;
            piped=true;
            return new PipeCommand(cmd_line,cmd_s.find("&") ==1, args, args_num, args_num_first);

        }

    }


    return nullptr;
}

//execute_command

void SmallShell::executeCommand(const char* cmd_line,bool flag,bool piped_son ){
    SmallShell::getInstance().son_piped=piped_son;
    char* args[COMMAND_MAX_ARGS];
    int num_arg = _parseCommandLine(cmd_line,args);
    if(num_arg==0) {
       // _deleteArgs(cmd_line,args);
        return;
    }
    //_deleteArgs(cmd_line,args);
    int is_external_cmd=0;
    int len = strlen(cmd_line)+1;
    char* cmd_line_t = strdup(cmd_line);
   // cmd_line_t [len-1]='\0';
    Command* cmd = CreateCommand(cmd_line_t,&is_external_cmd,&(this->current_pwd));
    if(!this->first_red)
        this->current_cmd=cmd;
    if(cmd == nullptr) {
        delete cmd_line_t;
        return;

    }
    bool is_background_cmd = _isBackgroundComamnd(cmd_line_t);
    if(is_external_cmd==0)
    {
        if (!_isBackgroundComamnd(cmd_line)) {
            this->current_cmd=cmd;
            cmd->execute();
            if(!piped_son)
                this->jobs_list->removeFinishedJobs();
            current_pid=0;
            SmallShell::getInstance().son_piped=false;
        }

        else {

            if(!piped&&!redirected)
                this->jobs_list->addJob(cmd,(pid_t)getpid());
            this->current_cmd=cmd;
            cmd->execute();
            if(!piped_son)
                this->jobs_list->removeFinishedJobs();
            current_pid=0;
            SmallShell::getInstance().son_piped=false;
        }
    }
    else{
        if(is_background_cmd){
            _removeBackgroundSign(cmd_line_t);
        }
        if(!this->first_red)
            this->current_cmd=cmd;
        if(flag){
            char* cmd_line_temp=new char[strlen(cmd_line_t)+1];
            strcpy(cmd_line_temp,cmd_line_t);
            char bin_bash[10] ="/bin/bash";
            char c[3] ="-c";
            char* const exe_args[] = {bin_bash,c,cmd_line_temp, nullptr};
            if(execv("/bin/bash" , exe_args) == -1)
                printf("didn't work\n");
            delete cmd_line_t;
            return;
        }
        pid_t pid;
        if(!flag)
            pid=fork();
        if(pid==0){
            if(!flag)
                setpgrp();
            char* cmd_line_t_t=new char[strlen(cmd_line_t)+1];
            strcpy(cmd_line_t_t,cmd_line_t);
            char bash[5] ="bash";
            char c[3] ="-c";
            char* const exe_args[] = {bash,c,cmd_line_t_t,nullptr};
            if(execv("/bin/bash" , exe_args) == -1)
                printf("didn't work\n");
        }
        else if(pid>0){ // father process

            if(is_background_cmd) {

                if(this->first_red){
                    this->jobs_list->addJob(current_cmd,pid,false,true);
                }
                else
                    this->jobs_list->addJob(cmd,pid);
            }
            else{
                this->current_pid = pid;
                delete [] this->cmd_line;

                this->cmd_line = new char[strlen(cmd_line)+1];
                strcpy(this->cmd_line,cmd_line);
                waitpid(pid, nullptr,WUNTRACED);
                current_pid = 0;
                if(!piped_son)
                    this->jobs_list->removeFinishedJobs();
                SmallShell::getInstance().son_piped=false;
            }
        }
        else{
            perror("fork() failed");
            delete cmd_line_t;
            exit(-1);
        }
    }
}

//remove finished jobs before every command
/*
void SmallShell::executeCommand(const char *cmd_line) {

     Command* cmd = CreateCommand(cmd_line, nullptr, nullptr);
    if(cmd == nullptr)
        return;
     cmd->execute();

}
*/
//chpromptCommand

chpromptCommand::chpromptCommand(const char* cmd_line,std::string name):BuiltInCommand(cmd_line) ,name("smash"){
    this->name=name;
}
void chpromptCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.shell_name=this->name;
}
/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/
//ShowPidCommand
ShowPidCommand::ShowPidCommand(const char* cmd_line):BuiltInCommand(cmd_line)
{
    int pid=getpid();
    this->process_pid=pid;
}
void ShowPidCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    cout<<"smash pid is "<<smash.smash_pid<<endl;
}


/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/

//ChangeDirCommand
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** last_Pwd):BuiltInCommand(cmd_line) {
    if (!*last_Pwd)
        return;
    else {
        length = strlen(*last_Pwd);
        this->last_pwd = new char[length + 1];
        strcpy(this->last_pwd, *last_Pwd);
    }
}
void ChangeDirCommand::execute() {

    char* args[COMMAND_MAX_ARGS];
    int args_num = _parseCommandLine(cmd_line,args);
    if ( args_num ==1 )
    {
        return;
    }
    if(args_num > 2) {
        cerr<<"smash error: cd: too many arguments"<<endl;
        return;
    }
    char curr_dir[COMMAND_ARGS_MAX_LENGTH];
    SmallShell& shell = SmallShell::getInstance();
    string  lastDirection_Sign = "-";
    char* lastDirection = shell.last;

    if(args[1]!=lastDirection_Sign) {
        char* tmp_dir = getcwd(curr_dir,COMMAND_ARGS_MAX_LENGTH);
        if (chdir(args[1]) != 0){
            perror("smash error: chdir failed");
            return;
        }
        else{
            delete [] shell.last;
            int x=strlen(tmp_dir);
            shell.last = new char [x+1];
            strcpy(shell.last,tmp_dir);
        }
    }
    else{
        if(shell.last == nullptr) {
            cerr<<"smash error: cd: OLDPWD not set"<<endl;
            return;
        }
        char* tmp_dir  = getcwd(curr_dir,COMMAND_ARGS_MAX_LENGTH);
        if (chdir(lastDirection) != 0){
            perror("smash error: chdir failed");
            return;
        }
        else{
            delete [] shell.last;
            int x=strlen(tmp_dir);
            shell.last= new char [x+1];
            strcpy(shell.last,tmp_dir);
        }
    }
}


/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/



//GetCurrDirCommand
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line):BuiltInCommand(cmd_line){}
void GetCurrDirCommand::execute() {
    char path[COMMAND_ARGS_MAX_LENGTH];
    if(getcwd(path, COMMAND_ARGS_MAX_LENGTH)){
        cout << path << endl;
    }
    else{
        perror("smash error: getcwd failed");
    }
}
/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/

//KillCommand
KillCommand::KillCommand(const char* cmd_line, JobsList* jobs):BuiltInCommand(cmd_line){
    this->jobsList=jobs;
}
void KillCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    if(!smash.son_piped)
        smash.jobs_list->removeFinishedJobs();
    char* args[COMMAND_MAX_ARGS];
    int num_of_args=_parseCommandLine(this->cmd_line,args);
    if(num_of_args!=3){
        cerr<<"smash error: kill: invalid arguments"<< endl;
        return ;
    }
    int signum= strtol(args[1], nullptr, 0);
    int job_id= strtol(args[2], nullptr, 0);

    if(job_id < 0){
        cerr<<"smash error: kill: job-id " <<job_id<<" does not exist"<< endl;
        return ;
    }
    if(job_id == 0){
        cerr<<"smash error: kill: invalid arguments"<< endl;
        return ;
    }
    if(signum == 0){
        cerr<<"smash error: kill: invalid arguments"<< endl ;
        return ;
    }
    signum = signum * (-1);
    if(signum<0){
        cerr<<"smash error: kill: invalid arguments"<< endl;
        return ;
    }
    JobsList::JobEntry* job = this->jobsList->getJobById(job_id);
    if(job == nullptr){
        cerr<<"smash error: kill: job-id " <<job_id<<" does not exist"<< endl;
        return;
    }
    pid_t job_pid = job->pid;
    if(job->piped) {
        if (killpg(job_pid, signum) != 0) {
            perror("smash error: kill failed");
            return;
        }
    }
    else{
        if (kill(job_pid, signum) != 0) {
            perror("smash error: kill failed");
            return;
        }
    }
    std::cout<<"signal number "<<signum<<" was sent to pid "<<job_pid<<std::endl;
    if(!smash.son_piped)
        smash.jobs_list->removeFinishedJobs();
}

/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/

//BackgroundCommand
BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs):jobID(0),BuiltInCommand(cmd_line),jobsList(nullptr){
    this->jobsList=jobs;
}
void BackgroundCommand::execute() {

    jobsList->removeFinishedJobs();
    char* args[COMMAND_MAX_ARGS];
    int args_num = _parseCommandLine(this->cmd_line,args);
    if(args_num > 2) {
        cerr<<"smash error: bg: invalid arguments"<<endl;
        return;
    }
    JobsList::JobEntry* job;
    int job_id;
    if(args_num == 2) {
        job_id = strtol(args[1], nullptr, 0);
        if (job_id < 0) {
            cerr<<"smash error: bg: job-id "<<job_id<<" does not exist"<<endl;
            return;
        }
        if (job_id == 0) {
            cerr<<"smash error: bg: invalid arguments"<<endl;
            return;
        }
        job = this->jobsList->getJobById(job_id);
        if (job == nullptr) {
            cerr<<"smash error: bg: job-id "<<job_id<<" does not exist"<<endl;
            return;
        } else if (!(this->jobsList->getJobById(job_id)->is_stopped)) {
            cerr<<"smash error: bg: job-id "<<job_id<<" is already running in the background"<< endl;
            return;
        }
    }
    else {
        if(jobsList->jobs->empty()){
            cerr<<"smash error: bg: there is no stopped jobs to resume"<<endl;
            return;
        }
        job = this->jobsList->getLastStoppedJob(&job_id);
        if(job == nullptr){
            cerr<<"smash error: bg: there is no stopped jobs to resume"<<endl;
            return;
        }
    }
    job->unstop();
    cout<<job->cmd_line<<" : "<<job->pid<<endl;
}

/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/
//ForegroundCommand
ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs):job_id(0),BuiltInCommand(cmd_line),jobsList(nullptr){
    this->jobsList=jobs;
}
void ForegroundCommand::execute() {
    if(!SmallShell::getInstance().son_piped)
        jobsList->removeFinishedJobs();

    char* args[COMMAND_MAX_ARGS];
    int args_num = _parseCommandLine(this->cmd_line,args);
    if(args_num > 2){
        cerr<<"smash error: fg: invalid arguments"<<endl;
        return;
    }
    JobsList::JobEntry *job;
    int job_id;
    if(args_num == 2) {
        job_id= strtol(args[1], nullptr, 0);
        if (job_id == 0) {
            cerr<<"smash error: fg: invalid arguments"<<endl;
            return;
        }
        if (job_id < 0) {
            cerr<<"smash error: fg: job-id " <<job_id<<" does not exist"<<endl;
            return;
        }
        job= this->jobsList->getJobById(job_id);
        if (job == nullptr) {
            cerr<<"smash error: fg: job-id " <<job_id<<" does not exist"<<endl;
            return;
        }
    }
    else{
        if(jobsList->jobs->empty()){
            cerr<<"smash error: fg: jobs list is empty"<<endl;
            return;
        }
        job = this->jobsList->getLastJob(&job_id);
    }

    if((this->jobsList->getJobById(job_id)->is_stopped))
        job->unstop();
    if(this->jobsList->getJobById(job_id)->piped)
        SmallShell::getInstance().signal_piped=true;

    pid_t pid =job->pid;
    cout<<job->cmd_line<<" : "<<job->pid<<endl;
    SmallShell::getInstance().current_pid =  pid;
    delete [] SmallShell::getInstance().cmd_line;
    int len=strlen(job->cmd_line)+1;
    SmallShell::getInstance().cmd_line = new char[len];
    strcpy(SmallShell::getInstance().cmd_line,job->cmd_line);
    waitpid(pid, nullptr,WUNTRACED);

}
/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/

//QuitCommand
QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs):BuiltInCommand(cmd_line){
    this->job_list=jobs;
}
void QuitCommand::execute() {

    char* args[COMMAND_MAX_ARGS];
    int argument_num=_parseCommandLine(this->cmd_line,args);
    if(argument_num<2){
        exit(0);
    }
    if(strcmp(args[1],"kill")==0){
        this->job_list->removeFinishedJobs();
        std::cout<<"smash: sending SIGKILL signal to "<< this->job_list->jobs->size() <<" jobs:"<<std::endl;
        this->job_list->killAllJobs();
    }
    exit(0);
}

/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/
//ExternalCommand
ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line){}
void ExternalCommand::execute ()
{
    SmallShell& smash = SmallShell::getInstance();
    char* args[COMMAND_MAX_ARGS + 1];
    bool isBackground= _isBackgroundComamnd(cmd_line);
    pid_t pid=fork();
    _parseCommandLine(cmd_line, args);
    if(pid==-1){
        perror("smash error: fork failed");
        return;
    }
    if(pid==0){
        setpgrp();
        char cmd[strlen(cmd_line)+1];
        strcpy(cmd,cmd_line);
        if(_isBackgroundComamnd(cmd)){
            _removeBackgroundSign(cmd);
        }
        char bin_bash[10] ="/bin/bash";
        char c[3] ="-c";
        char* argv[]={bin_bash,c,cmd, nullptr};
        int res=execv("/bin/bash",argv);
        if(res==-1){
            perror("smash error: execv failed");
        }
        exit(0);
    }else{
        if(!isBackground){
            SmallShell& smash = SmallShell::getInstance();
            smash.current_pid = pid;
            smash.current_cmd = this;
            waitpid(pid, nullptr, WUNTRACED);
            smash.current_pid = -1;
        }else{
            smash.jobs_list->addJob(this, pid, false);
        }
        smash.jobs_list->removeFinishedJobs();
    }

}

/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/
//RedirectionCommand
// to do delete & use in this function
void RedirectionCommand::execute() {

    SmallShell&smash=SmallShell::getInstance();
    char *args[COMMAND_MAX_ARGS];
    int is_ext = 0;
    char * cmd_line_temp= new char (strlen(cmd_line));
    remove_extra_mid_spaces_2(cmd_line,cmd_line_temp);
    string cmd_left=string (cmd_line);
    string cmd_right=string (cmd_line_temp);
    int redirect_sign_len=1;
    int redirect_index=-1;
    int back_to;
    int args_num = _parseCommandLine(this->cmd_line, args);
    //bool is_background=_isBackgroundComamnd(this->cmd_line);
    bool is_background=false;
    if((redirect_index=cmd_left.find(">>"))!=-1 ){
        redirect_sign_len = 2;
        string left_cmd=cmd_left.substr(0,redirect_index);
        _trim(left_cmd);
        if(is_background){
            left_cmd.push_back('&');
        }
        redirect_index=cmd_right.find(">");
        string right_cmd=cmd_right.substr(redirect_index+redirect_sign_len,cmd_right.size()-redirect_index-redirect_sign_len);
        _trim(right_cmd);
        char* filename=new char[strlen(right_cmd.c_str())+1];
        strcpy(filename,right_cmd.c_str());
        if(_isBackgroundComamnd(filename)){
            _removeBackgroundSign(filename);
        }
        int file_dir = open(filename, O_APPEND | O_RDWR | O_CREAT, 0666);
        if (file_dir == -1) {
            perror("smash error: open failed");
        }
        back_to = dup(1);
        SmallShell::getInstance().file_dir = dup(1);
        dup2(file_dir, 1);
        close(file_dir);
        SmallShell::getInstance().first_red = true;
        Command *excCommand = smash.CreateCommand(left_cmd.c_str(), &is_ext, &(smash.current_pwd));
        _removeBackgroundSign(excCommand->cmd_line);
        smash.executeCommand(excCommand->cmd_line);
        if(!SmallShell::getInstance().son_piped)
            smash.jobs_list->removeFinishedJobs();
        fflush(stdout);
        dup2(back_to, 1);
        close(back_to);
        smash.first_red=false;
        return ;
    }
    else if(((redirect_index=cmd_left.find(">"))!=string::npos)&&(cmd_left.find(">>") == -1)){
        string left_cmd=cmd_left.substr(0,redirect_index);
        _trim(left_cmd);
        if(is_background){
            left_cmd.push_back('&');
        }
        redirect_sign_len=1;
        redirect_index=cmd_right.find(">");
        string right_cmd=cmd_right.substr(redirect_index+redirect_sign_len,cmd_right.size()-redirect_index-redirect_sign_len);
        _trim(right_cmd);
        char* filename=new char[strlen(right_cmd.c_str())+1];
        strcpy(filename,right_cmd.c_str());
        if(_isBackgroundComamnd(filename)){
            _removeBackgroundSign(filename);
        }
        int file_dir = open(filename, O_TRUNC | O_RDWR | O_CREAT, 0666);
        if (file_dir == -1) {
            perror("smash error: open failed");
            return ;
        }
        back_to = dup(1);
        SmallShell::getInstance().file_dir = dup(1);
        dup2(file_dir, 1);
        close(file_dir);
        SmallShell::getInstance().first_red = true;
        Command *excCommand = smash.CreateCommand(left_cmd.c_str(), &is_ext, &(smash.current_pwd));
        _removeBackgroundSign(excCommand->cmd_line);
        smash.executeCommand(excCommand->cmd_line);
        if(!SmallShell::getInstance().son_piped)
            smash.jobs_list->removeFinishedJobs();
        fflush(stdout);
        dup2(back_to, 1);
        close(back_to);
        smash.first_red=false;
        return ;
    }
}



/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/

//JobsCommand
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs):BuiltInCommand(cmd_line){
    this->jobsList=jobs;
}

void JobsList:: removeFinishedJobs()
{

    SmallShell&smash=SmallShell::getInstance();
    if(jobs->empty())
        return;
    int w_status=0;
    pid_t pid;
    std::list <JobEntry> :: iterator iter;
    iter = jobs->begin();
    int wait_val;
    while(iter != jobs->end()){
        pid = (*iter).pid;
        wait_val = waitpid(pid,&w_status,WNOHANG);
        if ( wait_val != 0 ) {
            jobs->erase(iter);
            smash.timelist->removeTimedCommandByPid(pid);
            iter = jobs->begin();
            continue;
        }
        iter++;
    }
}

/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/


//CatCommand
CatCommand::CatCommand(const char* cmd_line):BuiltInCommand(cmd_line){}
void CatCommand:: execute() {

    char *args[COMMAND_MAX_ARGS];
    char buffer[COMMAND_ARGS_MAX_LENGTH];
    int args_num = _parseCommandLine(cmd_line, args);
    if(args_num < 2 )
    {
        cerr<<"cat: not enough arguments"<<endl;
        return;

    }
    char *temp;
    char file_path[COMMAND_ARGS_MAX_LENGTH];
   for (int i=1 ; i<=COMMAND_MAX_ARGS ; i++)
   {
       if(args[i]== nullptr )
           break;
       temp = args[i];
       realpath(temp,file_path);
       int temp_fd = open(file_path, O_RDONLY, 0666);
       if (temp_fd == -1) {
           perror("smash error: open failed");
           return;
       }
       while(1)
       {
           int  len = read(temp_fd,buffer,COMMAND_ARGS_MAX_LENGTH);
           if (len == 0)
               break;
                write(1,buffer,len);
       }
       close( temp_fd);
   }





}



/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/



// PipeCommand methods
PipeCommand::PipeCommand(const char *cmd_line, bool is_back, char **args, int args_num, int first_args_num)
        : Command(cmd_line) {

    this->is_background = is_back;
    this->args_num = args_num;
    this->first_args_num = first_args_num;
    std::string cmd_s=strdup(cmd_line);
   // this->cmd_length=cmd_s.length()+1;
    _trim(cmd_s);
    char* in= strdup(cmd_s.c_str());
    char* out=new char[cmd_s.length()+1];
    remove_extra_mid_spaces_1(in,out);
    _parseCommandLine(out,my_args);

}


// to check is background command
void PipeCommand::execute() {

    SmallShell &smash = SmallShell::getInstance();
   // this->is_background=_isBackgroundComamnd(cmd_line);
    this->is_background=false;
    int pid = fork();
    pid_t father;
    if (pid == 0) {
        father=getpid();
        setpgrp();
        int fork_1;
        int fork_2;
        int Pipes[2];
        pid_t pid_for_son_1;
        string cmd_string = string(this->cmd_line);
        int pipe_sign_len = 1;
        int pipe_index = -1;
        if (((pipe_index = cmd_string.find("|")) != -1) &&
            (cmd_string.find("|&") == -1)) {
            pipe_sign_len = 1;
            int result = dup(0);
            int result_2 = dup(1);
            string left_cmd = cmd_string.substr(0, pipe_index);
            _trim(left_cmd);
            string right_cmd = cmd_string.substr(pipe_index + pipe_sign_len, cmd_string.size() - pipe_index - pipe_sign_len);
            _trim(right_cmd);
            if (this->is_background) {
                left_cmd += "&";
            }

            pipe(Pipes);
            if(getpid()==father)
                if ((fork_1=fork()) == 0 ) {
                    pid_for_son_1 = getpid();
                    dup2(Pipes[1], 1);
                    close(Pipes[1]);
                    close(Pipes[0]);
                    smash.executeCommand(left_cmd.c_str(),true,true);
                    dup2(result_2, 1);
                    close(result_2);
                }
            if(getpid()==father)
                if ((fork_2=fork()) == 0) {
                    dup2(Pipes[0], 0);
                    close(Pipes[1]);
                    close(Pipes[0]);
                    smash.executeCommand(right_cmd.c_str(),true,true );
                    dup2(result, 0);
                    close(result);
                }
            close(Pipes[0]);
            close(Pipes[1]);
        }
        else if((pipe_index=cmd_string.find("|&"))!=-1){
            pipe_sign_len = 2;
            int output = dup(0);
            int output2 = dup(2);
            string left_cmd = cmd_string.substr(0, pipe_index);
            _trim(left_cmd);
            string right_cmd = cmd_string.substr(pipe_index + pipe_sign_len, cmd_string.size() - pipe_index - pipe_sign_len);
            _trim(right_cmd);
            if (this->is_background) {
                left_cmd += "&";
            }
            pipe(Pipes);
            if(getpid()==father)
                if ((fork_1=fork()) == 0 ) {
                    pid_for_son_1 = getpid();
                    dup2(Pipes[1], 2);
                    close(Pipes[1]);
                    close(Pipes[0]);
                    smash.executeCommand(left_cmd.c_str(),true,true);
                    dup2(output2, 2);
                    close(output2);

                }
            if(getpid()==father)
                if ((fork_2=fork()) == 0) {
                    dup2(Pipes[0], 0);
                    close(Pipes[1]);
                    close(Pipes[0]);
                    smash.executeCommand(right_cmd.c_str(),true);
                    dup2(output, 0);
                    close(output);
                }
            close(Pipes[0]);
            close(Pipes[1]);
        }
            waitpid(fork_1, nullptr, WUNTRACED);
            waitpid(fork_2, nullptr, WUNTRACED);
        exit(0);
    }
    else if(pid>0){
        if (this->is_background) {
            smash.jobs_list->addJob(this, pid,false,true);
        }

        if(!this->is_background) {
            SmallShell::getInstance().current_pid=pid;
            waitpid(pid, nullptr,WUNTRACED);
            return;
        }
    }
}
/*********************************************************************************************************/
/*********************************************************************************************************/
/*********************************************************************************************************/


//timeout command and execute method

TimeOutCommand::TimeOutCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
void  TimeOutCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    char *args[COMMAND_MAX_ARGS];
    int args_num = _parseCommandLine(cmd_line, args);
    int is_external =0 ;
    if ((args_num < 3) || (atoi(args[1]) <= 0)) {
        cout << "smash error: timeout: invalid arguments" << endl;
        return;
    }

    string cmd_to_exc = "";
    for (int i = 2; i < args_num; i++) {
        cmd_to_exc += " ";
        cmd_to_exc += args[i];
    }
    bool is_background = _isBackgroundComamnd(cmd_to_exc.c_str());
    char cmd[strlen(cmd_line)];
    strcpy(cmd,cmd_to_exc.c_str());
    if(is_background){
        _removeBackgroundSign(cmd);
    }
    Command *command = smash.CreateCommand(cmd, &(is_external), &(smash.current_pwd));
    if (smash.isBuiltInCommand(cmd_to_exc)){
        command->execute();
        return;
    }
    pid_t pid = fork();
    if(pid == 0){
        setpgrp();
        command->execute();
        exit(0);
    }
    else{
        if (is_background) {
            Command* comm = smash.CreateCommand(cmd_line,&(is_external), &(smash.current_pwd));

            smash.jobs_list->addJob(comm,  pid, false);
            smash.current_pid = -1;
            smash.cmd_line = nullptr;
            time_t *cmd_time = new time_t;
            time_t curr_time;
            curr_time = time(cmd_time);
            timeEntry timed=timeEntry(pid, cmd_to_exc, atoi(args[1]), curr_time);
            smash.timelist->insertTimedCommand(timed);
            return;
        } else {
            smash.current_pid = pid;
            smash.cmd_line =command->cmd_line;
            time_t *cmd_time = new time_t;
            time_t curr_time;
            curr_time = time(cmd_time);
            timeEntry timed=timeEntry(pid, cmd_to_exc, atoi(args[1]), curr_time);

            smash.timelist->insertTimedCommand(timed);
            if (waitpid(pid, nullptr, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            }
            smash.current_pid = -1;
            smash.cmd_line = nullptr;

        }
    }

}



