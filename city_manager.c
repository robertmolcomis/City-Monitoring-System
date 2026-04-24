#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

// fixed-size limits
#define NAME_LEN        32
#define CATEGORY_LEN    16
#define DESC_LEN        128
#define DISTRICT_LEN    64

// role constants
#define ROLE_INSPECTOR  0
#define ROLE_MANAGER    1

// Report record (binary, fixed-size)
typedef struct {
    int    id;
    char   inspector[NAME_LEN];
    double lat;
    double lon;
    char   category[CATEGORY_LEN];
    int    severity;          // 1=minor  2=moderate  3=critical
    time_t timestamp;
    char   description[DESC_LEN];
} Report;

// Parsed CLI arguments
typedef struct {
    int  role;                // ROLE_INSPECTOR or ROLE_MANAGER
    char user[NAME_LEN];
    char command[32];
    char district[DISTRICT_LEN];
    int  report_id;
    int  threshold;
    char conditions[8][64];
    int  nconditions;
} Args;


static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s --role <inspector|manager> --user <n> --<command> [args...]\n"
        "\n"
        "Commands:\n"
        "  --add              <district>\n"
        "  --list             <district>\n"
        "  --view             <district> <report_id>\n"
        "  --remove_report    <district> <report_id>\n"
        "  --update_threshold <district> <value>\n"
        "  --filter           <district> <condition> [condition...]\n"
        "\n"
        "Roles:  inspector | manager\n",
        prog);
}

static int parse_args(int argc, char *argv[], Args *args) {
    if (argc < 2) { usage(argv[0]); return -1; }

    memset(args, 0, sizeof(*args));
    args->role      = -1;
    args->report_id = -1;
    args->threshold = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) {
            i++;
            if      (strcmp(argv[i], "inspector") == 0) args->role = ROLE_INSPECTOR;
            else if (strcmp(argv[i], "manager")   == 0) args->role = ROLE_MANAGER;
            else { fprintf(stderr, "Unknown role: %s\n", argv[i]); return -1; }

        } else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
            strncpy(args->user, argv[++i], NAME_LEN - 1);

        } else if (strcmp(argv[i], "--add") == 0) {
            strncpy(args->command, "add", sizeof(args->command) - 1);
            if (i + 1 < argc) strncpy(args->district, argv[++i], DISTRICT_LEN - 1);

        } else if (strcmp(argv[i], "--list") == 0) {
            strncpy(args->command, "list", sizeof(args->command) - 1);
            if (i + 1 < argc) strncpy(args->district, argv[++i], DISTRICT_LEN - 1);

        } else if (strcmp(argv[i], "--view") == 0) {
            strncpy(args->command, "view", sizeof(args->command) - 1);
            if (i + 2 < argc) {
                strncpy(args->district, argv[++i], DISTRICT_LEN - 1);
                args->report_id = atoi(argv[++i]);
            }

        } else if (strcmp(argv[i], "--remove_report") == 0) {
            strncpy(args->command, "remove_report", sizeof(args->command) - 1);
            if (i + 2 < argc) {
                strncpy(args->district, argv[++i], DISTRICT_LEN - 1);
                args->report_id = atoi(argv[++i]);
            }

        } else if (strcmp(argv[i], "--update_threshold") == 0) {
            strncpy(args->command, "update_threshold", sizeof(args->command) - 1);
            if (i + 2 < argc) {
                strncpy(args->district, argv[++i], DISTRICT_LEN - 1);
                args->threshold = atoi(argv[++i]);
            }

        } else if (strcmp(argv[i], "--filter") == 0) {
            strncpy(args->command, "filter", sizeof(args->command) - 1);
            if (i + 1 < argc) strncpy(args->district, argv[++i], DISTRICT_LEN - 1);
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                if (args->nconditions < 8)
                    strncpy(args->conditions[args->nconditions++], argv[++i], 63);
                else
                    i++;
            }
        }
    }

    if (args->role == -1)         { fprintf(stderr, "Error: --role is required\n");         return -1; }
    if (args->user[0] == '\0')    { fprintf(stderr, "Error: --user is required\n");         return -1; }
    if (args->command[0] == '\0') { fprintf(stderr, "Error: no command given\n"); usage(argv[0]); return -1; }

    return 0;
}


static int cmd_add(const Args *args) {
    // TODO
    printf("[add] district=%s user=%s  (not yet implemented)\n",
           args->district, args->user);
    return 0;
}

static int cmd_list(const Args *args) {
    // TODO
    printf("[list] district=%s  (not yet implemented)\n", args->district);
    return 0;
}

static int cmd_view(const Args *args) {
    // TODO
    printf("[view] district=%s id=%d  (not yet implemented)\n",
           args->district, args->report_id);
    return 0;
}

static int cmd_remove_report(const Args *args) {
    // TODO
    if (args->role != ROLE_MANAGER) {
        fprintf(stderr, "Error: remove_report requires manager role\n");
        return -1;
    }
    printf("[remove_report] district=%s id=%d  (not yet implemented)\n",
           args->district, args->report_id);
    return 0;
}

static int cmd_update_threshold(const Args *args) {
    // TODO
    if (args->role != ROLE_MANAGER) {
        fprintf(stderr, "Error: update_threshold requires manager role\n");
        return -1;
    }
    printf("[update_threshold] district=%s value=%d  (not yet implemented)\n",
           args->district, args->threshold);
    return 0;
}

static int cmd_filter(const Args *args) {
    // TODO
    printf("[filter] district=%s conditions=%d  (not yet implemented)\n",
           args->district, args->nconditions);
    return 0;
}


int main(int argc, char *argv[]) {
    Args args;
    if (parse_args(argc, argv, &args) != 0)
        return EXIT_FAILURE;

    int ret = 0;
    if      (strcmp(args.command, "add")              == 0) ret = cmd_add(&args);
    else if (strcmp(args.command, "list")             == 0) ret = cmd_list(&args);
    else if (strcmp(args.command, "view")             == 0) ret = cmd_view(&args);
    else if (strcmp(args.command, "remove_report")    == 0) ret = cmd_remove_report(&args);
    else if (strcmp(args.command, "update_threshold") == 0) ret = cmd_update_threshold(&args);
    else if (strcmp(args.command, "filter")           == 0) ret = cmd_filter(&args);
    else { fprintf(stderr, "Unknown command: %s\n", args.command); ret = EXIT_FAILURE; }

    return ret;
}
