
/*
 * virtual.c
 * Simple virtual memory page replacement simulator supporting FIFO, LRU, and LFU.
 *
 * Usage:
 *   ./virtual ALGO NUM_FRAMES REF1 REF2 REF3 ...
 *   ALGO: FIFO | LRU | LFU   (case-insensitive)
 *   NUM_FRAMES: positive integer
 *   If no references are provided on the command line, the program reads a line
 *   of space-separated integers from stdin.
 *
 * Behavior & tie-breaking:
 *  - FIFO: replace the page that has been in frames the longest (queue order).
 *  - LRU: replace the least recently used page (tracked by last-used timestamp).
 *  - LFU: replace the page with the smallest access frequency; ties broken by LRU.
 *
 * Output:
 *  - A per-reference log showing the reference, frame contents after the operation,
 *    and whether it was a HIT or FAULT.
 *  - A summary with counts: references, hits, faults, and fault rate.
 *
 * This implementation is intended for educational/lab use and is commented for clarity.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_REFS 10000

typedef enum { ALG_FIFO, ALG_LRU, ALG_LFU } algo_t;

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s ALGO NUM_FRAMES [ref1 ref2 ...]\n", prog);
    fprintf(stderr, "  ALGO: FIFO | LRU | LFU\n");
    fprintf(stderr, "  If no refs provided, program reads a line of space-separated ints from stdin.\n");
    exit(1);
}

static algo_t parse_algo(const char *s) {
    char t[8]; int i=0;
    while (s[i] && i<7) { t[i]=toupper((unsigned char)s[i]); i++; }
    t[i]=0;
    if (strcmp(t,"FIFO")==0) return ALG_FIFO;
    if (strcmp(t,"LRU")==0) return ALG_LRU;
    if (strcmp(t,"LFU")==0) return ALG_LFU;
    fprintf(stderr,"Unknown algorithm: %s\n", s);
    usage("virtual");
    return ALG_FIFO; // unreachable
}

/* Utility: find page in frames, return index or -1 */
static int find_in_frames(int *frames, int frames_count, int page) {
    for (int i=0;i<frames_count;i++) if (frames[i]==page) return i;
    return -1;
}

/* Print frame contents */
static void print_frames(int *frames, int frames_count) {
    printf("[");
    for (int i=0;i<frames_count;i++) {
        if (frames[i] == -1) printf(" .");
        else printf(" %d", frames[i]);
        if (i<frames_count-1) printf(" ");
    }
    printf(" ]");
}

/* FIFO algorithm */
static void simulate_fifo(int *refs, int nrefs, int frames_count) {
    int *frames = calloc(frames_count, sizeof(int));
    int *order = calloc(frames_count, sizeof(int)); /* queue positions (store insertion sequence id) */
    for (int i=0;i<frames_count;i++) frames[i] = -1, order[i]= -1;
    int next_seq = 0;
    int faults=0, hits=0;
    printf("=== FIFO (frames=%d) ===\n", frames_count);
    for (int t=0;t<nrefs;t++) {
        int r = refs[t];
        int idx = find_in_frames(frames, frames_count, r);
        if (idx != -1) {
            hits++;
            printf("%3d: ", r);
            print_frames(frames, frames_count);
            printf("  HIT\n");
        } else {
            faults++;
            /* find an empty frame first */
            int empty = -1;
            for (int i=0;i<frames_count;i++) if (frames[i]==-1) { empty=i; break; }
            if (empty != -1) {
                frames[empty] = r;
                order[empty] = next_seq++;
            } else {
                /* find frame with smallest order value */
                int min_idx=0;
                for (int i=1;i<frames_count;i++) if (order[i] < order[min_idx]) min_idx = i;
                frames[min_idx] = r;
                order[min_idx] = next_seq++;
            }
            printf("%3d: ", r);
            print_frames(frames, frames_count);
            printf("  FAULT\n");
        }
    }
    printf("Summary: refs=%d hits=%d faults=%d fault_rate=%.2f%%\n\n",
           nrefs, hits, faults, (double)faults/nrefs*100.0);
    free(frames); free(order);
}

/* LRU algorithm */
static void simulate_lru(int *refs, int nrefs, int frames_count) {
    int *frames = calloc(frames_count, sizeof(int));
    int *last_used = calloc(frames_count, sizeof(int));
    for (int i=0;i<frames_count;i++) frames[i] = -1, last_used[i] = -1;
    int time = 0;
    int faults=0, hits=0;
    printf("=== LRU (frames=%d) ===\n", frames_count);
    for (int t=0;t<nrefs;t++) {
        int r = refs[t];
        int idx = find_in_frames(frames, frames_count, r);
        if (idx != -1) {
            hits++;
            last_used[idx] = time++;
            printf("%3d: ", r);
            print_frames(frames, frames_count);
            printf("  HIT\n");
        } else {
            faults++;
            /* find empty frame first */
            int empty = -1;
            for (int i=0;i<frames_count;i++) if (frames[i]==-1) { empty=i; break; }
            if (empty != -1) {
                frames[empty] = r;
                last_used[empty] = time++;
            } else {
                /* replace the frame with smallest last_used (least recently used) */
                int lru_idx = 0;
                for (int i=1;i<frames_count;i++) if (last_used[i] < last_used[lru_idx]) lru_idx = i;
                frames[lru_idx] = r;
                last_used[lru_idx] = time++;
            }
            printf("%3d: ", r);
            print_frames(frames, frames_count);
            printf("  FAULT\n");
        }
    }
    printf("Summary: refs=%d hits=%d faults=%d fault_rate=%.2f%%\n\n",
           nrefs, hits, faults, (double)faults/nrefs*100.0);
    free(frames); free(last_used);
}

/* LFU algorithm with LRU tie-breaker */
static void simulate_lfu(int *refs, int nrefs, int frames_count) {
    int *frames = calloc(frames_count, sizeof(int));
    int *freq = calloc(frames_count, sizeof(int));
    int *last_used = calloc(frames_count, sizeof(int));
    for (int i=0;i<frames_count;i++) frames[i] = -1, freq[i]=0, last_used[i]=-1;
    int time = 0;
    int faults=0, hits=0;
    printf("=== LFU (frames=%d) ===\n", frames_count);
    for (int t=0;t<nrefs;t++) {
        int r = refs[t];
        int idx = find_in_frames(frames, frames_count, r);
        if (idx != -1) {
            hits++;
            freq[idx]++;
            last_used[idx] = time++;
            printf("%3d: ", r);
            print_frames(frames, frames_count);
            printf("  HIT (freq=%d)\n", freq[idx]);
        } else {
            faults++;
            /* find empty frame first */
            int empty = -1;
            for (int i=0;i<frames_count;i++) if (frames[i]==-1) { empty=i; break; }
            if (empty != -1) {
                frames[empty] = r;
                freq[empty] = 1;
                last_used[empty] = time++;
            } else {
                /* find min frequency; break ties by smallest last_used (LRU among tied) */
                int victim = 0;
                for (int i=1;i<frames_count;i++) {
                    if (freq[i] < freq[victim]) victim = i;
                    else if (freq[i] == freq[victim] && last_used[i] < last_used[victim]) victim = i;
                }
                frames[victim] = r;
                freq[victim] = 1;
                last_used[victim] = time++;
            }
            printf("%3d: ", r);
            print_frames(frames, frames_count);
            printf("  FAULT\n");
        }
    }
    printf("Summary: refs=%d hits=%d faults=%d fault_rate=%.2f%%\n\n",
           nrefs, hits, faults, (double)faults/nrefs*100.0);
    free(frames); free(freq); free(last_used);
}

int main(int argc, char **argv) {
    if (argc < 3) usage(argv[0]);
    algo_t alg = parse_algo(argv[1]);
    int frames_count = atoi(argv[2]);
    if (frames_count <= 0) { fprintf(stderr,"NUM_FRAMES must be > 0\n"); return 1; }

    int refs[MAX_REFS];
    int nrefs = 0;

    /* Collect references from argv if provided, else read a line from stdin */
    if (argc > 3) {
        for (int i=3;i<argc;i++) {
            if (nrefs >= MAX_REFS) break;
            refs[nrefs++] = atoi(argv[i]);
        }
    } else {
        /* read a whole line */
        char line[10000];
        if (!fgets(line, sizeof(line), stdin)) { fprintf(stderr,"No refs provided\n"); return 1; }
        char *p = line;
        while (*p) {
            while (*p && isspace((unsigned char)*p)) p++;
            if (!*p) break;
            int val;
            int consumed;
            if (sscanf(p, "%d%n", &val, &consumed) == 1) {
                refs[nrefs++] = val;
                p += consumed;
            } else break;
        }
    }

    if (nrefs == 0) { fprintf(stderr,"No references parsed\n"); return 1; }

    switch (alg) {
        case ALG_FIFO: simulate_fifo(refs, nrefs, frames_count); break;
        case ALG_LRU: simulate_lru(refs, nrefs, frames_count); break;
        case ALG_LFU: simulate_lfu(refs, nrefs, frames_count); break;
    }
    return 0;
}
