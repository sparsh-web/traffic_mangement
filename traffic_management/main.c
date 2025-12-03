#include <stdio.h>
#include <stdlib.h>
#include "traffic.h"
#include "utils.h"
#include "emergency.h"

int main() {
    int n;
    printf("Enter number of roads/signals: ");
    scanf("%d", &n);

    if (n < 1 || n > 20) {
        printf("Invalid number of roads.\n");
        return 0;
    }

    RoadState roads[20];

    // Arrival rate input
    printf("\n--- Enter arrival rate for each road (cars per minute) ---\n");
    for (int i = 0; i < n; i++) {
        printf("Road %d arrival rate: ", i + 1);
        scanf("%lf", &roads[i].arrival_rate);
        roads[i].normal = 0;
        roads[i].emergency = 0;
        roads[i].total_passed = 0;
        roads[i].max_queue = 0;
        roads[i].cumulative_queue = 0.0;
    }

    int green_secs, red_secs, minutes;

    printf("\nEnter FIXED green time for NORMAL simulation (seconds): ");
    scanf("%d", &green_secs);

    printf("Enter red time for each signal (seconds): ");
    scanf("%d", &red_secs);

    printf("Enter total simulation time (minutes): ");
    scanf("%d", &minutes);

    // Reset CSV file before writing
    FILE *fp = fopen("traffic_data.csv", "w");
    if (!fp) {
        printf("Error creating CSV file.\n");
        return 0;
    }
    fprintf(fp, "cycle,road,arrived,passed,waiting,emergency,adaptive_green\n");
    fclose(fp);

    // ============================
    // RUN NORMAL SIMULATION
    // ============================
    printf("\n=====================================\n");
    printf(" RUNNING NORMAL (FIXED TIMING) SIMULATION \n");
    printf("=====================================\n");

    run_normal_simulation(roads, n, green_secs, red_secs, minutes);

    // Reinitialize roads before adaptive simulation
    for (int i = 0; i < n; i++) {
        roads[i].normal = 0;
        roads[i].emergency = 0;
        roads[i].total_passed = 0;
        roads[i].max_queue = 0;
        roads[i].cumulative_queue = 0.0;
    }

    // ============================
    // RUN ADAPTIVE SIMULATION
    // ============================
    printf("\n=====================================\n");
    printf(" RUNNING ADAPTIVE (SMART TIMING) SIMULATION \n");
    printf("=====================================\n");

    run_adaptive_simulation(roads, n, red_secs, minutes);

    printf("\nSimulation finished. Data written to traffic_data.csv\n");
    return 0;
}
