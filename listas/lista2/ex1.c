#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>

int value = 5;

int main() {
    pid_t pid;
    pid = fork();
    if (pid == 0) { /* processo-filho */
        value += 15;
        printf("CHILDREN: value = %d\n", value);
        return 0;
    } else if (pid > 0) { /* processo-pai */
        wait(NULL);
        printf("PARENT: value = %d\n", value); /* LINHA A */
        return 0;
    }
}
