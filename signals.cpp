#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

/*
 *
 * ctrl z handler which stop the process which is running currently in the foreground
 */
void ctrlZHandler(int sig_num) {
    cout<<"smash: got ctrl-Z"<<endl;
    SmallShell& shell = SmallShell::getInstance();
    pid_t pid = SmallShell::getInstance().current_pid;
    if(pid == 0)
        return;
    SmallShell::getInstance().current_pid = 0;
    int res;
    if(shell.signal_piped||shell.redirected) {
        res = killpg(pid, SIGSTOP);

    }
    else {
        res=kill(pid, SIGSTOP);
    }
    int flag =0;
    if(res==0){

        shell.jobs_list->addJob(shell.current_cmd,pid,true,shell.signal_piped);
        cout << "smash: process " << pid << " was stopped" << endl;
    }
}
/*
 *
 * ctrl c handler which kill the process which is running currently in the foreground
 */
void ctrlCHandler(int sig_num) {
    cout<<"smash: got ctrl-C"<<endl;
    pid_t pid = SmallShell::getInstance().current_pid;
    SmallShell::getInstance().current_pid = 0;
    if(pid == 0)
        return;
    if(SmallShell::getInstance().signal_piped||SmallShell::getInstance().redirected)
        killpg(pid,SIGKILL);
    else
        kill(pid,SIGKILL);
    cout << "smash: process " << pid << " was killed" << endl;
    // TODO: Add your implementation
}
void alarmHandler(int sig_num, siginfo_t*, void*){
    cout << "smash: got an alarm" << endl;
    SmallShell& smash = SmallShell::getInstance();
    pid_t pid=smash.timelist->returnFinishedOne();
    while(pid>0){
        if(kill(pid,SIGKILL) == -1){
            perror("smash error: kill failed");
            return;
        }
        else{
            cout << "smash:" << (smash.timelist->getCommandByPid(pid)) <<" timed out!" << endl;
        }
        smash.timelist->removeAndSetAlarm();
        pid=smash.timelist->returnFinishedOne();
    }
}

