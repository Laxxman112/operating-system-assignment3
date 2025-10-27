/**
 * Assignment 3
 * Students:
 * 1. Dharmapriya Bandara
 * 2. Laxman
 * 3. Jianda Hou
 * 
 * Parallel merge sort implementation.
 * Follows the Assignment 3 specification:
 * - my_mergesort: single-threaded recursive mergesort
 * - merge: merges two sorted subarrays [leftstart..leftend] and [rightstart..rightend]
 * - parallel_mergesort: spawns threads until level == cutoff, then falls back to my_mergesort
 * - buildArgs: heap-allocates the argument struct for thread entry
 */

#include <stdio.h>
#include <string.h> /* for memcpy */
#include <stdlib.h> /* for malloc */
#include <pthread.h>
#include "mergesort.h"


/* Merge two already-sorted subarrays of A using B as scratch:
   Left:  A[leftstart..leftend]
   Right: A[rightstart..rightend]
   Result is written back into A[leftstart..rightend].
*/
/* this function will be called by mergesort() and also by parallel_mergesort(). */
void merge(int leftstart, int leftend, int rightstart, int rightend) {
    int i = leftstart;
    int j = rightstart;
    int k = leftstart;

    /* Copy the whole span we will overwrite into B once, then merge from B->A. */
    memcpy(B + leftstart, A + leftstart, (rightend - leftstart + 1) * sizeof(int));

    /* Now B[leftstart..leftend] is original left, and B[rightstart..rightend] is original right. */
    while (i <= leftend && j <= rightend) {
        if (B[i] <= B[j]) {
            A[k++] = B[i++];
        } else {
            A[k++] = B[j++];
        }
    }
    while (i <= leftend)  A[k++] = B[i++];
    while (j <= rightend) A[k++] = B[j++];
}

/* Standard recursive single-threaded mergesort on A[left..right]. */
void my_mergesort(int left, int right) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    my_mergesort(left, mid);
    my_mergesort(mid + 1, right);
    merge(left, mid, mid + 1, right);
}

/* Thread entry for parallel merge sort.
   If current level == cutoff, run single-threaded my_mergesort.
   Otherwise, split, spawn two threads with level+1, join, then merge.
*/
void * parallel_mergesort(void *arg) {
    struct argument *a = (struct argument *)arg;
    int left  = a->left;
    int right = a->right;
    int level = a->level;

    if (left < right) {
        if (level >= cutoff) {
            /* Base case for threading: switch to single-threaded mergesort */
            my_mergesort(left, right);
        } else {
            int mid = left + (right - left) / 2;

            /* Prepare arguments for left and right halves */
            struct argument *leftArg  = buildArgs(left,     mid,     level + 1);
            struct argument *rightArg = buildArgs(mid + 1,  right,   level + 1);

            pthread_t t1, t2;

            /* Spawn two threads that recurse further (indirect recursion via pthread_create) */
            int rc1 = pthread_create(&t1, NULL, parallel_mergesort, (void*)leftArg);
            int rc2 = pthread_create(&t2, NULL, parallel_mergesort, (void*)rightArg);

            if (rc1 == 0) pthread_join(t1, NULL);
            else {
                /* Fallback if thread creation failed: do left half serially */
                my_mergesort(left, mid);
                free(leftArg);
            }

            if (rc2 == 0) pthread_join(t2, NULL);
            else {
                /* Fallback if thread creation failed: do right half serially */
                my_mergesort(mid + 1, right);
                free(rightArg);
            }

            /* After both halves are sorted, merge them */
            merge(left, mid, mid + 1, right);
        }
    }

    /* Free the argument for this call */
    free(a);
    return NULL;
}

/* Helper to allocate and populate thread argument on the heap. */
struct argument * buildArgs(int left, int right, int level) {
    struct argument *a = (struct argument *)malloc(sizeof(struct argument));
    if (!a) {
        /* In a constrained environment, you might want to handle this more gracefully. */
        perror("malloc failed in buildArgs");
        exit(EXIT_FAILURE);
    }
    a->left = left;
    a->right = right;
    a->level = level;
    return a;
}
