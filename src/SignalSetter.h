#pragma once

#include <csignal>

class SignalSetter {
public:
    SignalSetter(int signo, void (*handler)(int)) : signo_(signo) {
        struct sigaction action;
        action.sa_handler = handler;
        sigemptyset (&action.sa_mask);
        action.sa_flags = 0;
        sigaction(signo, &action, &oldAction_);
    }
    ~SignalSetter() {
        sigaction(signo_, &oldAction_, nullptr);
    }

private:
    const int signo_;
    struct sigaction oldAction_;
};
