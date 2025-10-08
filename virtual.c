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

// FIFO - Process Page Access
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
        for (int i = 0; i < *frame_cnt - 1; i++) {
            frame_pool[i] = frame_pool[i + 1];
        }
        (*frame_cnt)--;
        
        pte->is_valid = 1;
        pte->frame_number = frame;
        pte->arrival_timestamp = current_timestamp;
        pte->last_access_timestamp = current_timestamp;
        pte->reference_count = 1;
        
        return frame;
    }
    
    int victim_page = -1;
    int min_arrival = INT_MAX;
    
    for (int i = 0; i < *table_cnt; i++) {
        if (page_table[i].is_valid && page_table[i].arrival_timestamp < min_arrival) {
            min_arrival = page_table[i].arrival_timestamp;
            victim_page = i;
        }
    }
    
    if (victim_page == -1) return -1;
    
    int frame = page_table[victim_page].frame_number;
    
    page_table[victim_page].is_valid = 0;
    page_table[victim_page].frame_number = -1;
    page_table[victim_page].arrival_timestamp = -1;
    page_table[victim_page].last_access_timestamp = -1;
    page_table[victim_page].reference_count = -1;
    
    pte->is_valid = 1;
    pte->frame_number = frame;
    pte->arrival_timestamp = current_timestamp;
    pte->last_access_timestamp = current_timestamp;
    pte->reference_count = 1;
    
    return frame;
}

// FIFO - Count Page Faults
int count_page_faults_fifo(struct PTE page_table[TABLEMAX], int table_cnt,
                          int reference_string[REFERENCEMAX], int reference_cnt,
                          int frame_pool[POOLMAX], int frame_cnt) {
    
    struct PTE working_pt[TABLEMAX];
    int working_pool[POOLMAX];
    int working_frame_cnt = frame_cnt;
    
    // Copy initial state
    for (int i = 0; i < table_cnt; i++) {
        working_pt[i] = page_table[i];
    }
    for (int i = 0; i < frame_cnt; i++) {
        working_pool[i] = frame_pool[i];
    }
    
    int page_faults = 0;
    int current_timestamp = 1;
    
    for (int i = 0; i < reference_cnt; i++) {
        int page_number = reference_string[i];
        
        // Check if page is already in memory
        if (!working_pt[page_number].is_valid) {
            page_faults++;
            
            if (working_frame_cnt > 0) {
                // Use a free frame
                int frame = working_pool[0];
                for (int j = 0; j < working_frame_cnt - 1; j++) {
                    working_pool[j] = working_pool[j + 1];
                }
                working_frame_cnt--;
                
                working_pt[page_number].is_valid = 1;
                working_pt[page_number].frame_number = frame;
                working_pt[page_number].arrival_timestamp = current_timestamp;
                working_pt[page_number].last_access_timestamp = current_timestamp;
                working_pt[page_number].reference_count = 1;
            } else {
                // Find victim for FIFO - smallest arrival timestamp
                int victim_page = -1;
                int min_arrival = INT_MAX;
                
                for (int j = 0; j < table_cnt; j++) {
                    if (working_pt[j].is_valid && working_pt[j].arrival_timestamp < min_arrival) {
                        min_arrival = working_pt[j].arrival_timestamp;
                        victim_page = j;
                    }
                }
                
                if (victim_page != -1) {
                    int frame = working_pt[victim_page].frame_number;
                    
                    // Invalidate victim
                    working_pt[victim_page].is_valid = 0;
                    working_pt[victim_page].frame_number = -1;
                    working_pt[victim_page].arrival_timestamp = -1;
                    working_pt[victim_page].last_access_timestamp = -1;
                    working_pt[victim_page].reference_count = -1;
                    
                    // Allocate to new page
                    working_pt[page_number].is_valid = 1;
                    working_pt[page_number].frame_number = frame;
                    working_pt[page_number].arrival_timestamp = current_timestamp;
                    working_pt[page_number].last_access_timestamp = current_timestamp;
                    working_pt[page_number].reference_count = 1;
                }
            }
        } else {
            // Page hit - update access time
            working_pt[page_number].last_access_timestamp = current_timestamp;
            working_pt[page_number].reference_count++;
        }
        
        current_timestamp++;
    }
    
    return page_faults;
}

// LRU - Process Page Access
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
        for (int i = 0; i < *frame_cnt - 1; i++) {
            frame_pool[i] = frame_pool[i + 1];
        }
        (*frame_cnt)--;
        
        pte->is_valid = 1;
        pte->frame_number = frame;
        pte->arrival_timestamp = current_timestamp;
        pte->last_access_timestamp = current_timestamp;
        pte->reference_count = 1;
        
        return frame;
    }
    
    int victim_page = -1;
    int min_last_access = INT_MAX;
    
    for (int i = 0; i < *table_cnt; i++) {
        if (page_table[i].is_valid && page_table[i].last_access_timestamp < min_last_access) {
            min_last_access = page_table[i].last_access_timestamp;
            victim_page = i;
        }
    }
    
    if (victim_page == -1) return -1;
    
    int frame = page_table[victim_page].frame_number;
    
    page_table[victim_page].is_valid = 0;
    page_table[victim_page].frame_number = -1;
    page_table[victim_page].arrival_timestamp = -1;
    page_table[victim_page].last_access_timestamp = -1;
    page_table[victim_page].reference_count = -1;
    
    pte->is_valid = 1;
    pte->frame_number = frame;
    pte->arrival_timestamp = current_timestamp;
    pte->last_access_timestamp = current_timestamp;
    pte->reference_count = 1;
    
    return frame;
}

// LRU - Count Page Faults
int count_page_faults_lru(struct PTE page_table[TABLEMAX], int table_cnt,
                         int reference_string[REFERENCEMAX], int reference_cnt,
                         int frame_pool[POOLMAX], int frame_cnt) {
    
    struct PTE working_pt[TABLEMAX];
    int working_pool[POOLMAX];
    int working_frame_cnt = frame_cnt;
    
    // Copy initial state
    for (int i = 0; i < table_cnt; i++) {
        working_pt[i] = page_table[i];
    }
    for (int i = 0; i < frame_cnt; i++) {
        working_pool[i] = frame_pool[i];
    }
    
    int page_faults = 0;
    int current_timestamp = 1;
    
    for (int i = 0; i < reference_cnt; i++) {
        int page_number = reference_string[i];
        
        // Check if page is already in memory
        if (!working_pt[page_number].is_valid) {
            page_faults++;
            
            if (working_frame_cnt > 0) {
                // Use a free frame
                int frame = working_pool[0];
                for (int j = 0; j < working_frame_cnt - 1; j++) {
                    working_pool[j] = working_pool[j + 1];
                }
                working_frame_cnt--;
                
                working_pt[page_number].is_valid = 1;
                working_pt[page_number].frame_number = frame;
                working_pt[page_number].arrival_timestamp = current_timestamp;
                working_pt[page_number].last_access_timestamp = current_timestamp;
                working_pt[page_number].reference_count = 1;
            } else {
                // Find victim for LRU - smallest last access timestamp
                int victim_page = -1;
                int min_last_access = INT_MAX;
                
                for (int j = 0; j < table_cnt; j++) {
                    if (working_pt[j].is_valid && working_pt[j].last_access_timestamp < min_last_access) {
                        min_last_access = working_pt[j].last_access_timestamp;
                        victim_page = j;
                    }
                }
                
                if (victim_page != -1) {
                    int frame = working_pt[victim_page].frame_number;
                    
                    // Invalidate victim
                    working_pt[victim_page].is_valid = 0;
                    working_pt[victim_page].frame_number = -1;
                    working_pt[victim_page].arrival_timestamp = -1;
                    working_pt[victim_page].last_access_timestamp = -1;
                    working_pt[victim_page].reference_count = -1;
                    
                    // Allocate to new page
                    working_pt[page_number].is_valid = 1;
                    working_pt[page_number].frame_number = frame;
                    working_pt[page_number].arrival_timestamp = current_timestamp;
                    working_pt[page_number].last_access_timestamp = current_timestamp;
                    working_pt[page_number].reference_count = 1;
                }
            }
        } else {
            // Page hit - update access time
            working_pt[page_number].last_access_timestamp = current_timestamp;
            working_pt[page_number].reference_count++;
        }
        
        current_timestamp++;
    }
    
    return page_faults;
}

// LFU - Process Page Access
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
        for (int i = 0; i < *frame_cnt - 1; i++) {
            frame_pool[i] = frame_pool[i + 1];
        }
        (*frame_cnt)--;
        
        pte->is_valid = 1;
        pte->frame_number = frame;
        pte->arrival_timestamp = current_timestamp;
        pte->last_access_timestamp = current_timestamp;
        pte->reference_count = 1;
        
        return frame;
    }
    
    int victim_page = -1;
    int min_reference_count = INT_MAX;
    int min_arrival = INT_MAX;
    
    for (int i = 0; i < *table_cnt; i++) {
        if (page_table[i].is_valid) {
            if (page_table[i].reference_count < min_reference_count) {
                min_reference_count = page_table[i].reference_count;
                min_arrival = page_table[i].arrival_timestamp;
                victim_page = i;
            } else if (page_table[i].reference_count == min_reference_count) {
                if (page_table[i].arrival_timestamp < min_arrival) {
                    min_arrival = page_table[i].arrival_timestamp;
                    victim_page = i;
                }
            }
        }
    }
    
    if (victim_page == -1) return -1;
    
    int frame = page_table[victim_page].frame_number;
    
    page_table[victim_page].is_valid = 0;
    page_table[victim_page].frame_number = -1;
    page_table[victim_page].arrival_timestamp = -1;
    page_table[victim_page].last_access_timestamp = -1;
    page_table[victim_page].reference_count = -1;
    
    pte->is_valid = 1;
    pte->frame_number = frame;
    pte->arrival_timestamp = current_timestamp;
    pte->last_access_timestamp = current_timestamp;
    pte->reference_count = 1;
    
    return frame;
}

// LFU - Count Page Faults
int count_page_faults_lfu(struct PTE page_table[TABLEMAX], int table_cnt,
                         int reference_string[REFERENCEMAX], int reference_cnt,
                         int frame_pool[POOLMAX], int frame_cnt) {
    
    struct PTE working_pt[TABLEMAX];
    int working_pool[POOLMAX];
    int working_frame_cnt = frame_cnt;
    
    // Copy initial state
    for (int i = 0; i < table_cnt; i++) {
        working_pt[i] = page_table[i];
    }
    for (int i = 0; i < frame_cnt; i++) {
        working_pool[i] = frame_pool[i];
    }
    
    int page_faults = 0;
    int current_timestamp = 1;
    
    for (int i = 0; i < reference_cnt; i++) {
        int page_number = reference_string[i];
        
        // Check if page is already in memory
        if (!working_pt[page_number].is_valid) {
            page_faults++;
            
            if (working_frame_cnt > 0) {
                // Use a free frame
                int frame = working_pool[0];
                for (int j = 0; j < working_frame_cnt - 1; j++) {
                    working_pool[j] = working_pool[j + 1];
                }
                working_frame_cnt--;
                
                working_pt[page_number].is_valid = 1;
                working_pt[page_number].frame_number = frame;
                working_pt[page_number].arrival_timestamp = current_timestamp;
                working_pt[page_number].last_access_timestamp = current_timestamp;
                working_pt[page_number].reference_count = 1;
            } else {
                // Find victim for LFU
                int victim_page = -1;
                int min_reference_count = INT_MAX;
                int min_arrival = INT_MAX;
                
                for (int j = 0; j < table_cnt; j++) {
                    if (working_pt[j].is_valid) {
                        if (working_pt[j].reference_count < min_reference_count) {
                            min_reference_count = working_pt[j].reference_count;
                            min_arrival = working_pt[j].arrival_timestamp;
                            victim_page = j;
                        } else if (working_pt[j].reference_count == min_reference_count) {
                            if (working_pt[j].arrival_timestamp < min_arrival) {
                                min_arrival = working_pt[j].arrival_timestamp;
                                victim_page = j;
                            }
                        }
                    }
                }
                
                if (victim_page != -1) {
                    int frame = working_pt[victim_page].frame_number;
                    
                    // Invalidate victim
                    working_pt[victim_page].is_valid = 0;
                    working_pt[victim_page].frame_number = -1;
                    working_pt[victim_page].arrival_timestamp = -1;
                    working_pt[victim_page].last_access_timestamp = -1;
                    working_pt[victim_page].reference_count = -1;
                    
                    // Allocate to new page
                    working_pt[page_number].is_valid = 1;
                    working_pt[page_number].frame_number = frame;
                    working_pt[page_number].arrival_timestamp = current_timestamp;
                    working_pt[page_number].last_access_timestamp = current_timestamp;
                    working_pt[page_number].reference_count = 1;
                }
            }
        } else {
            // Page hit - update access time and count
            working_pt[page_number].last_access_timestamp = current_timestamp;
            working_pt[page_number].reference_count++;
        }
        
        current_timestamp++;
    }
    
    return page_faults;
}
