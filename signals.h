#ifndef SMASH__SIGNALS_H_
#define SMASH__SIGNALS_H_
#include <bits/types/siginfo_t.h>

void ctrlZHandler(int sig_num);
void ctrlCHandler(int sig_num);
void alarmHandler(int sig_num, siginfo_t*, void*);

#endif //SMASH__SIGNALS_H_
