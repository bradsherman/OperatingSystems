/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

typedef struct Node Node;

struct disk * disk;
char * physmem;
int * frames;
struct page_table *pt;
char * algo;
Node * head;
int disk_reads, disk_writes, page_faults;

// Functions and datatypes used for FIFO

typedef struct Node {
    int data;
    Node * next;
} Node;

void push_queue(int data) {
    Node * new = malloc(sizeof(Node));
    new->data = data;
    new->next = head;
    head = new;
}

int pop_queue() {
    Node * curr = head;
    Node * prev = curr;
    while(curr->next != NULL) {
        prev = curr;
        curr = curr->next;
    }
    int data = curr->data;
    free(curr);
    prev->next = NULL;
    return data;
}

void destroy_queue() {
    Node * curr = head;
    Node * prev = curr;
    while(curr->next != NULL) {
        prev = curr;
        curr = curr->next;
        free(prev);
    }
    free(curr);
}

int get_random_page() {
    return rand() % page_table_get_nframes(pt);
}

int get_fifo_page() {
    return pop_queue();
}

int get_custom_page() {
    int r = 0;
    int max = page_table_get_nframes(pt) / 3;
    int tries = 0;
    int frame, bits;
    while(tries < max) {
        r = rand() % page_table_get_nframes(pt);
        int page = frames[r];
        page_table_get_entry(pt, page, &frame, &bits);
        if(!(bits & PROT_WRITE)) return r;
        tries++;
    }
    return r;
}

void page_fault_handler( struct page_table *pt, int page )
{
    int frame, bits;
    page_table_get_entry(pt, page, &frame, &bits);
    if(bits == 0) {
        // only increment on a real page fault
        page_faults++;
        // page not in memory, need to check open frames and bring it in
        int i;
        for(i = 0; i < page_table_get_nframes(pt); i++) {
            if(frames[i] == -1) {
                // empty frame, put entry here
                page_table_set_entry(pt, page, i, PROT_READ);
                disk_read(disk, page, &physmem[PAGE_SIZE*i]);
                disk_reads++;
                frames[i] = page;
                push_queue(i);
                return;
            }
        }

        // if we make it past the for loop, there are no empty addresses
        int r; // frame to be replaced
        if(!strcmp(algo, "rand")) {
            r = get_random_page();
        } else if(!strcmp(algo, "fifo")) {
            r = get_fifo_page();
        } else if(!strcmp(algo, "custom")) {
            r = get_custom_page();
        } else {
            fprintf(stderr,"unknown algorithm: %s\n",algo);
            exit(1);
        }
        int page_to_replace = frames[r]; // page to be reset
        int rFrame, rBits;
        page_table_get_entry(pt, page_to_replace, &rFrame, &rBits);
        if(rBits == (PROT_READ|PROT_WRITE)) {
            // must write back to disk if page modified
            disk_write(disk, page_to_replace, &physmem[r*PAGE_SIZE]);
            disk_writes++;
        }
        // read in new page, and update page table
        disk_read(disk, page, &physmem[r*PAGE_SIZE]);
        disk_reads++;
        page_table_set_entry(pt, page, r, PROT_READ);
        page_table_set_entry(pt, page_to_replace, 0, 0);
        push_queue(r);
        frames[r] = page;


    } else if(bits == PROT_READ) {
        // "fake" page fault due to no write access
        page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
    }
}

int main( int argc, char *argv[] )
{
    if(argc!=5) {
        printf("use: virtmem <npages> <nframes> <rand|fifo|lru|custom> <sort|scan|focus>\n");
        return 1;
    }

    int npages = atoi(argv[1]);
    int nframes = atoi(argv[2]);
    if(nframes > npages) {
        printf("It is not a valid configuration to have more frames than pages\n");
        return 1;
    }
    algo = argv[3];
    const char *program = argv[4];
    head = NULL;
    disk_reads = 0;
    disk_writes = 0;


    // setup frames array, initalize to 0 since they are all empty
    frames = malloc(sizeof(int)*nframes);
    int i;
    for(i = 0; i < nframes; i++)
        frames[i] = -1;

    disk = disk_open("myvirtualdisk",npages);
    if(!disk) {
        fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
        return 1;
    }

    pt = page_table_create( npages, nframes, page_fault_handler );

    if(!pt) {
        fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
        return 1;
    }

    srand(time(NULL));

    char *virtmem = page_table_get_virtmem(pt);

    physmem = page_table_get_physmem(pt);

    if(!strcmp(program,"sort")) {
        sort_program(virtmem,npages*PAGE_SIZE);

    } else if(!strcmp(program,"scan")) {
        scan_program(virtmem,npages*PAGE_SIZE);

    } else if(!strcmp(program,"focus")) {
        focus_program(virtmem,npages*PAGE_SIZE);

    } else {
        fprintf(stderr,"unknown program: %s\n",argv[4]);
        return 1;
    }

    page_table_delete(pt);
    disk_close(disk);

    // clean up
    free(frames);
    destroy_queue();

    // print stats
    printf("# Reads: %d\n", disk_reads);
    printf("# Writes: %d\n", disk_writes);
    printf("# Page Faults: %d\n", page_faults);

    return 0;
}
