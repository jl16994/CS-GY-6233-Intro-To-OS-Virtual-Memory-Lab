#include <stdio.h>
#include <limits.h>
#include "oslabs.h"

int process_page_access_fifo(struct PTE page_table[TABLEMAX], int *table_cnt, int page_number, int frame_pool[POOLMAX], int *frame_cnt, int current_timestamp) {
    // Check if page is already in memory
    if (page_table[page_number].is_valid) {
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count++;
        return page_table[page_number].frame_number;
    }
    
    // Page fault - check if free frames available
    if (*frame_cnt > 0) {
        // Use a free frame from pool
        int frame = frame_pool[0];
        
        // Remove frame from pool
        for (int i = 0; i < *frame_cnt - 1; i++) {
            frame_pool[i] = frame_pool[i + 1];
        }
        (*frame_cnt)--;
        
        // Update page table entry
        page_table[page_number].is_valid = 1;
        page_table[page_number].frame_number = frame;
        page_table[page_number].arrival_timestamp = current_timestamp;
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count = 1;
        
        return frame;
    }
    
    // No free frames - need to replace a page using FIFO
    int oldest_timestamp = INT_MAX;
    int page_to_replace = -1;
    
    // Find page with smallest arrival timestamp
    for (int i = 0; i < *table_cnt; i++) {
        if (page_table[i].is_valid && page_table[i].arrival_timestamp < oldest_timestamp) {
            oldest_timestamp = page_table[i].arrival_timestamp;
            page_to_replace = i;
        }
    }
    
    if (page_to_replace != -1) {
        int frame = page_table[page_to_replace].frame_number;
        
        // Evict the old page
        page_table[page_to_replace].is_valid = 0;
        page_table[page_to_replace].frame_number = -1;
        page_table[page_to_replace].arrival_timestamp = -1;
        page_table[page_to_replace].last_access_timestamp = -1;
        page_table[page_to_replace].reference_count = -1;
        
        // Load new page
        page_table[page_number].is_valid = 1;
        page_table[page_number].frame_number = frame;
        page_table[page_number].arrival_timestamp = current_timestamp;
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count = 1;
        
        return frame;
    }
    
    return -1; // Should not reach here
}

int count_page_faults_fifo(struct PTE page_table[TABLEMAX], int table_cnt, int reference_string[REFERENCEMAX], int reference_cnt, int frame_pool[POOLMAX], int frame_cnt) {
    int faults = 0;
    int current_timestamp = 1;
    
    // Create a copy of frame pool and page table for simulation
    int local_frame_pool[POOLMAX];
    int local_frame_cnt = frame_cnt;
    struct PTE local_page_table[TABLEMAX];
    
    for (int i = 0; i < table_cnt; i++) {
        local_page_table[i] = page_table[i];
    }
    for (int i = 0; i < frame_cnt; i++) {
        local_frame_pool[i] = frame_pool[i];
    }
    
    for (int i = 0; i < reference_cnt; i++) {
        int page_number = reference_string[i];
        
        if (!local_page_table[page_number].is_valid) {
            faults++;
            
            if (local_frame_cnt > 0) {
                // Use free frame
                int frame = local_frame_pool[0];
                for (int j = 0; j < local_frame_cnt - 1; j++) {
                    local_frame_pool[j] = local_frame_pool[j + 1];
                }
                local_frame_cnt--;
                
                local_page_table[page_number].is_valid = 1;
                local_page_table[page_number].frame_number = frame;
                local_page_table[page_number].arrival_timestamp = current_timestamp;
                local_page_table[page_number].last_access_timestamp = current_timestamp;
                local_page_table[page_number].reference_count = 1;
            } else {
                // Replace using FIFO
                int oldest_timestamp = INT_MAX;
                int page_to_replace = -1;
                
                for (int j = 0; j < table_cnt; j++) {
                    if (local_page_table[j].is_valid && local_page_table[j].arrival_timestamp < oldest_timestamp) {
                        oldest_timestamp = local_page_table[j].arrival_timestamp;
                        page_to_replace = j;
                    }
                }
                
                if (page_to_replace != -1) {
                    int frame = local_page_table[page_to_replace].frame_number;
                    
                    local_page_table[page_to_replace].is_valid = 0;
                    local_page_table[page_to_replace].frame_number = -1;
                    local_page_table[page_to_replace].arrival_timestamp = -1;
                    local_page_table[page_to_replace].last_access_timestamp = -1;
                    local_page_table[page_to_replace].reference_count = -1;
                    
                    local_page_table[page_number].is_valid = 1;
                    local_page_table[page_number].frame_number = frame;
                    local_page_table[page_number].arrival_timestamp = current_timestamp;
                    local_page_table[page_number].last_access_timestamp = current_timestamp;
                    local_page_table[page_number].reference_count = 1;
                }
            }
        } else {
            // Page is in memory, update access info
            local_page_table[page_number].last_access_timestamp = current_timestamp;
            local_page_table[page_number].reference_count++;
        }
        
        current_timestamp++;
    }
    
    return faults;
}

int process_page_access_lru(struct PTE page_table[TABLEMAX], int *table_cnt, int page_number, int frame_pool[POOLMAX], int *frame_cnt, int current_timestamp) {
    // Check if page is already in memory
    if (page_table[page_number].is_valid) {
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count++;
        return page_table[page_number].frame_number;
    }
    
    // Page fault - check if free frames available
    if (*frame_cnt > 0) {
        // Use a free frame from pool
        int frame = frame_pool[0];
        
        // Remove frame from pool
        for (int i = 0; i < *frame_cnt - 1; i++) {
            frame_pool[i] = frame_pool[i + 1];
        }
        (*frame_cnt)--;
        
        // Update page table entry
        page_table[page_number].is_valid = 1;
        page_table[page_number].frame_number = frame;
        page_table[page_number].arrival_timestamp = current_timestamp;
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count = 1;
        
        return frame;
    }
    
    // No free frames - need to replace a page using LRU
    int oldest_access = INT_MAX;
    int page_to_replace = -1;
    
    // Find page with smallest last access timestamp
    for (int i = 0; i < *table_cnt; i++) {
        if (page_table[i].is_valid && page_table[i].last_access_timestamp < oldest_access) {
            oldest_access = page_table[i].last_access_timestamp;
            page_to_replace = i;
        }
    }
    
    if (page_to_replace != -1) {
        int frame = page_table[page_to_replace].frame_number;
        
        // Evict the old page
        page_table[page_to_replace].is_valid = 0;
        page_table[page_to_replace].frame_number = -1;
        page_table[page_to_replace].arrival_timestamp = -1;
        page_table[page_to_replace].last_access_timestamp = -1;
        page_table[page_to_replace].reference_count = -1;
        
        // Load new page
        page_table[page_number].is_valid = 1;
        page_table[page_number].frame_number = frame;
        page_table[page_number].arrival_timestamp = current_timestamp;
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count = 1;
        
        return frame;
    }
    
    return -1; // Should not reach here
}

int count_page_faults_lru(struct PTE page_table[TABLEMAX], int table_cnt, int reference_string[REFERENCEMAX], int reference_cnt, int frame_pool[POOLMAX], int frame_cnt) {
    int faults = 0;
    int current_timestamp = 1;
    
    // Create a copy of frame pool and page table for simulation
    int local_frame_pool[POOLMAX];
    int local_frame_cnt = frame_cnt;
    struct PTE local_page_table[TABLEMAX];
    
    for (int i = 0; i < table_cnt; i++) {
        local_page_table[i] = page_table[i];
    }
    for (int i = 0; i < frame_cnt; i++) {
        local_frame_pool[i] = frame_pool[i];
    }
    
    for (int i = 0; i < reference_cnt; i++) {
        int page_number = reference_string[i];
        
        if (!local_page_table[page_number].is_valid) {
            faults++;
            
            if (local_frame_cnt > 0) {
                // Use free frame
                int frame = local_frame_pool[0];
                for (int j = 0; j < local_frame_cnt - 1; j++) {
                    local_frame_pool[j] = local_frame_pool[j + 1];
                }
                local_frame_cnt--;
                
                local_page_table[page_number].is_valid = 1;
                local_page_table[page_number].frame_number = frame;
                local_page_table[page_number].arrival_timestamp = current_timestamp;
                local_page_table[page_number].last_access_timestamp = current_timestamp;
                local_page_table[page_number].reference_count = 1;
            } else {
                // Replace using LRU
                int oldest_access = INT_MAX;
                int page_to_replace = -1;
                
                for (int j = 0; j < table_cnt; j++) {
                    if (local_page_table[j].is_valid && local_page_table[j].last_access_timestamp < oldest_access) {
                        oldest_access = local_page_table[j].last_access_timestamp;
                        page_to_replace = j;
                    }
                }
                
                if (page_to_replace != -1) {
                    int frame = local_page_table[page_to_replace].frame_number;
                    
                    local_page_table[page_to_replace].is_valid = 0;
                    local_page_table[page_to_replace].frame_number = -1;
                    local_page_table[page_to_replace].arrival_timestamp = -1;
                    local_page_table[page_to_replace].last_access_timestamp = -1;
                    local_page_table[page_to_replace].reference_count = -1;
                    
                    local_page_table[page_number].is_valid = 1;
                    local_page_table[page_number].frame_number = frame;
                    local_page_table[page_number].arrival_timestamp = current_timestamp;
                    local_page_table[page_number].last_access_timestamp = current_timestamp;
                    local_page_table[page_number].reference_count = 1;
                }
            }
        } else {
            // Page is in memory, update access info
            local_page_table[page_number].last_access_timestamp = current_timestamp;
            local_page_table[page_number].reference_count++;
        }
        
        current_timestamp++;
    }
    
    return faults;
}

int process_page_access_lfu(struct PTE page_table[TABLEMAX], int *table_cnt, int page_number, int frame_pool[POOLMAX], int *frame_cnt, int current_timestamp) {
    // Check if page is already in memory
    if (page_table[page_number].is_valid) {
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count++;
        return page_table[page_number].frame_number;
    }
    
    // Page fault - check if free frames available
    if (*frame_cnt > 0) {
        // Use a free frame from pool
        int frame = frame_pool[0];
        
        // Remove frame from pool
        for (int i = 0; i < *frame_cnt - 1; i++) {
            frame_pool[i] = frame_pool[i + 1];
        }
        (*frame_cnt)--;
        
        // Update page table entry
        page_table[page_number].is_valid = 1;
        page_table[page_number].frame_number = frame;
        page_table[page_number].arrival_timestamp = current_timestamp;
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count = 1;
        
        return frame;
    }
    
    // No free frames - need to replace a page using LFU
    int min_ref_count = INT_MAX;
    int oldest_timestamp = INT_MAX;
    int page_to_replace = -1;
    
    // Find page with smallest reference count, and if tie, smallest arrival timestamp
    for (int i = 0; i < *table_cnt; i++) {
        if (page_table[i].is_valid) {
            if (page_table[i].reference_count < min_ref_count) {
                min_ref_count = page_table[i].reference_count;
                oldest_timestamp = page_table[i].arrival_timestamp;
                page_to_replace = i;
            } else if (page_table[i].reference_count == min_ref_count && 
                       page_table[i].arrival_timestamp < oldest_timestamp) {
                oldest_timestamp = page_table[i].arrival_timestamp;
                page_to_replace = i;
            }
        }
    }
    
    if (page_to_replace != -1) {
        int frame = page_table[page_to_replace].frame_number;
        
        // Evict the old page
        page_table[page_to_replace].is_valid = 0;
        page_table[page_to_replace].frame_number = -1;
        page_table[page_to_replace].arrival_timestamp = -1;
        page_table[page_to_replace].last_access_timestamp = -1;
        page_table[page_to_replace].reference_count = -1;
        
        // Load new page
        page_table[page_number].is_valid = 1;
        page_table[page_number].frame_number = frame;
        page_table[page_number].arrival_timestamp = current_timestamp;
        page_table[page_number].last_access_timestamp = current_timestamp;
        page_table[page_number].reference_count = 1;
        
        return frame;
    }
    
    return -1; // Should not reach here
}

int count_page_faults_lfu(struct PTE page_table[TABLEMAX], int table_cnt, int reference_string[REFERENCEMAX], int reference_cnt, int frame_pool[POOLMAX], int frame_cnt) {
    int faults = 0;
    int current_timestamp = 1;
    
    // Create a copy of frame pool and page table for simulation
    int local_frame_pool[POOLMAX];
    int local_frame_cnt = frame_cnt;
    struct PTE local_page_table[TABLEMAX];
    
    for (int i = 0; i < table_cnt; i++) {
        local_page_table[i] = page_table[i];
    }
    for (int i = 0; i < frame_cnt; i++) {
        local_frame_pool[i] = frame_pool[i];
    }
    
    for (int i = 0; i < reference_cnt; i++) {
        int page_number = reference_string[i];
        
        if (!local_page_table[page_number].is_valid) {
            faults++;
            
            if (local_frame_cnt > 0) {
                // Use free frame
                int frame = local_frame_pool[0];
                for (int j = 0; j < local_frame_cnt - 1; j++) {
                    local_frame_pool[j] = local_frame_pool[j + 1];
                }
                local_frame_cnt--;
                
                local_page_table[page_number].is_valid = 1;
                local_page_table[page_number].frame_number = frame;
                local_page_table[page_number].arrival_timestamp = current_timestamp;
                local_page_table[page_number].last_access_timestamp = current_timestamp;
                local_page_table[page_number].reference_count = 1;
            } else {
                // Replace using LFU
                int min_ref_count = INT_MAX;
                int oldest_timestamp = INT_MAX;
                int page_to_replace = -1;
                
                for (int j = 0; j < table_cnt; j++) {
                    if (local_page_table[j].is_valid) {
                        if (local_page_table[j].reference_count < min_ref_count) {
                            min_ref_count = local_page_table[j].reference_count;
                            oldest_timestamp = local_page_table[j].arrival_timestamp;
                            page_to_replace = j;
                        } else if (local_page_table[j].reference_count == min_ref_count && 
                                   local_page_table[j].arrival_timestamp < oldest_timestamp) {
                            oldest_timestamp = local_page_table[j].arrival_timestamp;
                            page_to_replace = j;
                        }
                    }
                }
                
                if (page_to_replace != -1) {
                    int frame = local_page_table[page_to_replace].frame_number;
                    
                    local_page_table[page_to_replace].is_valid = 0;
                    local_page_table[page_to_replace].frame_number = -1;
                    local_page_table[page_to_replace].arrival_timestamp = -1;
                    local_page_table[page_to_replace].last_access_timestamp = -1;
                    local_page_table[page_to_replace].reference_count = -1;
                    
                    local_page_table[page_number].is_valid = 1;
                    local_page_table[page_number].frame_number = frame;
                    local_page_table[page_number].arrival_timestamp = current_timestamp;
                    local_page_table[page_number].last_access_timestamp = current_timestamp;
                    local_page_table[page_number].reference_count = 1;
                }
            }
        } else {
            // Page is in memory, update access info
            local_page_table[page_number].last_access_timestamp = current_timestamp;
            local_page_table[page_number].reference_count++;
        }
        
        current_timestamp++;
    }
    
    return faults;
}
