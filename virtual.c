/*
 * virtual.c — Virtual Memory Lab
 * Implements FIFO, LRU, and LFU page replacement algorithms.
 *
 * Expected functions (called by provided main.c):
 *   void process_page_access_fifo(int page_number);
 *   int  count_page_faults_fifo(int num_frames);
 *   void process_page_access_lru(int page_number);
 *   int  count_page_faults_lru(int num_frames);
 *   void process_page_access_lfu(int page_number);
 *   int  count_page_faults_lfu(int num_frames);
 *
 * Each algorithm maintains its own internal state
 * across calls to process_page_access_*().
 */

#include <stdio.h>
#include <stdlib.h>

#define MAX_PAGES 10000

/* ---------- FIFO ---------- */

static int fifo_frames[100];
static int fifo_queue_index = 0;
static int fifo_count = 0;
static int fifo_faults = 0;
static int fifo_num_frames = 0;

void process_page_access_fifo(int page_number) {
    /* check if page already exists in frames */
    int hit = 0;
    for (int i = 0; i < fifo_count; i++) {
        if (fifo_frames[i] == page_number) {
            hit = 1;
            break;
        }
    }
    if (hit) return;

    /* page fault */
    fifo_faults++;

    if (fifo_count < fifo_num_frames) {
        fifo_frames[fifo_count++] = page_number;
    } else {
        fifo_frames[fifo_queue_index] = page_number;
        fifo_queue_index = (fifo_queue_index + 1) % fifo_num_frames;
    }
}

int count_page_faults_fifo(int num_frames) {
    fifo_num_frames = num_frames;
    fifo_queue_index = 0;
    fifo_count = 0;
    fifo_faults = 0;

    /* main.c will call process_page_access_fifo() for each page,
       then call this function at the end to return the fault count */
    return fifo_faults;
}

/* ---------- LRU ---------- */

static int lru_frames[100];
static int lru_last_used[100];
static int lru_time = 0;
static int lru_count = 0;
static int lru_faults = 0;
static int lru_num_frames = 0;

void process_page_access_lru(int page_number) {
    lru_time++;
    /* check if page exists */
    int hit_index = -1;
    for (int i = 0; i < lru_count; i++) {
        if (lru_frames[i] == page_number) {
            hit_index = i;
            break;
        }
    }
    if (hit_index != -1) {
        lru_last_used[hit_index] = lru_time;
        return;
    }

    /* page fault */
    lru_faults++;
    if (lru_count < lru_num_frames) {
        lru_frames[lru_count] = page_number;
        lru_last_used[lru_count] = lru_time;
        lru_count++;
    } else {
        /* find least recently used */
        int lru_index = 0;
        for (int i = 1; i < lru_num_frames; i++) {
            if (lru_last_used[i] < lru_last_used[lru_index])
                lru_index = i;
        }
        lru_frames[lru_index] = page_number;
        lru_last_used[lru_index] = lru_time;
    }
}

int count_page_faults_lru(int num_frames) {
    lru_num_frames = num_frames;
    lru_count = 0;
    lru_faults = 0;
    lru_time = 0;
    return lru_faults;
}

/* ---------- LFU ---------- */

static int lfu_frames[100];
static int lfu_freq[100];
static int lfu_last_used[100];
static int lfu_time = 0;
static int lfu_count = 0;
static int lfu_faults = 0;
static int lfu_num_frames = 0;

void process_page_access_lfu(int page_number) {
    lfu_time++;
    /* check if page exists */
    int hit_index = -1;
    for (int i = 0; i < lfu_count; i++) {
        if (lfu_frames[i] == page_number) {
            hit_index = i;
            break;
        }
    }
    if (hit_index != -1) {
        lfu_freq[hit_index]++;
        lfu_last_used[hit_index] = lfu_time;
        return;
    }

    /* page fault */
    lfu_faults++;

    if (lfu_count < lfu_num_frames) {
        lfu_frames[lfu_count] = page_number;
        lfu_freq[lfu_count] = 1;
        lfu_last_used[lfu_count] = lfu_time;
        lfu_count++;
    } else {
        /* find least frequently used; break ties with LRU */
        int victim = 0;
        for (int i = 1; i < lfu_num_frames; i++) {
            if (lfu_freq[i] < lfu_freq[victim]) victim = i;
            else if (lfu_freq[i] == lfu_freq[victim] &&
                     lfu_last_used[i] < lfu_last_used[victim]) victim = i;
        }
        lfu_frames[victim] = page_number;
        lfu_freq[victim] = 1;
        lfu_last_used[victim] = lfu_time;
    }
}

int count_page_faults_lfu(int num_frames) {
    lfu_num_frames = num_frames;
    lfu_count = 0;
    lfu_faults = 0;
    lfu_time = 0;
    return lfu_faults;
}
