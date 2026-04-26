# ai usage

i used ai (gemini 3.1 pro) for the filter functions.

my prompts:
- asked for a C function to split strings like 'severity:>=:2' into 3 parts.
- pasted my Report struct and asked for a match function to check if a report fits the rule.

what it gave me:
- used `strtok` to chop up the string.
- used `strcmp` and `atoi` to compare the struct fields.

what i had to fix:
- the ai ran `strtok` directly on a `const char*`. crashed instantly with a segfault. 
- fixed it by adding a `char temp[64]` buffer and copying it with `strncpy` first.
- cleaned up the messy if/else logic in the match function.

what i learned:
- `strtok` actually destroys the string by inserting `\0`s. gotta use a copy.
