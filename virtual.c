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

// --- Helper for FIFO eviction ---
static int select_fifo_victim(struct PTE pt[TABLEMAX], int table_cnt) {
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

// --- Helper for LRU eviction ---
static int select_lru_victim(struct PTE pt[TABLEMAX], int table_cnt) {
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

// FIFO - Count Page Faults (Fixed)
int count_page_faults_fifo(struct PTE page_table[TABLEMAX], int table_cnt,
                          int reference_string[REFERENCEMAX], int reference_cnt,
                          int frame_pool[POOLMAX], int frame_cnt) {

    struct PTE working_pt[TABLEMAX];
    int working_pool[POOLMAX];
    int working_frame_cnt = frame_cnt;

    for (int i = 0; i < table_cnt; i++) working_pt[i] = page_table[i];
    for (int i = 0; i < frame_cnt; i++) working_pool[i] = frame_pool[i];

    int page_faults = 0;
    int current_timestamp = 0;

    for (int i = 0; i < reference_cnt; i++) {
        current_timestamp++; // FIX: increment before processing
        int page_number = reference_string[i];

        if (!working_pt[page_number].is_valid) {
            page_faults++;

            if (working_frame_cnt > 0) {
                int frame = working_pool[0];
                for (int j = 0; j < working_frame_cnt - 1; j++)
                    working_pool[j] = working_pool[j + 1];
                working_frame_cnt--;

                working_pt[page_number].is_valid = 1;
                working_pt[page_number].frame_number = frame;
                working_pt[page_number].arrival_timestamp = current_timestamp;
                working_pt[page_number].last_access_timestamp = current_timestamp;
                working_pt[page_number].reference_count = 1;
            } else {
                int victim_page = select_fifo_victim(working_pt, table_cnt);
                int frame = working_pt[victim_page].frame_number;

                working_pt[victim_page].is_valid = 0;
                working_pt[victim_page].frame_number = -1;
                working_pt[victim_page].arrival_timestamp = -1;
                working_pt[victim_page].last_access_timestamp = -1;
                working_pt[victim_page].reference_count = -1;

                working_pt[page_number].is_valid = 1;
                working_pt[page_number].frame_number = frame;
                working_pt[page_number].arrival_timestamp = current_timestamp;
                working_pt[page_number].last_access_timestamp = current_timestamp;
                working_pt[page_number].reference_count = 1;
            }
        } else {
            working_pt[page_number].last_access_timestamp = current_timestamp;
            working_pt[page_number].reference_count++;
        }
    }

    return page_faults;
}

// LRU - Count Page Faults (Fixed)
int count_page_faults_lru(struct PTE page_table[TABLEMAX], int table_cnt,
                         int reference_string[REFERENCEMAX], int reference_cnt,
                         int frame_pool[POOLMAX], int frame_cnt) {

    struct PTE working_pt[TABLEMAX];
    int working_pool[POOLMAX];
    int working_frame_cnt = frame_cnt;

    for (int i = 0; i < table_cnt; i++) working_pt[i] = page_table[i];
    for (int i = 0; i < frame_cnt; i++) working_pool[i] = frame_pool[i];

    int page_faults = 0;
    int current_timestamp = 0;

    for (int i = 0; i < reference_cnt; i++) {
        current_timestamp++; // FIX
        int page_number = reference_string[i];

        if (!working_pt[page_number].is_valid) {
            page_faults++;

            if (working_frame_cnt > 0) {
                int frame = working_pool[0];
                for (int j = 0; j < working_frame_cnt - 1; j++)
                    working_pool[j] = working_pool[j + 1];
                working_frame_cnt--;

                working_pt[page_number].is_valid = 1;
                working_pt[page_number].frame_number = frame;
                working_pt[page_number].arrival_timestamp = current_timestamp;
                working_pt[page_number].last_access_timestamp = current_timestamp;
                working_pt[page_number].reference_count = 1;
            } else {
                int victim_page = select_lru_victim(working_pt, table_cnt);
                int frame = working_pt[victim_page].frame_number;

                working_pt[victim_page].is_valid = 0;
                working_pt[victim_page].frame_number = -1;
                working_pt[victim_page].arrival_timestamp = -1;
                working_pt[victim_page].last_access_timestamp = -1;
                working_pt[victim_page].reference_count = -1;

                working_pt[page_number].is_valid = 1;
                working_pt[page_number].frame_number = frame;
                working_pt[page_number].arrival_timestamp = current_timestamp;
                working_pt[page_number].last_access_timestamp = current_timestamp;
                working_pt[page_number].reference_count = 1;
            }
        } else {
            working_pt[page_number].last_access_timestamp = current_timestamp;
            working_pt[page_number].reference_count++;
        }
    }

    return page_faults;
}
