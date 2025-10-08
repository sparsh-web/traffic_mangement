#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Vehicle structure
typedef struct Vehicle {
    char type[10]; // "Normal" or "Emergency"
    int id;
    struct Vehicle* next;
} Vehicle;

// Queue structure for each lane/road
typedef struct Queue {
    Vehicle* front;
    Vehicle* rear;
    int count;
} Queue;

// Function to create a new vehicle
Vehicle* createVehicle(int id, char* type) {
    Vehicle* v = (Vehicle*)malloc(sizeof(Vehicle));
    v->id = id;
    strcpy(v->type, type);
    v->next = NULL;
    return v;
}

// Initialize a queue
void initQueue(Queue* q) {
    q->front = q->rear = NULL;
    q->count = 0;
}

// Enqueue vehicle
void enqueue(Queue* q, Vehicle* v) {
    if (!q->rear) {
        q->front = q->rear = v;
    } else {
        q->rear->next = v;
        q->rear = v;
    }
    q->count++;
}

// Dequeue vehicle
Vehicle* dequeue(Queue* q) {
    if (!q->front) return NULL;
    Vehicle* v = q->front;
    q->front = q->front->next;
    if (!q->front) q->rear = NULL;
    q->count--;
    return v;
}

// Circular queue for signals
int nextSignal(int current, int totalSignals) {
    return (current + 1) % totalSignals;
}

// Manual input function
void manualInput(Queue lanes[], int totalSignals) {
    int n;
    printf("Enter number of vehicles to add manually: ");
    scanf("%d", &n);
    for (int i = 0; i < n; i++) {
        int lane;
        char type[10];
        int id;
        printf("Enter vehicle ID, type (Normal/Emergency), and lane (1-%d): ", totalSignals);
        scanf("%d %s %d", &id, type, &lane);
        if (lane < 1 || lane > totalSignals) {
            printf("Invalid lane. Skipping vehicle.\n");
            continue;
        }
        enqueue(&lanes[lane - 1], createVehicle(id, type));
    }
}

// Dataset input function
void datasetInput(Queue lanes[], int totalSignals) {
    FILE* fp = fopen("traffic_data.txt", "r");
    if (!fp) {
        printf("Dataset file not found.\n");
        return;
    }
    int id, lane;
    char type[10];
    while (fscanf(fp, "%d %s %d", &id, type, &lane) == 3) {
        if (lane < 1 || lane > totalSignals) continue;
        enqueue(&lanes[lane - 1], createVehicle(id, type));
    }
    fclose(fp);
}

// Main simulation
int main() {
    int totalSignals;
    printf("Enter number of roads/signals: ");
    scanf("%d", &totalSignals);

    Queue* lanes = (Queue*)malloc(totalSignals * sizeof(Queue));
    for (int i = 0; i < totalSignals; i++) {
        initQueue(&lanes[i]);
    }

    int choice;
    printf("Select Input Method:\n1. Manual Input\n2. Dataset Input\nChoice: ");
    scanf("%d", &choice);
    if (choice == 1)
        manualInput(lanes, totalSignals);
    else if (choice == 2)
        datasetInput(lanes, totalSignals);
    else {
        printf("Invalid choice. Exiting.\n");
        free(lanes);
        return 0;
    }

    int* greenTime = (int*)malloc(totalSignals * sizeof(int));
    for (int i = 0; i < totalSignals; i++) greenTime[i] = 5; // default green time

    int currentSignal = 0;
    printf("\nTraffic Simulation Started:\n\n");

    for (int t = 0; t < 10; t++) { // simulate 10 time units
        printf("Signal %d GREEN for %d sec\n", currentSignal + 1, greenTime[currentSignal]);

        // Serve emergency vehicles first
        if (lanes[currentSignal].front && strcmp(lanes[currentSignal].front->type, "Emergency") == 0) {
            Vehicle* v = dequeue(&lanes[currentSignal]);
            printf("Emergency vehicle %d passed from lane %d\n", v->id, currentSignal + 1);
            free(v);
        }
        // Serve normal vehicle
        else if (lanes[currentSignal].front) {
            Vehicle* v = dequeue(&lanes[currentSignal]);
            printf("Vehicle %d passed from lane %d\n", v->id, currentSignal + 1);
            free(v);
        }

        currentSignal = nextSignal(currentSignal, totalSignals); // move to next signal
        printf("\n");
    }

    printf("Simulation Ended.\n");

    free(lanes);
    free(greenTime);
    return 0;
}
