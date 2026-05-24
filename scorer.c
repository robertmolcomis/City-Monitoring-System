#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define NAME_LEN        32
#define CATEGORY_LEN    16
#define DESC_LEN        128
#define MAX_INSPECTORS  50

// Must perfectly match the struct in city_manager.c
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

// Used to aggregate scores
typedef struct {
    char inspector[NAME_LEN];
    int  total_score;
} InspectorScore;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <district_id>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *district = argv[1];
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    FILE *f = fopen(path, "rb");
    if (!f) {
        // We print to stdout rather than stderr so city_hub can pipe this output
        printf("--- Workload for %s ---\n", district);
        printf("  [No reports.dat file found for this district]\n\n");
        return EXIT_SUCCESS;
    }

    InspectorScore scores[MAX_INSPECTORS];
    int num_inspectors = 0;
    Report r;

    // Read the binary file and calculate sums
    while (fread(&r, sizeof(Report), 1, f) == 1) {
        int found = 0;
        for (int i = 0; i < num_inspectors; i++) {
            if (strcmp(scores[i].inspector, r.inspector) == 0) {
                scores[i].total_score += r.severity;
                found = 1;
                break;
            }
        }
        
        // If this is a new inspector, add them to the array
        if (!found && num_inspectors < MAX_INSPECTORS) {
            strncpy(scores[num_inspectors].inspector, r.inspector, NAME_LEN - 1);
            scores[num_inspectors].inspector[NAME_LEN - 1] = '\0';
            scores[num_inspectors].total_score = r.severity;
            num_inspectors++;
        }
    }
    fclose(f);

    // Print the summarized plain-text report
    printf("--- Workload for %s ---\n", district);
    if (num_inspectors == 0) {
        printf("  [District has no filed reports]\n");
    } else {
        for (int i = 0; i < num_inspectors; i++) {
            printf("  Inspector: %-15s | Total Workload Score: %d\n", scores[i].inspector, scores[i].total_score);
        }
    }
    printf("\n");

    return EXIT_SUCCESS;
}
