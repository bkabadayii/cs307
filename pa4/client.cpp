#include <random>
#include <iostream>
#include "allocator.cpp"

 // GLOBAL HEAP MANAGER
HeapManager heapManager;
int MEM_SIZE;
int MAX_ALLOCATION;
int NUM_THREADS;

void *threadCase4(void *args) {
    // Cast the void pointer back to int
    int id = *((int*) args);
    // Generate a random allocation size for each thread
    int size = rand() % MAX_ALLOCATION + 1;

    int allocated = heapManager.myMalloc(id, size);
    sleep(1);
    int freed = heapManager.myFree(id, allocated);   
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <memSize> <maxAllocation> <numThreads>" << std::endl;
        return 1;
    }
    // Convert command-line arguments to integers
    MEM_SIZE = atoi(argv[1]);
    MAX_ALLOCATION = atoi(argv[2]);
    NUM_THREADS = atoi(argv[3]);

    // Check if the numbers are positive
    if (MEM_SIZE <= 0 || MAX_ALLOCATION <= 0 || NUM_THREADS <= 0) {
        std::cerr << "Error: All input numbers must be positive." << std::endl;
        return 1;
    }

    // Initialize the heap
    heapManager.initHeap(MEM_SIZE);
    // Initialize threads
    pthread_t threads[NUM_THREADS];
    int threadIds[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        threadIds[i] = i;
        pthread_create(&threads[i], NULL, threadCase4, &threadIds[i]);
    }

    // Wait all threads to terminate before returning
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Execution is done\n");
    heapManager.print();
    return 0;
}