#include <stdio.h>
#include <limits.h>

#define TABLEMAX 100
#define POOLMAX 100
#define REFERENCEMAX 100

struct PTE {
    int is_valid;
    int frame_number;
    int arrival_timestamp;
    int last_access_timestamp;
    int reference_count;
};

// ------------------------------------
// Helper functions for victim selection
// ------------------------------------
static int fifo_victim(struct PTE pt[TABLEMAX], int table_cnt) {
    int victim = -1;
    int min_arrival = INT_MAX;
    for (int i = 0; i < table_cnt; i++) {
        if (pt[i].is_valid) {
            if (pt[i].arrival_timestamp < min_arrival ||
                (pt[i].arrival_timestamp == min_arrival && (victim == -1 || i < victim))) {
                min_arrival = pt[i].arrival_timestamp;
                victim = i;
            }
        }
    }
    return victim;
}

static int lru_victim(struct PTE pt[TABLEMAX], int table_cnt) {
    int victim = -1;
    int min_last = INT_MAX;
    for (int i = 0; i < table_cnt; i++) {
        if (pt[i].is_valid) {
            if (pt[i].last_access_timestamp < min_last ||
                (pt[i].last_access_timestamp == min_last && (victim == -1 || i < victim))) {
                min_last = pt[i].last_access_timestamp;
                victim = i;
            }
        }
    }
    return victim;
}

static int lfu_victim(struct PTE pt[TABLEMAX], int table_cnt) {
    int victim = -1;
    int min_ref = INT_MAX;
    int min_arrival = INT_MAX;
    for (int i = 0; i < table_cnt; i++) {
        if (pt[i].is_valid) {
            if (pt[i].reference_count < min_ref ||
                (pt[i].reference_count == min_ref && pt[i].arrival_timestamp < min_arrival) ||
                (pt[i].reference_count == min_ref && pt[i].arrival_timestamp == min_arrival && (victim == -1 || i < victim))) {
                min_ref = pt[i].reference_count;
                min_arrival = pt[i].arrival_timestamp;
                victim = i;
            }
        }
    }
    return victim;
}

// ------------------------------------
// Process Page Access FIFO
// ------------------------------------
int process_page_access_fifo(struct PTE page_table[TABLEMAX], int *table_cnt,
                             int page_number, int frame_pool[POOLMAX],
                             int *frame_cnt, int current_timestamp) {
    struct PTE *pte = &page_table[page_number];
    if (pte->is_valid) {
        pte->last_access_timestamp = current_timestamp;
        pte->reference_count++;
        return pte->frame_number;
    }
    if (*frame_cnt > 0) {
        int frame = frame_pool[0];
        for (int i = 0; i < *frame_cnt - 1; i++) frame_pool[i] = frame_pool[i + 1];
        (*frame_cnt)--;
        pte->is_valid = 1;
        pte->frame_number = frame;
        pte->arrival_timestamp = current_timestamp;
        pte->last_access_timestamp = current_timestamp;
        pte->reference_count = 1;
        return frame;
    }
    int victim = fifo_victim(page_table, *table_cnt);
    if (victim == -1) return -1;
    int frame = page_table[victim].frame_number;
    page_table[victim].is_valid = 0;
    page_table[victim].frame_number = -1;
    page_table[victim].arrival_timestamp = -1;
    page_table[victim].last_access_timestamp = -1;
    page_table[victim].reference_count = -1;
    pte->is_valid = 1;
    pte->frame_number = frame;
    pte->arrival_timestamp = current_timestamp;
    pte->last_access_timestamp = current_timestamp;
    pte->reference_count = 1;
    return frame;
}

// ------------------------------------
// Count Page Faults FIFO (timestamp starts at 1)
// ------------------------------------
int count_page_faults_fifo(struct PTE page_table[TABLEMAX], int table_cnt,
                           int reference_string[REFERENCEMAX], int reference_cnt,
                           int frame_pool[POOLMAX], int frame_cnt) {
    struct PTE pt[TABLEMAX];
    int pool[POOLMAX];
    int free_cnt = frame_cnt;
    int tc = table_cnt;

    for (int i = 0; i < table_cnt; i++) pt[i] = page_table[i];
    for (int i = 0; i < frame_cnt; i++) pool[i] = frame_pool[i];

    int faults = 0;
    int timestamp = 1;

    for (int i = 0; i < reference_cnt; i++) {
        int page = reference_string[i];
        int was_valid = pt[page].is_valid;
        if (!was_valid) faults++;
        process_page_access_fifo(pt, &tc, page, pool, &free_cnt, timestamp);
        timestamp++;
    }

    return faults;
}

// ------------------------------------
// Process Page Access LRU
// ------------------------------------
int process_page_access_lru(struct PTE page_table[TABLEMAX], int *table_cnt,
                            int page_number, int frame_pool[POOLMAX],
                            int *frame_cnt, int current_timestamp) {
    struct PTE *pte = &page_table[page_number];
    if (pte->is_valid) {
        pte->last_access_timestamp = current_timestamp;
        pte->reference_count++;
        return pte->frame_number;
    }
    if (*frame_cnt > 0) {
        int frame = frame_pool[0];
        for (int i = 0; i < *frame_cnt - 1; i++) frame_pool[i] = frame_pool[i + 1];
        (*frame_cnt)--;
        pte->is_valid = 1;
        pte->frame_number = frame;
        pte->arrival_timestamp = current_timestamp;
        pte->last_access_timestamp = current_timestamp;
        pte->reference_count = 1;
        return frame;
    }
    int victim = lru_victim(page_table, *table_cnt);
    if (victim == -1) return -1;
    int frame = page_table[victim].frame_number;
    page_table[victim].is_valid = 0;
    page_table[victim].frame_number = -1;
    page_table[victim].arrival_timestamp = -1;
    page_table[victim].last_access_timestamp = -1;
    page_table[victim].reference_count = -1;
    pte->is_valid = 1;
    pte->frame_number = frame;
    pte->arrival_timestamp = current_timestamp;
    pte->last_access_timestamp = current_timestamp;
    pte->reference_count = 1;
    return frame;
}

// ------------------------------------
// Count Page Faults LRU (timestamp starts at 1)
// ------------------------------------
int count_page_faults_lru(struct PTE page_table[TABLEMAX], int table_cnt,
                          int reference_string[REFERENCEMAX], int reference_cnt,
                          int frame_pool[POOLMAX], int frame_cnt) {
    struct PTE pt[TABLEMAX];
    int pool[POOLMAX];
    int free_cnt = frame_cnt;
    int tc = table_cnt;

    for (int i = 0; i < table_cnt; i++) pt[i] = page_table[i];
    for (int i = 0; i < frame_cnt; i++) pool[i] = frame_pool[i];

    int faults = 0;
    int timestamp = 1;

    for (int i = 0; i < reference_cnt; i++) {
        int page = reference_string[i];
        int was_valid = pt[page].is_valid;
        if (!was_valid) faults++;
        process_page_access_lru(pt, &tc, page, pool, &free_cnt, timestamp);
        timestamp++;
    }

    return faults;
}

// ------------------------------------
// Process Page Access LFU
// ------------------------------------
int process_page_access_lfu(struct PTE page_table[TABLEMAX], int *table_cnt,
                            int page_number, int frame_pool[POOLMAX],
                            int *frame_cnt, int current_timestamp) {
    struct PTE *pte = &page_table[page_number];
    if (pte->is_valid) {
        pte->last_access_timestamp = current_timestamp;
        pte->reference_count++;
        return pte->frame_number;
    }
    if (*frame_cnt > 0) {
        int frame = frame_pool[0];
        for (int i = 0; i < *frame_cnt - 1; i++) frame_pool[i] = frame_pool[i + 1];
        (*frame_cnt)--;
        pte->is_valid = 1;
        pte->frame_number = frame;
        pte->arrival_timestamp = current_timestamp;
        pte->last_access_timestamp = current_timestamp;
        pte->reference_count = 1;
        return frame;
    }
    int victim = lfu_victim(page_table, *table_cnt);
    if (victim == -1) return -1;
    int frame = page_table[victim].frame_number;
    page_table[victim].is_valid = 0;
    page_table[victim].frame_number = -1;
    page_table[victim].arrival_timestamp = -1;
    page_table[victim].last_access_timestamp = -1;
    page_table[victim].reference_count = -1;
    pte->is_valid = 1;
    pte->frame_number = frame;
    pte->arrival_timestamp = current_timestamp;
    pte->last_access_timestamp = current_timestamp;
    pte->reference_count = 1;
    return frame;
}

// ------------------------------------
// Count Page Faults LFU (timestamp starts at 1)
// ------------------------------------
int count_page_faults_lfu(struct PTE page_table[TABLEMAX], int table_cnt,
                          int reference_string[REFERENCEMAX], int reference_cnt,
                          int frame_pool[POOLMAX], int frame_cnt) {
    struct PTE pt[TABLEMAX];
    int pool[POOLMAX];
    int free_cnt = frame_cnt;
    int tc = table_cnt;

    for (int i = 0; i < table_cnt; i++) pt[i] = page_table[i];
    for (int i = 0; i < frame_cnt; i++) pool[i] = frame_pool[i];

    int faults = 0;
    int timestamp = 1;

    for (int i = 0; i < reference_cnt; i++) {
        int page = reference_string[i];
        int was_valid = pt[page].is_valid;
        if (!was_valid) faults++;
        process_page_access_lfu(pt, &tc, page, pool, &free_cnt, timestamp);
        timestamp++;
    }

    return faults;
}
