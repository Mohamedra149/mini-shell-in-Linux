#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <time.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
 public:
    char* cmd_line;
    int cmd_length;
  Command(const char* cmd_line){
      this->cmd_length= strlen(cmd_line)+1;
      this-> cmd_line= strdup(cmd_line);
  }
  virtual ~Command(){

      if(this->cmd_line != nullptr )
      {
          delete cmd_line;
      }
  }
  virtual void execute() = 0;
};
class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line):Command(cmd_line){}
  virtual ~BuiltInCommand() {}
};
class ExternalCommand : public Command {
 public:
    ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};
class PipeCommand : public Command {
  // TODO: Add your data members
 public:
    bool is_background;
    int args_num;
    int first_args_num;
    char* my_args[COMMAND_MAX_ARGS];
    PipeCommand(const char *cmd_line, bool is_back, char **args, int args_num, int first_args_num);

    virtual ~PipeCommand() {}
    void execute() override;
};
class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line):Command(cmd_line){}
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};
class chpromptCommand:public  BuiltInCommand{
public:
    std::string name;
    chpromptCommand(const char* cmd_line,std::string name);
    ~chpromptCommand(){}
    void execute() override;
};
class ChangeDirCommand : public BuiltInCommand {
    public:
        char* last_pwd;
        int     length;
        ChangeDirCommand(const char* cmd_line, char** last_Pwd);
        virtual ~ChangeDirCommand() {}
        void execute() override;
};
class GetCurrDirCommand : public BuiltInCommand {
 public:
    GetCurrDirCommand(const char* cmd_line);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};
class ShowPidCommand : public BuiltInCommand {
 public:
  int process_pid;
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() =default;
    void execute() override;
};
class JobsList {
 public:
  class JobEntry {
  public:
      int job_id;
      char* cmd_line;
      int pid;
      time_t time_added;
      bool is_stopped;
      bool piped;
      JobEntry(char* cmd_line,pid_t pid_p,bool isStopped,bool piped=false){

          time(&time_added);
          this->is_stopped = isStopped;
          this->pid=pid_p;
          int x=strlen(cmd_line);
          this->cmd_line = new char[x+1];
          strcpy(this->cmd_line,cmd_line);
          this->piped=piped;
      }
      JobEntry(const JobEntry& copy){
          int len=strlen(copy.cmd_line)+1;
          this->job_id=copy.job_id;
          this->pid=copy.pid;
          this->cmd_line= new char[len];
          this->time_added=copy.time_added;
          this->is_stopped=copy.is_stopped;
          this->piped=copy.piped;
          strcpy(this->cmd_line,copy.cmd_line);
      }

      void stop(){
          this->is_stopped = true;
          time(&time_added);
      }
      void unstop(){
          this->is_stopped = false;
          if(this->piped) {
              killpg(this->pid, SIGCONT);
          }
          else{
              kill(this->pid,SIGCONT);
          }
      }

  };

 public:
    int jobs_number;
    std::list<JobEntry>*jobs;
  JobsList(){
      jobs_number= 0;
      this->jobs=new std::list<JobEntry>();
  }
  ~JobsList(){}
  void addJob(Command* cmd,pid_t pid, bool isStopped = false,bool pipe=false ){
      this->removeFinishedJobs();
      std::list <JobEntry> :: iterator it;
      for(it = jobs->begin() ; it != jobs->end() ; it++){
          if(pid == (*it).pid){
              (*it).stop();
              return;
          }
      }
      jobs_number++;
      JobEntry newJob(cmd->cmd_line, pid, isStopped,pipe);
      if (jobs->empty()) {
          newJob.job_id=1;
      } else{
          newJob.job_id=(jobs->back().job_id + 1);
      }
      jobs->push_back(newJob);


  }
  void printJobsList(){
      time_t cur_time;
      double time_passed;
      bool stopped=false;
      time(&cur_time);
      std::list <JobEntry> :: iterator it;
      if(cur_time == (time_t)-1) { // check this type error
          perror("smash error: time failed");
          return;
      }
      for(it = jobs->begin(); it != jobs->end(); ++it ){
          time_passed = difftime(cur_time,(*it).time_added);
          if(time_passed == (time_t)-1){
              perror("smash error: time failed");
              return;
          }
          std::cout<<"["<<(*it).job_id<<"]"<<" "<<(*it).cmd_line<<" : "<<(*it).pid<<" "<<time_passed<<" secs";
          if((*it).is_stopped) {
              stopped=true;
              std::cout<<" (stopped)"<<std::endl;
          }
          if(!stopped)
              std::cout << std::endl;
          stopped= false;
      }
      return;

  }
  void killAllJobs() {
      if(this->jobs->empty())
          return;
      std::list <JobEntry> :: iterator iter;
      for(iter = this->jobs->begin(); iter != (this->jobs->end()); ++iter) {
          std::cout << (*iter).pid << ": " << (*iter).cmd_line << std::endl;
          int ret_value;
          if ((*iter).piped)
              ret_value = killpg((*iter).pid, SIGKILL);
          else
              ret_value = kill((*iter).pid, SIGKILL);
          if (ret_value != 0)
              perror("smash error: kill failed");
      }
  }
  void removeFinishedJobs() ;
  JobEntry * getJobById(int job_id){
      std::list <JobEntry> :: iterator iter=jobs->begin();
      while(iter!=jobs->end()){
          if(job_id==(*iter).job_id){
              return &(*iter);
          }
          ++iter;
      }
      return nullptr;
  }
    JobEntry * getJobByPid(pid_t pid){
        std::list <JobEntry> :: iterator iter=jobs->begin();
        while(iter!=jobs->end()){
            if(pid==(*iter).pid){
                return &(*iter);
            }
            ++iter;
        }
        return nullptr;
    }
  void removeJobById(int job_id){
      std::list <JobEntry> :: iterator iter=jobs->begin();
      while(iter!=jobs->end()){
          if(job_id==(*iter).job_id){
              jobs->erase(iter);
              break;
          }
          ++iter;
      }
  }

  JobEntry * getLastJob(int* lastJobId){
          *lastJobId = jobs->back().job_id;
          return &(jobs->back());
    }

  JobEntry *getLastStoppedJob(int *jobId){

      if (jobs->empty()){
          *jobId =-1;
          return nullptr;
      }
      std::list <JobEntry> :: iterator it = jobs->end();
      it--;
      for(; it != jobs->begin(); it--){
          if((*it).is_stopped){
              *jobId =(*it).job_id;
              break;
          }
      }
      if(it == jobs->begin() && !((*it).is_stopped)){
          *jobId =-1;
          return nullptr;
      }
      return &(*it);
  }

};
class QuitCommand : public BuiltInCommand {

public:
    JobsList* job_list;
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {};
    void execute() override;
};
class JobsCommand : public BuiltInCommand {
public:
    JobsList* jobsList;
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand(){}
    void execute() override{

        this->jobsList->removeFinishedJobs();
        this->jobsList->printJobsList();
    }
};
class KillCommand : public BuiltInCommand {
 public:
    JobsList* jobsList;
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};
class ForegroundCommand : public BuiltInCommand {
 public:
    JobsList* jobsList;
    int job_id;
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {  }
  void execute() override;
};
class BackgroundCommand : public BuiltInCommand {
 public:
    JobsList* jobsList;
    int jobID;
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() { }
  void execute() override;
};
class CatCommand : public BuiltInCommand {
 public:
  CatCommand(const char* cmd_line);
  virtual ~CatCommand(){}
  void execute() override;
};

class TimeOutCommand : public BuiltInCommand
{
    virtual ~TimeOutCommand() {}
void execute() override;

public:
    TimeOutCommand(const char* cmd_line);
};

class timeEntry {
public:
    pid_t pid;
    std::string  cmd_line;
    int duration;
    time_t exec_time;

    timeEntry(pid_t pid, std::string cmd_line, int duration, time_t exec_time) : pid(pid),cmd_line(cmd_line), duration(duration),
                                                                            exec_time(exec_time) {
    }

    ~timeEntry() = default;
};

class timeList {
public:
    std::vector <timeEntry> time_list;
    timeList() = default;
    ~timeList() = default;


    pid_t returnFinishedOne() const {
        if (this->time_list.empty()) {
            return -1;
        }
        auto it = this->time_list.begin();
        time_t exec_time = it->exec_time;

        time_t *cmd_time = new time_t;
        time_t curr_time;
        curr_time = time(cmd_time);

        double diff_time = difftime(curr_time, exec_time);
        if (diff_time >= it->duration) {
            return it->pid;
        }
        return -1;
    }

    std::string getCommandByPid(pid_t pid) const {
        for (auto &it : this->time_list) {
            if (it.pid == pid) {
                return it.cmd_line;
            }
        }
        return nullptr;
    }

    void insertTimedCommand(const timeEntry &cmd) {
        time_t *cmd_time = new time_t;
        time_t curr_time;
        curr_time = time(cmd_time);
        if (this->time_list.size() == 0) {
            this->time_list.push_back(cmd);
            alarm(cmd.duration);
            return;
        }
        double diff_time = difftime(curr_time, this->time_list.begin()->exec_time);
        double time_left = this->time_list.begin()->duration - diff_time;
        if (cmd.duration < time_left) {
            this->time_list.insert(this->time_list.begin(), cmd);
            alarm(cmd.duration);
            return;
        }

        const std::vector<timeEntry>::iterator it = this->time_list.begin();
        for (unsigned int i = 1; i < this->time_list.size(); ++i) {

            double diff_time_2 = difftime(curr_time, it[i].exec_time);
            double time_left_2 = it[i].duration - diff_time_2;

            double diff_time_1 = difftime(curr_time, it[i - 1].exec_time);
            double time_left_1 = it[i - 1].duration - diff_time_1;

            if ((time_left_2 > cmd.duration) && (time_left_1 < cmd.duration)) {
                this->time_list.insert(this->time_list.begin() + i, cmd);
                return;
            }
        }
        this->time_list.push_back(cmd);
    }

    void removeTimedCommandByPid(int pid) {
        if (time_list.empty())
            return;
        if (this->time_list.begin()->pid == pid) {
            removeAndSetAlarm();
        }
        for (int i = 0; i < time_list.size(); i++) {
            if (time_list[i].pid == pid) {
                time_list.erase(time_list.begin() + i);
                return;
            }
        }
    }

    void removeAndSetAlarm() {
        if (this->time_list.empty()) {
            return;
        }
        auto it = this->time_list.begin();
        this->time_list.erase(it);
        it = this->time_list.begin();
        time_t *cmd_time = new time_t;
        time_t curr_time;
        curr_time = time(cmd_time);
        double diff_time_2 = difftime(curr_time, it->exec_time);
        double time_left_2 = it->duration - diff_time_2;
        alarm((unsigned int) time_left_2);
        delete cmd_time;
    }
};



class SmallShell {
 public:
    pid_t smash_pid;
    JobsList* jobs_list ;
    std::string shell_name ;
    Command* current_cmd;
    char* current_pwd;
    char* last;
    char* cmd_line;
    int current_pid;
    timeList* timelist;
    // redirection checks
    int file_dir;
    bool redirected;
    bool first_red;

    // pipe checks
    bool piped;
    bool is_back_pipe;
    bool  signal_piped;
    bool son_piped ;


  SmallShell(){
      this->smash_pid= getpid();
      this->jobs_list= new JobsList();
      timelist=new timeList();
      this->shell_name="smash";
      this->current_pid=    0;
      this->current_pwd = nullptr;
      this->cmd_line= nullptr;
      this->piped= false;
      this->redirected=false;
      this->signal_piped=false;
      this->son_piped=false;

  }
 public:
    Command *CreateCommand(const char* cmd_line,int* is_external,char** last_pwd);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell() =default;
    void executeCommand(const char* cmd_line,bool flag=false,bool piped_son=false );
    bool isBuiltInCommand(const std::string& command_to_check){
        return command_to_check.find("pwd") != std::string::npos ||
                command_to_check.find("showpid") != std::string::npos
                || command_to_check.find("ls") != std::string::npos
                 || command_to_check.find("chprompt") != std::string::npos ||
                    command_to_check.find("cd") != std::string::npos ||
                command_to_check.find("jobs") != std::string::npos
                || command_to_check.find("kill") != std::string::npos ||
                command_to_check.find("fg") != std::string::npos ||
                command_to_check.find("bg") != std::string::npos ||
                command_to_check.find("quit") != std::string::npos;
    }
};

#endif //SMASH_COMMAND_H_
