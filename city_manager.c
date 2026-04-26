#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

// Fixed-size limits as required by the specifications
#define NAME_LEN        32
#define CATEGORY_LEN    16
#define DESC_LEN        128
#define DISTRICT_LEN    64

#define ROLE_INSPECTOR  0
#define ROLE_MANAGER    1

// The fixed-size binary record structure
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

// Structure to hold parsed command-line arguments
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
    fprintf(stderr, "Usage: %s --role <inspector|manager> --user <n> --<command> [args...]\n", prog);
}

// Parses the CLI arguments and populates the Args structure
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
        } else if (strcmp(argv[i], "--add") == 0 && i + 1 < argc) {
            strncpy(args->command, "add", 31); strncpy(args->district, argv[++i], DISTRICT_LEN - 1);
        } else if (strcmp(argv[i], "--list") == 0 && i + 1 < argc) {
            strncpy(args->command, "list", 31); strncpy(args->district, argv[++i], DISTRICT_LEN - 1);
        } else if (strcmp(argv[i], "--view") == 0 && i + 2 < argc) {
            strncpy(args->command, "view", 31);
            strncpy(args->district, argv[++i], DISTRICT_LEN - 1); args->report_id = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--remove_report") == 0 && i + 2 < argc) {
            strncpy(args->command, "remove_report", 31);
            strncpy(args->district, argv[++i], DISTRICT_LEN - 1); args->report_id = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--update_threshold") == 0 && i + 2 < argc) {
            strncpy(args->command, "update_threshold", 31);
            strncpy(args->district, argv[++i], DISTRICT_LEN - 1); args->threshold = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--filter") == 0 && i + 1 < argc) {
            strncpy(args->command, "filter", 31); strncpy(args->district, argv[++i], DISTRICT_LEN - 1);
            // Collect all filter conditions
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

// Helper: Converts permission bits to a 9-character string (e.g., "rw-rw-r--")
void mode_to_str(mode_t mode, char *str) {
    strcpy(str, "---------");
    if (mode & S_IRUSR) str[0] = 'r'; if (mode & S_IWUSR) str[1] = 'w'; if (mode & S_IXUSR) str[2] = 'x';
    if (mode & S_IRGRP) str[3] = 'r'; if (mode & S_IWGRP) str[4] = 'w'; if (mode & S_IXGRP) str[5] = 'x';
    if (mode & S_IROTH) str[6] = 'r'; if (mode & S_IWOTH) str[7] = 'w'; if (mode & S_IXOTH) str[8] = 'x';
}

// Helper: Checks if the current role has the requested read/write access based on file stats
int check_access(const char *path, int role, int need_read, int need_write) {
    struct stat st;
    if (stat(path, &st) != 0) return 1; // If file doesn't exist yet, allow operation to create it
    
    // Managers act as Owners (USR), Inspectors act as Group (GRP)
    int can_read = (role == ROLE_MANAGER) ? (st.st_mode & S_IRUSR) : (st.st_mode & S_IRGRP);
    int can_write = (role == ROLE_MANAGER) ? (st.st_mode & S_IWUSR) : (st.st_mode & S_IWGRP);
    
    if (need_read && !can_read) return 0;
    if (need_write && !can_write) return 0;
    return 1;
}

// Creates the district directory, files, explicit permissions, and symbolic links
void init_district(const char *dist) {
    struct stat st;
    // Create directory if it doesn't exist, set to 750 (rwxr-x---)
    if (stat(dist, &st) == -1) { mkdir(dist, 0750); chmod(dist, 0750); }
    char path[256], slink[256];
    
    // Create binary file, set to 664 (rw-rw-r--)
    snprintf(path, sizeof(path), "%s/reports.dat", dist);
    if (stat(path, &st) == -1) { int fd = open(path, O_CREAT, 0664); close(fd); chmod(path, 0664); }
    
    // Create config file, set to 640 (rw-r-----)
    snprintf(path, sizeof(path), "%s/district.cfg", dist);
    if (stat(path, &st) == -1) { int fd = open(path, O_CREAT, 0640); close(fd); chmod(path, 0640); }
    
    // Create log file, set to 644 (rw-r--r--)
    snprintf(path, sizeof(path), "%s/logged_district", dist);
    if (stat(path, &st) == -1) { int fd = open(path, O_CREAT, 0644); close(fd); chmod(path, 0644); }
    
    // Handle the symbolic link
    snprintf(path, sizeof(path), "%s/reports.dat", dist);
    snprintf(slink, sizeof(slink), "active_reports-%s", dist);
    
    // Use lstat to check if link exists without following it
    if (lstat(slink, &st) == 0) {
        if (S_ISLNK(st.st_mode)) {
            struct stat tgt;
            // Check if the link is dangling
            if (stat(slink, &tgt) != 0) fprintf(stderr, "Warning: Dangling symlink %s\n", slink);
        }
    } else symlink(path, slink); // Create link if it doesn't exist
}

// Logs actions to logged_district. Rejects inspectors writing to it.
void log_action(const Args *args) {
    char path[256]; snprintf(path, sizeof(path), "%s/logged_district", args->district);
    if (!check_access(path, args->role, 0, 1)) {
        fprintf(stderr, "Diagnostic: Inspector role refused write access to logged_district.\n"); return;
    }
    FILE *f = fopen(path, "a");
    if (f) {
        fprintf(f, "%ld\t%s\t%s\t%s\n", time(NULL), args->user, args->role == ROLE_MANAGER ? "manager" : "inspector", args->command);
        fclose(f);
    }
}

// AI-Assisted Function: Parses a "field:op:value" string
int parse_condition(const char *input, char *field, char *op, char *value) {
    char temp[64]; strncpy(temp, input, 63); temp[63] = '\0';
    char *t1 = strtok(temp, ":");
    char *t2 = strtok(NULL, ":");
    char *t3 = strtok(NULL, "");
    if (!t1 || !t2 || !t3) return 0;
    strcpy(field, t1); strcpy(op, t2); strcpy(value, t3);
    return 1;
}

// AI-Assisted Function: Matches a parsed condition against a Report record
int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int v = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == v;
        if (strcmp(op, "!=") == 0) return r->severity != v;
        if (strcmp(op, ">") == 0) return r->severity > v;
        if (strcmp(op, ">=") == 0) return r->severity >= v;
        if (strcmp(op, "<") == 0) return r->severity < v;
        if (strcmp(op, "<=") == 0) return r->severity <= v;
    } else if (strcmp(field, "category") == 0) {
        int cmp = strcmp(r->category, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
    } else if (strcmp(field, "inspector") == 0) {
        int cmp = strcmp(r->inspector, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
    }
    return 0;
}

static int cmd_add(const Args *args) {
    init_district(args->district);
    char path[256]; snprintf(path, sizeof(path), "%s/reports.dat", args->district);
    if (!check_access(path, args->role, 0, 1)) { fprintf(stderr, "Error: Access denied.\n"); return -1; }

    Report r; memset(&r, 0, sizeof(r));
    struct stat st; stat(path, &st);
    
    // Auto-increment ID based on file size
    r.id = (st.st_size / sizeof(Report)) + 1;
    strncpy(r.inspector, args->user, NAME_LEN - 1);
    r.timestamp = time(NULL);

    printf("X: "); scanf("%lf", &r.lat);
    printf("Y: "); scanf("%lf", &r.lon);
    printf("Category: "); scanf("%15s", r.category);
    printf("Severity (1/2/3): "); scanf("%d", &r.severity);
    
    // Clear input buffer before reading string with spaces
    int c; while ((c = getchar()) != '\n' && c != EOF);
    
    printf("Description: ");
    fgets(r.description, DESC_LEN, stdin); r.description[strcspn(r.description, "\n")] = 0; // Strip newline

    FILE *f = fopen(path, "ab"); // Append binary
    if (f) { fwrite(&r, sizeof(Report), 1, f); fclose(f); }
    log_action(args); return 0;
}

static int cmd_list(const Args *args) {
    init_district(args->district);
    char path[256]; snprintf(path, sizeof(path), "%s/reports.dat", args->district);
    if (!check_access(path, args->role, 1, 0)) { fprintf(stderr, "Error: Access denied.\n"); return -1; }

    struct stat st;
    if (stat(path, &st) == 0) {
        char perms[10]; mode_to_str(st.st_mode, perms);
        printf("File: %s | Perms: %s | Size: %ld bytes\n", path, perms, st.st_size);
    }
    
    FILE *f = fopen(path, "rb"); // Read binary
    if (f) {
        Report r;
        // Read struct-by-struct
        while (fread(&r, sizeof(Report), 1, f) == 1)
            printf("ID: %d | Cat: %s | Sev: %d | Desc: %s\n", r.id, r.category, r.severity, r.description);
        fclose(f);
    }
    log_action(args); return 0;
}

static int cmd_view(const Args *args) {
    char path[256]; snprintf(path, sizeof(path), "%s/reports.dat", args->district);
    if (!check_access(path, args->role, 1, 0)) { fprintf(stderr, "Error: Access denied.\n"); return -1; }

    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    Report r; int found = 0;
    while (fread(&r, sizeof(Report), 1, f) == 1) {
        if (r.id == args->report_id) {
            printf("--- Report %d ---\nInspector: %s\nCoords: %f, %f\nCategory: %s\nSeverity: %d\nDescription: %s\n",
                   r.id, r.inspector, r.lat, r.lon, r.category, r.severity, r.description);
            found = 1; break;
        }
    }
    fclose(f);
    if (!found) printf("Report %d not found.\n", args->report_id);
    log_action(args); return 0;
}

static int cmd_remove_report(const Args *args) {
    if (args->role != ROLE_MANAGER) { fprintf(stderr, "Error: Requires manager role\n"); return -1; }
    char path[256]; snprintf(path, sizeof(path), "%s/reports.dat", args->district);
    
    int fd = open(path, O_RDWR);
    if (fd < 0) return -1;
    
    Report r; off_t pos = 0, found_pos = -1;
    // Find the record to delete
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (r.id == args->report_id) { found_pos = pos; break; }
        pos += sizeof(Report);
    }
    
    if (found_pos != -1) {
        off_t read_pos = found_pos + sizeof(Report);
        off_t write_pos = found_pos;
        
        // Shift all subsequent records left by one block
        while (1) {
            lseek(fd, read_pos, SEEK_SET);
            if (read(fd, &r, sizeof(Report)) != sizeof(Report)) break;
            lseek(fd, write_pos, SEEK_SET);
            write(fd, &r, sizeof(Report));
            read_pos += sizeof(Report); write_pos += sizeof(Report);
        }
        // Truncate the file to chop off the leftover duplicate at the end
        ftruncate(fd, write_pos);
        printf("Report %d removed.\n", args->report_id);
    } else {
        printf("Report %d not found.\n", args->report_id);
    }
    close(fd); log_action(args); return 0;
}

static int cmd_update_threshold(const Args *args) {
    if (args->role != ROLE_MANAGER) { fprintf(stderr, "Error: Requires manager role\n"); return -1; }
    char path[256]; snprintf(path, sizeof(path), "%s/district.cfg", args->district);
    
    struct stat st;
    if (stat(path, &st) == 0) {
        // Validate strict 640 permissions before modifying
        if ((st.st_mode & 0777) != 0640) {
            fprintf(stderr, "Diagnostic: district.cfg permissions altered! Refusing update.\n");
            return -1;
        }
    }
    
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "THRESHOLD=%d\n", args->threshold); fclose(f); }
    log_action(args); return 0;
}

static int cmd_filter(const Args *args) {
    char path[256]; snprintf(path, sizeof(path), "%s/reports.dat", args->district);
    if (!check_access(path, args->role, 1, 0)) return -1;

    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    Report r;
    // Iterate over all reports and test against all given conditions (AND logic)
    while (fread(&r, sizeof(Report), 1, f) == 1) {
        int match_all = 1;
        for (int i = 0; i < args->nconditions; i++) {
            char field[32], op[8], val[32];
            if (parse_condition(args->conditions[i], field, op, val)) {
                if (!match_condition(&r, field, op, val)) { match_all = 0; break; }
            }
        }
        if (match_all) printf("ID: %d | Cat: %s | Sev: %d | Desc: %s\n", r.id, r.category, r.severity, r.description);
    }
    fclose(f); log_action(args); return 0;
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
    
    return ret;
}
