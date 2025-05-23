We want to print out escape sequences that notify the terminal of a new
prompt, and provide history and autocompete.

There are two ways we could do this.

1.  Hook into readline internals.

    rl_startup_hook() and rl_pre_input_hook() are part of the public API, and
    get called internally in readline if they're set. However, their values
    may be altered by the application, and I'm not sure. 

    readline_internal_teardown() gets called after each line of input, in
    both readline.c and callback.c (depending on how readline is being
    called). We can try to replace it with a wrapper.
 
    For example, readline_internal looks like this:
      {
         readline_internal_setup();
         int eof = readline_internal_charloop();
         return readline_internal_teardown(eof);
      }

    For history and autocomplete, we might have to write custom "readline 
    functions" and add bindings to them.

2.  Replace the readline() function(s).

    The readline library is already configured by setting global variables
    and writing to hooks. We don't care about any of the line editor
    functionality.
 
    For history and autocomplete, we can parse the line that's received and
    see if it's special. If it isn't, then just return the line.

    NOTE: There's an alternate non-blocking readline interface which uses
    the following functions instead:
    *   rl_callback_handler_install  - Displays the prompt
    *   rl_callback_read_char        - Read input
    *   rl_callback_sigcleanup
    *   rl_callback_handler_remove
    (This is what python uses.)

In general, applications which use libreadline have access to a lot of
library internals. (Just look at all the externs in readline.h.)
It may be difficult to predict what bits of state they actually depend on.

See: https://tiswww.case.edu/php/chet/readline/readline.html#Programming-with-GNU-Readline