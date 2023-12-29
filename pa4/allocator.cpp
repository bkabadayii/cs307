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
        // Head of the linked list
        HeapNode* head;
    public:
        // Heap initializer 
        int initHeap(int size) {
            printf("Memory initialized\n");
            head = new HeapNode(-1, size, 0);
            return 1;
        }

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

        // Allocates memory from the heap if there is a free chunk with enough size
        // Returns index of the chunk allocated if allocation succeeds
        // Returns -1 otherwise
        int myMalloc(int ID, int size) {
            HeapNode* temp = head;
            // Iterate over all chunks
            int allocatedIndex = -1;
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
            return allocatedIndex;
        }

};

HeapManager heapManager;

// TEMP MAIN
int main() {
    int val = heapManager.initHeap(100);
    heapManager.myMalloc(0, 40);
    heapManager.myMalloc(0, 20);
    heapManager.myMalloc(0, 30);
    heapManager.myMalloc(0, 11);
    heapManager.myMalloc(0, 6);
    heapManager.myMalloc(0, 4);
    heapManager.myMalloc(0, 8);

    printf("Execution is done\n");
    heapManager.print();
    return 0;
}