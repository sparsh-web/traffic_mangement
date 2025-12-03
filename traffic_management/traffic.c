// traffic.c  (Option D: Emergency preemption + extension for NORMAL sim)
// Replace your traffic.c with this file.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "traffic.h"
#include "utils.h"
#include "emergency.h"

#define REALISTIC_CAR_TIME 3
#define MAX_THROUGHPUT 8
#define MAX_ROADS 20

// CSV logger
void log_cycle_full(FILE *fp, int cycle, int road, int arrived, int passed, int waiting, int emergency, int green_time) {
    fprintf(fp, "%d,%d,%d,%d,%d,%d,%d\n",
        cycle, road, arrived, passed, waiting, emergency, green_time);
}

// arrivals generator (simple poisson-ish mean)
int generate_arrivals(double rate, double secs) {
    double mean = rate * (secs / 60.0);
    if (mean < 0) mean = 0;
    return (int)(mean + 0.5);
}

// serve with emergency-first behavior
int serve_lane(RoadState *r, int allowed) {
    int served = 0;

    // Serve emergency vehicles first
    while (served < allowed && r->emergency > 0) {
        r->emergency--;
        r->total_passed++;
        served++;
    }

    // Serve normal vehicles
    while (served < allowed && r->normal > 0) {
        r->normal--;
        r->total_passed++;
        served++;
    }

    return served;
}

int cars_allowed_by_time(int green_secs) {
    int allowed = green_secs / REALISTIC_CAR_TIME;
    if (allowed < 1) allowed = 1;
    if (allowed > MAX_THROUGHPUT) allowed = MAX_THROUGHPUT;
    return allowed;
}

// -------------------- NORMAL SIMULATION (with emergency handling) --------------------
void run_normal_simulation(RoadState roads[], int n, int green_secs, int red_secs, int minutes) {

    printf("\n===== NORMAL SIMULATION (with Emergency Preemption) =====\n");

    int cycle_secs = green_secs + red_secs;
    int cycles = (minutes * 60) / cycle_secs;
    if (cycles < 1) cycles = 1;

    FILE *fp = fopen("traffic_data.csv", "a");
    if (!fp) {
        printf("Error opening traffic_data.csv for logging (normal).\n");
        return;
    }

    for (int c = 1; c <= cycles; c++) {
        printf("\n-- Cycle %d (Normal) --\n", c);

        // For each road, generate arrivals for the whole cycle period
        for (int i = 0; i < n; i++) {
            int arrivals = generate_arrivals(roads[i].arrival_rate, cycle_secs);

            int new_em = 0;
            for (int k = 0; k < arrivals; k++)
                if (rand_percent(5)) new_em++;

            roads[i].emergency += new_em;
            roads[i].normal += (arrivals - new_em);
        }

        // We will serve roads, but if any lane has emergency, that lane gets immediate priority (preemption)
        int served_flag[MAX_ROADS] = {0};
        int served_count = 0;
        int idx = 0;

        while (served_count < n) {
            // find any global emergency lane (returns -1 if none)
            int em_lane = find_global_emergency((int *)&roads[0].emergency, n);

            int lane_to_serve = -1;

            if (em_lane != -1 && !served_flag[em_lane]) {
                // Emergency preemption: serve this lane immediately
                lane_to_serve = em_lane;

                // Extend green for emergency: fixed green + emergency_count*2 seconds
                int emergency_count = roads[em_lane].emergency;
                int effective_green = green_secs + (emergency_count * 2);
                effective_green = clamp(effective_green, 5, 300); // safety clamp

                int allowed = cars_allowed_by_time(effective_green);
                int passed = serve_lane(&roads[lane_to_serve], allowed);

                int waiting = roads[lane_to_serve].normal + roads[lane_to_serve].emergency;

                printf("Emergency PREEMPT → Road %d | Arrived: 0 | Passed: %d | Waiting: %d | Green=%ds (extended)\n",
                    lane_to_serve + 1, passed, waiting, effective_green);

                // Log using actual arrivals for this cycle: we did generate arrivals earlier,
                // but to keep CSV consistent we may log arrivals_per_road elsewhere.
                // Here we log 0 for arrivals since arrivals were already logged per-road earlier in cycle.
                log_cycle_full(fp, c, lane_to_serve + 1,
                               0, passed, waiting,
                               roads[lane_to_serve].emergency,
                               effective_green);

                served_flag[lane_to_serve] = 1;
                served_count++;
            } else {
                // No emergency (or emergency already served) -> follow normal round-robin fixed green
                // pick next unserved lane
                int j;
                for (j = 0; j < n; j++) {
                    int candidate = (idx + j) % n;
                    if (!served_flag[candidate]) {
                        lane_to_serve = candidate;
                        idx = candidate + 1;
                        break;
                    }
                }
                if (lane_to_serve == -1) break; // safety

                // If this lane has emergencies at this moment but em_lane was -1, we still handle (rare)
                int emergency_count = roads[lane_to_serve].emergency;
                int effective_green = green_secs;
                if (emergency_count > 0) {
                    // give extra green for emergency in same lane (also preemptive behavior)
                    effective_green = green_secs + (emergency_count * 2);
                    effective_green = clamp(effective_green, 5, 300);
                }

                int allowed = cars_allowed_by_time(effective_green);
                int passed = serve_lane(&roads[lane_to_serve], allowed);

                int waiting = roads[lane_to_serve].normal + roads[lane_to_serve].emergency;

                printf("Road %d → Arrived: 0 | Passed: %d | Waiting: %d | Green=%ds\n",
                       lane_to_serve + 1, passed, waiting, effective_green);

                log_cycle_full(fp, c, lane_to_serve + 1,
                               0, passed, waiting,
                               roads[lane_to_serve].emergency,
                               effective_green);

                served_flag[lane_to_serve] = 1;
                served_count++;
            }
        } // end while serve loop
    } // end cycles

    fclose(fp);
    printf("\nNormal Simulation Completed.\n");
}

// -------------------- ADAPTIVE SIMULATION (unchanged except logging) --------------------
void run_adaptive_simulation(RoadState roads[], int n, int red_secs, int minutes) {

    printf("\n===== ADAPTIVE SIMULATION =====\n");

    double approx_cycle = red_secs + 30;
    int cycles = (int)((minutes * 60) / approx_cycle);
    if (cycles < 1) cycles = 1;

    FILE *fp = fopen("traffic_data.csv", "a");
    if (!fp) {
        printf("Error opening traffic_data.csv for logging (adaptive).\n");
        return;
    }

    int arrivals_per_road[MAX_ROADS] = {0};

    for (int c = 1; c <= cycles; c++) {
        printf("\n-- Cycle %d (Adaptive) --\n", c);

        // ARRIVALS first
        for (int i = 0; i < n; i++) {
            int arrivals = generate_arrivals(roads[i].arrival_rate, approx_cycle);
            arrivals_per_road[i] = arrivals;

            int new_em = 0;
            for (int k = 0; k < arrivals; k++)
                if (rand_percent(5)) new_em++;

            roads[i].emergency += new_em;
            roads[i].normal += (arrivals - new_em);
        }

        int served_flag[MAX_ROADS] = {0};
        int served = 0;
        int idx = 0;

        while (served < n) {

            int em_lane = find_global_emergency((int *)&roads[0].emergency, n);

            int to_serve = -1;

            if (em_lane != -1 && !served_flag[em_lane]) {
                to_serve = em_lane;
            } else {
                for (int off = 0; off < n; off++) {
                    int j = (idx + off) % n;
                    if (!served_flag[j]) {
                        to_serve = j;
                        idx = j + 1;
                        break;
                    }
                }
            }

            int adaptive_green =
                5 + roads[to_serve].normal + 2 * roads[to_serve].emergency;
            adaptive_green = clamp(adaptive_green, 5, 300);

            int allowed = cars_allowed_by_time(adaptive_green);
            int passed = serve_lane(&roads[to_serve], allowed);

            int waiting = roads[to_serve].normal + roads[to_serve].emergency;

            printf("Road %d → Arrived: %d | Passed: %d | Waiting: %d | Green: %ds%s\n",
                to_serve + 1,
                arrivals_per_road[to_serve],
                passed,
                waiting,
                adaptive_green,
                (em_lane == to_serve) ? " [EMERGENCY]" : "");

            // LOG with correct arrivals + adaptive green
            if (fp)
                log_cycle_full(fp, c, to_serve + 1,
                    arrivals_per_road[to_serve],
                    passed,
                    waiting,
                    roads[to_serve].emergency,
                    adaptive_green);

            served_flag[to_serve] = 1;
            served++;
        }

    } // cycles

    fclose(fp);
    printf("\nAdaptive Simulation Completed.\n");
}
