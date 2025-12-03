#ifndef TRAFFIC_H
#define TRAFFIC_H

typedef struct {
    int normal;
    int emergency;
    double arrival_rate;
    int total_passed;
    int max_queue;
    double cumulative_queue;
} RoadState;

void run_normal_simulation(RoadState roads[], int n, int green_secs, int red_secs, int total_minutes);
void run_adaptive_simulation(RoadState roads[], int n, int red_secs, int total_minutes);

int generate_arrivals(double rate, double secs);
int serve_lane(RoadState *r, int allowed);
int cars_allowed_by_time(int green_secs);
void copy_state(RoadState src[], RoadState dst[], int n);

#endif
