// clang++ -lc -lc++ \
   login.cc \
   -o login && chown root:wheel login && chmod 4655 login

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/ADlibc++.hpp"

#define JB "/var/jb"

void login_root() {
    if (setuid(0) != 0 || setgid(0) != 0) {
        perror("setuid/setgid failed");
        exit(1);
    }
    execl(JB "/usr/bin/zsh", JB "/usr/bin/zsh", NULL);
    perror("execl failed");
    exit(1);
}

void login_ad() {
    if (setuid(500) != 0 || setgid(500) != 0) {
        perror("setuid/setgid failed");
        exit(1);
    }
    execl(JB "/usr/bin/zsh", JB "/usr/bin/zsh", NULL);
    perror("execl failed");
    exit(1);
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [root|ad]\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "root") == 0) {
        login_root();
    } else if (strcasecmp(argv[1], "ad") == 0) {
        login_ad();
    } else {
        fprintf(stderr, "error: invalid argument '%s'\n", argv[1]);
        fprintf(stderr, "Usage: %s [root|ad]\n", argv[0]);
        return 1;
    }
    
    return 0;
}