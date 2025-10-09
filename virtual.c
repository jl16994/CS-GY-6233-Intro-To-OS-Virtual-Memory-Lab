/*
 * virtual.c
 *
 * Virtual Memory lab functions. Matches signatures declared in oslabs.h.
 *
 * Changes per user request:
 *  - count_page_faults_fifo: timestamp starts at 1 (timestamp = i + 1)
 *  - count_page_faults_lru: timestamp starts at 0 (timestamp = i)
 *  - count_page_faults_lfu: unchanged (timestamp = i + 1)
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "oslabs.h"

/* Pop front frame from frame_pool (shift left). Returns -1 if empty */
static int pop_frame_front_int(int frame_pool[POOLMAX], int *frame_cnt) {
    if (*frame_cnt <= 0) return -1;
    int fn = frame_pool[0];
    for (int i = 1; i < *frame_cnt; ++i) frame_pool[i-1] = frame_pool[i];
    (*frame_cnt)--;
    return fn;
}

/* Choose FIFO victim: valid page with smallest arrival_timestamp */
static int choose_fifo_victim_pte(struct PTE *page_table, int table_cnt) {
    int victim = -1;
    int min_arrival = INT_MAX;
    for (int i = 0; i < table_cnt; ++i) {
        if (page_table[i].is_valid) {
            if (page_table[i].arrival_timestamp < min_arrival) {
                min_arrival = page_table[i].arrival_timestamp;
                victim = i;
            }
        }
    }
    return victim;
}

/* Choose LRU victim: valid page with smallest last_access_timestamp */
static int choose_lru_victim_pte(struct PTE *page_table, int table_cnt) {
    int victim = -1;
    int min_last = INT_MAX;
    for (int i = 0; i < table_cnt; ++i) {
        if (page_table[i].is_valid) {
            if (page_table[i].last_access_timestamp < min_last) {
                min_last = page_table[i].last_access_timestamp;
                victim = i;
            }
        }
    }
    return victim;
}

/* Choose LFU victim: smallest reference_count, tie-breaker: smallest arrival_timestamp */
static int choose_lfu_victim_pte(struct PTE *page_table, int table_cnt) {
    int victim = -1;
    int min_ref = INT_MAX;
    int min_arr = INT_MAX;
    for (int i = 0; i < table_cnt; ++i) {
        if (page_table[i].is_valid) {
            int rc = page_table[i].reference_count;
            int at = page_table[i].arrival_timestamp;
            if (rc < min_ref || (rc == min_ref && at < min_arr)) {
                min_ref = rc;
                min_arr = at;
                victim = i;
            }
        }
    }
    return victim;
}

/* Invalidate a PTE (set is_valid = 0 and mark fields to -1) */
static void invalidate_pte(struct PTE *p) {
    p->is_valid = 0;
    p->frame_number = -1;
    p->arrival_timestamp = -1;
    p->last_access_timestamp = -1;
    p->reference_count = -1;
}

/* ---------------- FIFO single access ---------------- */
int process_page_access_fifo(struct PTE *page_table, int *table_cnt, int page_number,
                             int *frame_pool, int *frame_cnt, int current_timestamp) {
    int tcnt = (table_cnt ? *table_cnt : TABLEMAX);
    if (page_number < 0 || page_number >= tcnt) return -1;

    if (page_table[page_number].is_valid) {
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count += 1;
        return page_table[page_number].frame_number;
    }

    if (*frame_cnt > 0) {
        int fn = pop_frame_front_int(frame_pool, frame_cnt);
        page_table[page_number].is_valid = 1;
        page_table[page_number].frame_number = fn;
        page_table[page_number].arrival_timestamp = current_timestamp;
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count = 1;
        return fn;
    }

    int victim = choose_fifo_victim_pte(page_table, tcnt);
    if (victim < 0) return -1;
    int freed = page_table[victim].frame_number;
    invalidate_pte(&page_table[victim]);

    page_table[page_number].is_valid = 1;
    page_table[page_number].frame_number = freed;
    page_table[page_number].arrival_timestamp = current_timestamp;
    page_table[page_number].last_access_timestamp = current_timestamp;
    page_table[page_number].reference_count = 1;
    return freed;
}

/* ---------------- FIFO page-fault counting ----------------
 * timestamp starts at 1 (timestamp = i + 1)
 */
int count_page_faults_fifo(struct PTE *page_table, int table_cnt,
                           int refrence_string[REFERENCEMAX], int reference_cnt,
                           int frame_pool[POOLMAX], int frame_cnt) {
    if (table_cnt <= 0) return 0;
    int faults = 0;

    for (int i = 0; i < reference_cnt; ++i) {
        int page = refrence_string[i];
        int timestamp = i + 1; /* start at 1 */

        if (page >= 0 && page < table_cnt && page_table[page].is_valid) {
            page_table[page].last_access_timestamp = timestamp;
            page_table[page].reference_count += 1;
        } else {
            faults++;
            if (frame_cnt > 0) {
                int fn = pop_frame_front_int(frame_pool, &frame_cnt);
                page_table[page].is_valid = 1;
                page_table[page].frame_number = fn;
                page_table[page].arrival_timestamp = timestamp;
                page_table[page].last_access_timestamp = timestamp;
                page_table[page].reference_count = 1;
            } else {
                int victim = choose_fifo_victim_pte(page_table, table_cnt);
                if (victim >= 0) {
                    int freed = page_table[victim].frame_number;
                    invalidate_pte(&page_table[victim]);

                    page_table[page].is_valid = 1;
                    page_table[page].frame_number = freed;
                    page_table[page].arrival_timestamp = timestamp;
                    page_table[page].last_access_timestamp = timestamp;
                    page_table[page].reference_count = 1;
                }
            }
        }
    }
    return faults;
}

/* ---------------- LRU single access ---------------- */
int process_page_access_lru(struct PTE *page_table, int *table_cnt, int page_number,
                            int *frame_pool, int *frame_cnt, int current_timestamp) {
    int tcnt = (table_cnt ? *table_cnt : TABLEMAX);
    if (page_number < 0 || page_number >= tcnt) return -1;

    if (page_table[page_number].is_valid) {
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count += 1;
        return page_table[page_number].frame_number;
    }

    if (*frame_cnt > 0) {
        int fn = pop_frame_front_int(frame_pool, frame_cnt);
        page_table[page_number].is_valid = 1;
        page_table[page_number].frame_number = fn;
        page_table[page_number].arrival_timestamp = current_timestamp;
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count = 1;
        return fn;
    }

    int victim = choose_lru_victim_pte(page_table, tcnt);
    if (victim < 0) return -1;
    int freed = page_table[victim].frame_number;
    invalidate_pte(&page_table[victim]);

    page_table[page_number].is_valid = 1;
    page_table[page_number].frame_number = freed;
    page_table[page_number].arrival_timestamp = current_timestamp;
    page_table[page_number].last_access_timestamp = current_timestamp;
    page_table[page_number].reference_count = 1;
    return freed;
}

/* ---------------- LRU page-fault counting ----------------
 * timestamp starts at 0 (timestamp = i)
 */
int count_page_faults_lru(struct PTE *page_table, int table_cnt,
                          int refrence_string[REFERENCEMAX], int reference_cnt,
                          int frame_pool[POOLMAX], int frame_cnt) {
    if (table_cnt <= 0) return 0;
    int faults = 0;

    for (int i = 0; i < reference_cnt; ++i) {
        int page = refrence_string[i];
        int timestamp = i; /* start at 0 as requested */

        if (page >= 0 && page < table_cnt && page_table[page].is_valid) {
            page_table[page].last_access_timestamp = timestamp;
            page_table[page].reference_count += 1;
        } else {
            faults++;
            if (frame_cnt > 0) {
                int fn = pop_frame_front_int(frame_pool, &frame_cnt);
                page_table[page].is_valid = 1;
                page_table[page].frame_number = fn;
                page_table[page].arrival_timestamp = timestamp;
                page_table[page].last_access_timestamp = timestamp;
                page_table[page].reference_count = 1;
            } else {
                int victim = choose_lru_victim_pte(page_table, table_cnt);
                if (victim >= 0) {
                    int freed = page_table[victim].frame_number;
                    invalidate_pte(&page_table[victim]);

                    page_table[page].is_valid = 1;
                    page_table[page].frame_number = freed;
                    page_table[page].arrival_timestamp = timestamp;
                    page_table[page].last_access_timestamp = timestamp;
                    page_table[page].reference_count = 1;
                }
            }
        }
    }
    return faults;
}

/* ---------------- LFU single access ---------------- */
int process_page_access_lfu(struct PTE *page_table, int *table_cnt, int page_number,
                            int *frame_pool, int *frame_cnt, int current_timestamp) {
    int tcnt = (table_cnt ? *table_cnt : TABLEMAX);
    if (page_number < 0 || page_number >= tcnt) return -1;

    if (page_table[page_number].is_valid) {
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count += 1;
        return page_table[page_number].frame_number;
    }

    if (*frame_cnt > 0) {
        int fn = pop_frame_front_int(frame_pool, frame_cnt);
        page_table[page_number].is_valid = 1;
        page_table[page_number].frame_number = fn;
        page_table[page_number].arrival_timestamp = current_timestamp;
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count = 1;
        return fn;
    }

    int victim = choose_lfu_victim_pte(page_table, tcnt);
    if (victim < 0) return -1;
    int freed = page_table[victim].frame_number;
    invalidate_pte(&page_table[victim]);

    page_table[page_number].is_valid = 1;
    page_table[page_number].frame_number = freed;
    page_table[page_number].arrival_timestamp = current_timestamp;
    page_table[page_number].last_access_timestamp = current_timestamp;
    page_table[page_number].reference_count = 1;
    return freed;
}

/* ---------------- LFU page-fault counting ----------------
 * timestamp starts at 1 (timestamp = i + 1) — left unchanged
 */
int count_page_faults_lfu(struct PTE *page_table, int table_cnt,
                          int refrence_string[REFERENCEMAX], int reference_cnt,
                          int frame_pool[POOLMAX], int frame_cnt) {
    if (table_cnt <= 0) return 0;
    int faults = 0;

    for (int i = 0; i < reference_cnt; ++i) {
        int page = refrence_string[i];
        int timestamp = i + 1; /* start at 1 */

        if (page >= 0 && page < table_cnt && page_table[page].is_valid) {
            page_table[page].last_access_timestamp = timestamp;
            page_table[page].reference_count += 1;
        } else {
            faults++;
            if (frame_cnt > 0) {
                int fn = pop_frame_front_int(frame_pool, &frame_cnt);
                page_table[page].is_valid = 1;
                page_table[page].frame_number = fn;
                page_table[page].arrival_timestamp = timestamp;
                page_table[page].last_access_timestamp = timestamp;
                page_table[page].reference_count = 1;
            } else {
                int victim = choose_lfu_victim_pte(page_table, table_cnt);
                if (victim >= 0) {
                    int freed = page_table[victim].frame_number;
                    invalidate_pte(&page_table[victim]);

                    page_table[page].is_valid = 1;
                    page_table[page].frame_number = freed;
                    page_table[page].arrival_timestamp = timestamp;
                    page_table[page].last_access_timestamp = timestamp;
                    page_table[page].reference_count = 1;
                }
            }
        }
    }
    return faults;
}
