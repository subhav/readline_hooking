// Readline replacement to use with automation.

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "readline/readline.h"

// There's no need to build multiple object files.
// TODO: xmalloc is nothing special. We can implement it ourselves
#include "xmalloc.c"
#include "savestring.c"

char *savestring (const char *s);

#define DBG(...) //fprintf (stderr, __VA_ARGS__);
#define BUF_SIZE 512

// Redefine these only if they don't already exist.
// Python doesn't have these symbols.
// TODO: I have no clue if this works. Test it somehow.
#pragma weak rl_line_buffer
#pragma weak rl_instream
#pragma weak rl_outstream
char* rl_line_buffer;
FILE* rl_instream;
FILE* rl_outstream;

char* input;

static int initialized;

static void init() {
	if (initialized != 0) {
		return;
	}
	initialized = 1;

	// I suppose it's possible that the terminal isn't on stdin/out.
	if (!rl_instream) {
		rl_instream = stdin;
	}
	if (!rl_outstream) {
		rl_outstream = stdout;
	}
	
	// rl_line_buffer is initialized in readline_initialize_everything
	// I wonder if it's needed for any internal readline stuff?
	input = (char*) xmalloc(BUF_SIZE);
	rl_line_buffer = input + 5;
}

static void write_prompt(const char* prompt) {
	// TODO: Explicitly indicate that this is for the GUI?

	fprintf(rl_outstream, "\e]133;A\e\\");
	fprintf(rl_outstream, "%s", prompt);
	fprintf(rl_outstream, "\e]133;B\e\\");
	fflush(stdout); // stdout is buffered in python
}

static void write_cleanup() {
	fprintf(rl_outstream, "\e]133;C\e\\");
	// Ideally, this would be before the next prompt.
	fprintf(rl_outstream, "\e]133;D;0\e\\");
	fflush(stdout);
}

// Returns whether to stop reading, as well as the line if it was read.
static int read_once(char** line_ptr) {
	*line_ptr = NULL;
	char* s = fgets(input, BUF_SIZE, rl_instream);
	if (s == NULL) {
		return 1;
	}

	// TODO: This breaks sending multiple lines of input to a program,
	//       or buffering inputs ahead of time, unless they're all
	//       prefixed with 'exec '.

	if (strncmp(input, "exec ", 5) == 0) {
		*line_ptr = savestring(input+5);
		return 1;
	} else if (strncmp(input, "hist ", 5) == 0) {
		// TODO: print readline history
		return 0;
	} else if (strncmp(input, "comp ", 5) == 0) {
		// TODO: print readline complete
		return 0;
	}

	DBG("[read_once: bad command]")
	return 1;
}

/*
 * Blocking interface
 */

char* readline(const char* prompt) {
	init();
	write_prompt(prompt);

	while (1) {
		char* line;
		int done = read_once(&line);
		if (done) {
			write_cleanup();
			return line;
		}
	}
}

/*
 * Non-Blocking Interface
 */

rl_vcpfunc_t *rl_linefunc;

void rl_callback_handler_install(const char* prompt, rl_vcpfunc_t* linefunc) {
	DBG("[handler_install]");
	init();
	rl_linefunc = linefunc;
	write_prompt(prompt);
}

void rl_callback_read_char(){
	DBG("[read_char]");
	char* line;
	// We always expect that a full line is available to read, so even though
	// this makes the non-blocking interface buffered, it shouldn't be noticeable.
	int done = read_once(&line);
	if (done) {
		write_cleanup();
		rl_linefunc(line);
	}
}

void rl_callback_sigcleanup() {}

void rl_callback_handler_remove() {
	// Should this do anything?
}
