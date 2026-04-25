#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

#define NAME_LEN        32
#define CATEGORY_LEN    16
#define DESC_LEN        128
#define DISTRICT_LEN    64

#define ROLE_INSPECTOR  0
#define ROLE_MANAGER    1

typedef struct {
    int    id;
    char   inspector[NAME_LEN];
    double lat;
    double lon;
    char   category[CATEGORY_LEN];
    int    severity;
    time_t timestamp;
    char   description[DESC_LEN];
} Report;

typedef struct {
    int  role;
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
        "Usage: %s --role <inspector|manager> --user <n> --<command> [args...]\n", prog);
}

static int parse_args(int argc, char *argv[], Args *args) {
    if (argc < 2) { usage(argv[0]); return -1; }
    memset(args, 0, sizeof(*args));
    args->role = -1; args->report_id = -1; args->threshold = -1;

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
        } else if (strcmp(argv[i], "--view") == 0 && i + 2 < argc) {
            strncpy(args->command, "view", sizeof(args->command) - 1);
            strncpy(args->district, argv[++i], DISTRICT_LEN - 1);
            args->report_id = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--remove_report") == 0 && i + 2 < argc) {
            strncpy(args->command, "remove_report", sizeof(args->command) - 1);
            strncpy(args->district, argv[++i], DISTRICT_LEN - 1);
            args->report_id = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--update_threshold") == 0 && i + 2 < argc) {
            strncpy(args->command, "update_threshold", sizeof(args->command) - 1);
            strncpy(args->district, argv[++i], DISTRICT_LEN - 1);
            args->threshold = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--filter") == 0 && i + 1 < argc) {
            strncpy(args->command, "filter", sizeof(args->command) - 1);
            strncpy(args->district, argv[++i], DISTRICT_LEN - 1);
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                if (args->nconditions < 8) strncpy(args->conditions[args->nconditions++], argv[++i], 63);
                else i++;
            }
        }
    }
    if (args->role == -1 || args->user[0] == '\0' || args->command[0] == '\0') {
        fprintf(stderr, "Error: Missing required arguments.\n"); return -1;
    }
    return 0;
}

// --- Iteration 1 Helpers ---

void mode_to_str(mode_t mode, char *str) {
    strcpy(str, "---------");
    if (mode & S_IRUSR) str[0] = 'r'; if (mode & S_IWUSR) str[1] = 'w'; if (mode & S_IXUSR) str[2] = 'x';
    if (mode & S_IRGRP) str[3] = 'r'; if (mode & S_IWGRP) str[4] = 'w'; if (mode & S_IXGRP) str[5] = 'x';
    if (mode & S_IROTH) str[6] = 'r'; if (mode & S_IWOTH) str[7] = 'w'; if (mode & S_IXOTH) str[8] = 'x';
}

int check_access(const char *path, int role, int need_read, int need_write) {
    struct stat st;
    if (stat(path, &st) != 0) return 1; // Allow if file doesn't exist yet
    int can_read = (role == ROLE_MANAGER) ? (st.st_mode & S_IRUSR) : (st.st_mode & S_IRGRP);
    int can_write = (role == ROLE_MANAGER) ? (st.st_mode & S_IWUSR) : (st.st_mode & S_IWGRP);
    if (need_read && !can_read) return 0;
    if (need_write && !can_write) return 0;
    return 1;
}

void init_district(const char *dist) {
    struct stat st;
    if (stat(dist, &st) == -1) { mkdir(dist, 0750); chmod(dist, 0750); }
    char path[256], slink[256];
    
    snprintf(path, sizeof(path), "%s/reports.dat", dist);
    if (stat(path, &st) == -1) { int fd = open(path, O_CREAT, 0664); close(fd); chmod(path, 0664); }
    
    snprintf(path, sizeof(path), "%s/district.cfg", dist);
    if (stat(path, &st) == -1) { int fd = open(path, O_CREAT, 0640); close(fd); chmod(path, 0640); }
    
    snprintf(path, sizeof(path), "%s/logged_district", dist);
    if (stat(path, &st) == -1) { int fd = open(path, O_CREAT, 0644); close(fd); chmod(path, 0644); }
    
    snprintf(path, sizeof(path), "%s/reports.dat", dist);
    snprintf(slink, sizeof(slink), "active_reports-%s", dist);
    if (lstat(slink, &st) == 0) {
        if (S_ISLNK(st.st_mode)) {
            struct stat tgt;
            if (stat(slink, &tgt) != 0) fprintf(stderr, "Warning: Dangling symlink %s\n", slink);
        }
    } else {
        symlink(path, slink);
    }
}

void log_action(const Args *args) {
    char path[256];
    snprintf(path, sizeof(path), "%s/logged_district", args->district);
    if (!check_access(path, args->role, 0, 1)) {
        fprintf(stderr, "Diagnostic: Inspector role refused write access to logged_district.\n");
        return;
    }
    FILE *f = fopen(path, "a");
    if (f) {
        fprintf(f, "%ld\t%s\t%s\t%s\n", time(NULL), args->user, 
                args->role == ROLE_MANAGER ? "manager" : "inspector", args->command);
        fclose(f);
    }
}

// --- Iteration 1 Commands ---

static int cmd_add(const Args *args) {
    init_district(args->district);
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", args->district);

    if (!check_access(path, args->role, 0, 1)) {
        fprintf(stderr, "Error: Access denied to write reports.dat\n"); return -1;
    }

    Report r;
    memset(&r, 0, sizeof(r));
    struct stat st; stat(path, &st);
    r.id = (st.st_size / sizeof(Report)) + 1;
    strncpy(r.inspector, args->user, NAME_LEN - 1);
    r.timestamp = time(NULL);

    printf("X: "); scanf("%lf", &r.lat);
    printf("Y: "); scanf("%lf", &r.lon);
    printf("Category (road/lighting/flooding/other): "); scanf("%15s", r.category);
    printf("Severity level (1/2/3): "); scanf("%d", &r.severity);
    int c; while ((c = getchar()) != '\n' && c != EOF); // Clear stdin
    printf("Description: ");
    fgets(r.description, DESC_LEN, stdin);
    r.description[strcspn(r.description, "\n")] = 0;

    FILE *f = fopen(path, "ab");
    if (f) { fwrite(&r, sizeof(Report), 1, f); fclose(f); }
    log_action(args);
    return 0;
}

static int cmd_list(const Args *args) {
    init_district(args->district);
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", args->district);

    if (!check_access(path, args->role, 1, 0)) {
        fprintf(stderr, "Error: Access denied to read reports.dat\n"); return -1;
    }

    struct stat st;
    if (stat(path, &st) == 0) {
        char perms[10]; mode_to_str(st.st_mode, perms);
        printf("File: %s | Perms: %s | Size: %ld bytes | Last Mod: %s", path, perms, st.st_size, ctime(&st.st_mtime));
    }

    FILE *f = fopen(path, "rb");
    if (f) {
        Report r;
        while (fread(&r, sizeof(Report), 1, f) == 1) {
            printf("ID: %d | Cat: %s | Sev: %d | Desc: %s\n", r.id, r.category, r.severity, r.description);
        }
        fclose(f);
    }
    log_action(args);
    return 0;
}

// --- Iteration 2 Placeholders ---

static int cmd_view(const Args *args) {
    printf("[view] district=%s id=%d (Iter 2)\n", args->district, args->report_id);
    return 0;
}

static int cmd_remove_report(const Args *args) {
    if (args->role != ROLE_MANAGER) { fprintf(stderr, "Error: Requires manager role\n"); return -1; }
    printf("[remove_report] district=%s id=%d (Iter 2)\n", args->district, args->report_id);
    return 0;
}

static int cmd_update_threshold(const Args *args) {
    if (args->role != ROLE_MANAGER) { fprintf(stderr, "Error: Requires manager role\n"); return -1; }
    printf("[update_threshold] district=%s value=%d (Iter 2)\n", args->district, args->threshold);
    return 0;
}

static int cmd_filter(const Args *args) {
    printf("[filter] district=%s conditions=%d (Iter 2)\n", args->district, args->nconditions);
    return 0;
}

int main(int argc, char *argv[]) {
    Args args;
    if (parse_args(argc, argv, &args) != 0) return EXIT_FAILURE;

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
