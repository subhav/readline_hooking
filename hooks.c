#include <stdio.h>
#include <dlfcn.h>
#include "readline/readline.h"


// Post-Exec Marker: OSC 133 ; D ST
// Pre-Prompt Marker: OSC 133 ; A ST
// Pre-Cmd Marker: OSC 133 ; B ST

rl_hook_func_t *rl_startup_hook;
rl_hook_func_t *rl_pre_input_hook;

static int first = 1;

int startup(void) {
    if (first) {
        first = 0;
    } else {
        printf("\e]133;D\e\\");
    }
    printf("\e]133;A\e\\");
    // printf("\e]133;B\e\\");
}

int pre_input(void) {
    // This doesn't seem to output until after I start typing 
    printf("\e]133;B\e\\");
}

// Pre-Exec Marker: OSC 133 ; C ST
//     I think this matters less than knowing when a prompt starts.
//     Once I send a command, I'm *probably* just going to assume that whatever
//     comes after it is command output, unless I see another pre-cmd marker. 

typedef char* (*teardownfunc_t)(int);
static teardownfunc_t real_teardown;

char* readline_internal_teardown(int eof) {
    fprintf(stderr, "called readline_internal_teardown(%d)\n", eof);
    
    // TODO: Is this correct?
    if(eof) {
        printf("\e]133;D\e\\");
    } else {
        printf("\e]133;C\e\\");
    }
    
    if (!real_teardown) {
        real_teardown = dlsym(RTLD_NEXT, "readline_internal_teardown");
        if (real_teardown == NULL) {
            fprintf(stderr, "error finding function: %s\n", dlerror());
        }
    }
    // TODO: python crashes with a sigfault when this is called
    return real_teardown(eof);
}


void __attribute__((constructor)) inject_init() {
    // Python overwrites these...
    rl_startup_hook = startup;
    rl_pre_input_hook = pre_input;
}
