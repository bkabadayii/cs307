#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

// Fan type: 0 = A, 1 = B
int FAN_A = 0;
int FAN_B = 1;

// In order to keep car information
struct car
{
    int numFans[2]; // Num fans reserved the car, numFans[0]: Team A, numFans[1]: Team B
    int inCar; // Num fans got inside the car

    // Default constructor
    car() {
        numFans[0] = 0;
        numFans[1] = 0;
        inCar = 0;
    }
};

// Class handling the program flow and synchronization
class SynchronizationMechanism {
public:
    int NUM_GROUPS;
    int CAR_ID;

    sem_t * groupCheckSync; // Will be used as a lock when checking group availability
    sem_t * carWaitSync; // Will be used as a barrier when waiting others to complete the group

    sem_t captainLock; // Will be used as a lock for car groups' output to be consecutive
    sem_t getIntoCarLock; // Will be used as a lock when getting in the car
    sem_t fanWait; // Will be used as a barrier when waiting others to get in the car

    car* carInfo; // Will be used to keep the cars2 information

    // Initializes synchronization mechanism
    void initSynchronizationMechanism (int numGroups) {
        NUM_GROUPS = numGroups;
        CAR_ID = 0;

        groupCheckSync = new sem_t[numGroups];
        carWaitSync = new sem_t[numGroups];
        carInfo = new car[numGroups];

        for (int i = 0; i < numGroups; i++) {
            sem_init(&groupCheckSync[i], 0, 1); 
            sem_init(&carWaitSync[i], 0, 0);
        }

        sem_init(&captainLock, 0, 1);
        sem_init(&getIntoCarLock, 0, 1);  
        sem_init(&fanWait, 0, 0);        
    }

    // Checks group availability for a fan
    // If group is available for the fan, the fan reserves a spot in that group's car
    // If the fan is the last one reserving the car, set them as the captain
    bool checkAndReserveGroup(int groupIdx, int fanType, bool &isCaptain) {
        bool check = true;
        // Acquire lock when checking
        sem_wait(&(groupCheckSync[groupIdx]));
        
        int numFansA = carInfo[groupIdx].numFans[0];
        int numFansB = carInfo[groupIdx].numFans[1];

        // If the car is full, return false
        if (numFansA + numFansB == 4) {
            sem_post(&(groupCheckSync[groupIdx]));
            return false;
        }

        // Check if the fan satisfies the condition to reserve a spot in the group's car 
        if (fanType == 0) {
            if (numFansB > 2) {
                check = false;
            }
            else if (numFansA > 1 && numFansB > 0) {
                check = false;
            }
        }
        else if (fanType == 1) {
            if (numFansA > 2) {
                check = false;
            }
            else if (numFansB > 1 && numFansA > 0) {
                check = false;
            }
        }
        else {
            printf("Invalid Fan Type!\n");
            exit(1);
        } 

        // Reserve a spot in the group if available
        if (check) {
            carInfo[groupIdx].numFans[fanType]++;
            // If the fan is the last one, set them is the captain 
            if (carInfo[groupIdx].numFans[0] + carInfo[groupIdx].numFans[1] == 4) {
                isCaptain = true;
            }
        }

        // Release lock
        sem_post(&(groupCheckSync[groupIdx]));
        return check;
    }

    // Waits for a group to complete
    // When captain arrives, all fans are are awaken and they get into the car
    void waitAndGetIntoCar (int groupIdx, char team, bool isCaptain) {
        // If a fan is not the captain
        if (!isCaptain) {
            // Wait for captain to arrive
            sem_wait(&(carWaitSync[groupIdx]));

            // After captain arrives, acquire lock and get into the car
            sem_wait(&getIntoCarLock);
            carInfo[groupIdx].inCar++;
            printf("Thread ID: %lu, Team: %c, I have found a spot in a car\n", pthread_self(), team);
            fsync(STDOUT_FILENO);
            // If the fan is last one getting into the car, awake others
            if (carInfo[groupIdx].inCar == 4) {
                sem_post(&fanWait);
                sem_post(&fanWait);
                sem_post(&fanWait);
                sem_post(&fanWait);
            }
            sem_post(&getIntoCarLock);
            // Wait for everyone to get into the car
            sem_wait(&fanWait);
        }
        // If a fan is the captain
        else {
            // Get into the car
            carInfo[groupIdx].inCar++;
            // Acquire captain lock to block other groups from printing
            sem_wait(&captainLock);
            printf("Thread ID: %lu, Team: %c, I have found a spot in a car\n", pthread_self(), team);
            fsync(STDOUT_FILENO);

            // Send signal to other fans to get into car
            sem_post(&(carWaitSync[groupIdx]));
            sem_post(&(carWaitSync[groupIdx]));
            sem_post(&(carWaitSync[groupIdx]));

            // Wait for everyone to get into the car
            sem_wait(&fanWait);
            printf("Thread ID: %lu, Team: %c, I am the captain and driving the car with ID %i\n", pthread_self(), team, CAR_ID);
            fsync(STDOUT_FILENO);
            CAR_ID++;
            // Release captain lock 
            sem_post(&captainLock);
        }
    }

    // Destructor to free dynamic memory
    ~SynchronizationMechanism() {
        delete[] groupCheckSync;
        delete[] carWaitSync;
        delete[] carInfo;
    }

    // FOR DEBUGGING
    void printAllCars() {
        for (int i = 0; i < NUM_GROUPS; i++) {
            car currentCar = carInfo[i];
            printf("CAR NO: %i\n", i);
            printf("---------\n");
            printf("Fans from team A: %i\n", currentCar.numFans[0]);
            printf("Fans from team B: %i\n", currentCar.numFans[1]);
        }
    }
};

// Global sync mechanism to handle thread synchronization
SynchronizationMechanism synchronizationMechanism;

void *fanThread(void *args)
{
    int fanType = *((int *)args);
    char team;
    if (fanType) {
        team = 'B';
    }
    else {
        team = 'A';
    }
    printf("Thread ID: %lu, Team: %c, I am looking for a car\n", pthread_self(), team);

    // Iterate over groups to reserve a car 
    for (int i = 0; i < synchronizationMechanism.NUM_GROUPS; i++) {
        bool isCaptain = false;
        bool reserved = synchronizationMechanism.checkAndReserveGroup(i, fanType, isCaptain);   
        // If reserve is not successful, look for the next group
        if (!reserved) {
            continue;
        }
        // Wait for group to form and then get into car
        synchronizationMechanism.waitAndGetIntoCar(i, team, isCaptain);
        break;
    }
}

int main(int argc, char *argv[])
{
    // Get arguments from console
    int numFansA = atoi(argv[1]);
    int numFansB = atoi(argv[2]);

    // Check if arguments are valid, if not return from main
    if (numFansA % 2 != 0 || numFansB % 2 != 0 || (numFansA + numFansB) % 4 != 0)
    {
        printf("The main terminates\n");
        return 0;
    }

    // If there are no errors regarding inputs, start the program
    int numThreads = numFansA + numFansB;
    int numGroups = (numFansA + numFansB) / 4;

    // Initialize synchronization mechanism
    synchronizationMechanism.initSynchronizationMechanism(numGroups);

    // Initialize fan threads
    pthread_t fanThreads[numThreads];

    // Create team A fan threads
    for (int i = 0; i < numFansA; i++)
    {
        pthread_create(&fanThreads[i], NULL, fanThread, &FAN_A);
    }
    // Create team B fan threads
    for (int i = 0; i < numFansB; i++)
    {
        pthread_create(&fanThreads[numFansA + i], NULL, fanThread, &FAN_B);
    }

    // Wait all fan threads to terminate before returning
    for (int t = 0; t < numThreads; t++)
    {
        pthread_join(fanThreads[t], NULL);
    }

    printf("The main terminates\n");
    return 0;
}