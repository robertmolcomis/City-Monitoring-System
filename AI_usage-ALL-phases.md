# AI Usage Documentation - All Phases
**Project:** City Infrastructure Issue Reporting and Monitoring System
**Tool Used:** Google Gemini

## Phase 1: File Systems & Condition Matching

**Prompts Given:**
1. *"I have a C struct called `Report` with fields: `int severity`, `char category[16]`, and `char inspector[32]`. Generate a function `int parse_condition(const char *input, char *field, char *op, char *value);` that splits a string formatted as 'field:operator:value'."*
2. *"Now generate a function `int match_condition(Report *r, const char *field, const char *op, const char *value);` that returns 1 if the record satisfies the condition and 0 otherwise."*

**What was generated:**
The AI generated two functions. `parse_condition` used `strtok` to split the string based on the `:` delimiter. `match_condition` used a series of `if/else` statements to compare the extracted `field` string against the struct members.

**What I changed and why:**
* **Memory Safety:** The AI originally ran `strtok` directly on the `const char *input`, which causes segmentation faults because string literals are read-only and `strtok` modifies the string. I changed the code to copy the input into a temporary buffer (`char temp[64]`) before parsing.
* **Type Casting:** For the `severity` field, I had to ensure the AI's string `value` was converted using `atoi(value)` so it could be correctly evaluated with mathematical operators (`>=`, `<=`, `==`, etc.) against the struct's integer.
* **String Comparisons:** I ensured `strcmp` was correctly implemented for the `category` and `inspector` fields, rather than standard equality operators.

---

## Phase 2: Processes and Signals

**Prompts Given:**
1. *"How do I safely delete a directory and its contents from within a C program using `fork()` and `exec` without using `system()`?"*
2. *"How do I write a background monitor process in C that stays alive and responds to `SIGUSR1` and `SIGINT`? Please use `sigaction`, not `signal()`."*

**What was generated:**
The AI provided a `fork()` pattern where the child process calls `execlp("rm", "rm", "-rf", district_name, NULL)` and the parent waits using `wait()`. For the monitor, it provided a `sigaction` setup that flipped a volatile global flag to exit a `while(1)` loop cleanly.

**What I changed and why:**
* **Security Guards:** Passing raw command-line arguments to `rm -rf` is incredibly dangerous. I added a manual check to ensure the user input does not contain slashes (`/`) or `..` to prevent directory traversal attacks (e.g., stopping a malicious user from running `remove_district ../../`).
* **Compiler Warnings:** The AI's signal handlers left the `int sig` parameter unused, which triggered `-Wunused-parameter` warnings in `gcc`. I added the `(void)sig;` macro to suppress this warning and ensure a clean compilation.
* **Async-Signal Safety:** I ensured the monitor uses `write(STDOUT_FILENO, ...)` inside the signal handlers instead of `printf()`, as `printf()` is not async-signal-safe and can cause deadlocks if interrupted.

---

## Phase 3: Pipes and Redirects

**Prompts Given:**
1. *"How do I spawn multiple child processes in C, redirect their standard output to a pipe using `dup2()`, and have the parent process read the combined text output?"*
2. *"My background monitor process is buffering its `printf` output when it is redirected into a pipe, meaning the parent process doesn't see the text immediately. How do I fix this?"*

**What was generated:**
The AI provided a basic example of creating an `int pipefd[2]`, forking, mapping `STDOUT_FILENO` to `pipefd[1]` in the child, and using `fdopen(pipefd[0], "r")` in the parent to read the results line-by-line using `fgets`. To solve the buffering issue, it recommended adding `fflush(stdout)` after every print statement.

**What I changed and why:**
* **Struct Aggregation (Scorer):** I wrote the logic for `scorer.c` myself to read the fixed-size binary records and aggregate the scores into an array of `InspectorScore` structs.
* **Message Tagging:** When routing the monitor through `city_hub`'s background pipe (`hub_mon`), it became difficult to tell standard info apart from program termination. I modified the monitor to prefix all its string outputs with specific tags (`[INFO]`, `[ALERT]`, `[ERROR]`, `[END]`).
* **Hub Interception:** I modified the `city_hub` read loop to actively parse the incoming strings from the pipe. If the hub detects the `[END]` or `[ERROR]` tag, it injects a clean system alert into the interactive prompt, making the UI much more robust and user-friendly.

---

## Build System (Makefile)

**Prompts Given:**
1. *"Generate a Makefile that compiles `city_manager.c`, `monitor_reports.c`, `scorer.c`, and `city_hub.c` into their respective executables, and includes a clean rule to remove old binaries."*

**What was generated:**
The AI generated a standard GNU Makefile with variables for the compiler (`CC`) and flags (`CFLAGS`), establishing build targets for each of the four separate C programs, as well as an `all` target to compile everything at once.

**What I changed and why:**
* **Compilation Flags:** I ensured the `CFLAGS` included `-Wall`, `-Wextra`, and `-g`. This was crucial to ensure the project compiled cleanly according to strict course requirements, catching potential bugs like unused variables in my signal handlers before execution.
* **Clean Target:** I modified the `clean` target to also remove the `.monitor_pid` file, ensuring no leftover background state persists between fresh builds.

---

## Final Learnings
Across this project, using AI was highly effective for generating standard POSIX boilerplate (like the syntax for `sigaction` or `pipe` mapping). However, AI frequently failed to account for environmental security (like unsafe inputs to `rm -rf`), strict compiler warnings (`-Wall -Wextra`), and I/O buffering edge cases (`fflush`). I learned that AI is a great tool for generating foundational syntax, but the developer must intimately understand the system architecture to apply memory safety, handle concurrent race conditions, and correctly format binary reads/writes.
