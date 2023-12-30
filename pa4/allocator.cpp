#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

struct HeapNode {
    int id;
    int size;
    int index;
    HeapNode* next;

    HeapNode(int _id, int _size ,int _index) {
        id = _id;
        size = _size;
        index = _index;
        next = NULL;
    }
};

class HeapManager {
    private:
        HeapNode* head; // Head of the linked list
        sem_t memLock; // Lock for memory management
    public:
        // Prints the current state of the heap
        void print() {
            HeapNode* temp = head;
            while (temp != NULL) {
                printf("[%i][%i][%i]", temp -> id, temp -> size, temp -> index);
                if (temp -> next != NULL) {
                    printf("---");
                }
                // Continue with the next chunk
                temp = temp -> next;
            }
            printf("\n");
        }

        // Heap initializer 
        int initHeap(int size) {
            printf("Memory initialized\n");
            head = new HeapNode(-1, size, 0);
            sem_init(&memLock, 0, 1);
            print();
            return 1;
        }

        // Allocates memory from the heap if there is a free chunk with enough size
        // Returns index of the chunk allocated if allocation succeeds
        // Returns -1 otherwise
        int myMalloc(int ID, int size) {
            int allocatedIndex = -1;
            // Lock memLock before checking
            sem_wait(&memLock);
            HeapNode* temp = head;
            // Iterate over all chunks
            while (temp != NULL && allocatedIndex == -1) {
                // If current chunk is free and its size >= requested size, allocate memory from that chunk
                if (temp -> id == -1 && temp -> size >= size) {
                    allocatedIndex = temp -> index;
                    int oldSize = temp -> size;
                    temp -> size = size;
                    temp -> id = ID;
                    // If all chunk is not used, allocate memory for the remaining empty chunk
                    if (oldSize > size) {
                        HeapNode* newEmptyChunk = new HeapNode(-1, oldSize - size, temp -> index + size);
                        newEmptyChunk -> next = temp -> next;
                        temp -> next = newEmptyChunk;
                    }
                }
                // Continue with the next chunk
                temp = temp -> next;
            }
            if (allocatedIndex > -1) {
                printf("Allocated for thread %i\n", ID);
            }
            else {
                printf("Can not allocate, requested size %i for thread %i is bigger than remaining size\n", size, ID);
            }
            print();
            // After the operation is done, release the lock
            sem_post(&memLock);
            return allocatedIndex;
        }

        // Frees an allocated memory if it exists
        // Returns 1 if free succeeds
        // Returns -1 otherwise
        int myFree(int ID, int index) {
            HeapNode* prev = NULL;
            // Iterate over all chunks
            int isFreed = -1;
            // Lock memLock before checking
            sem_wait(&memLock);
            HeapNode* temp = head;
            while (temp != NULL && isFreed == -1) {
                // If current chunk's id and index is equal to given values, perform free operation
                if (temp -> id == ID && temp -> index == index) {
                    // Check if left and right chunks are empty
                    bool leftEmpty = false;
                    bool rightEmpty = false;
                    if (prev && prev -> id == -1) {
                        leftEmpty = true;
                    }
                    if (temp -> next && temp -> next -> id == -1) {
                        rightEmpty = true;
                    }

                    // If left chunk is empty, destroy other empty chunks and combine their information with the left chunk
                    if (leftEmpty) {
                        HeapNode* connection = temp -> next;
                        int newSize = prev -> size + temp -> size;
                        // If right chunk is also empty, increment newSize by its size and shift connection to right
                        if (rightEmpty) {
                            connection = connection -> next;
                            newSize += temp -> next -> size;
                            // Destroy right chunk
                            delete temp -> next;
                        }
                        // Destroy current chunk
                        delete temp;
                        // Update left chunks information
                        prev -> id = -1;
                        prev -> size = newSize;
                        prev -> next = connection;
                    }
                    // Else, keep current chunk and destroy right chunk (if empty) and combine its information with the current chunk
                    else {
                        HeapNode* connection = temp -> next;
                        int newSize = temp -> size;
                        // If right chunk is empty, increment newSize by its size and shift connection to right
                        if (rightEmpty) {
                            connection = connection -> next;
                            newSize += temp -> next -> size;
                            // Destroy right chunk
                            delete temp -> next;
                        }
                        // Update current chunks information
                        temp -> id = -1;
                        temp -> size = newSize;
                        temp -> next = connection;
                    }
                    isFreed = 1;
                    break;
                }
                prev = temp;
                temp = temp -> next;
            }
            if (isFreed) {
                printf("Freed for thread %i\n", ID);
            }
            else {
                printf("Free failed for thread %i\n", ID);
            }
            print();
            sem_post(&memLock);
            return isFreed;
        }

};