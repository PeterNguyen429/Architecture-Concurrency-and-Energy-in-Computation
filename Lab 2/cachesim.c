#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cachesim.h"
#include <math.h>
counter_t accesses = 0, hits = 0, misses = 0, writebacks = 0;

//addition of global variables so cachesim_access can access from cachesim_init
int indexes; // computed by the cachesim_init, tells how many indexes are there in the cache.
addr_t offset_bits; // calculations done in cachesim_init
addr_t index_bits;
addr_t tag_bits;
int global_ways;
int read_op = 0;
int write_op = 0;
int read_misses = 0;
int write_misses = 0;

 // define a cache structure and its attributes
typedef struct cacheStruct{
  addr_t* tag;
  int* valid_bit; // valid bit that defines if the cache line is empty or full. valid = 1 full | valid = 0 empty
  int* dirty_bit; // dirty bit that defines if the line is first brought in to cache. valid = 1 & dirty = 0, clean | else dirty
  int* Lru;
}cacheStruct;

struct cacheStruct *cache; //creates a stucture name cache

void cachesim_init(int blocksize, int cachesize, int ways) {
  //calculation of the bits for each area of the address for the cache
  offset_bits = log2(blocksize); //calculates the number of offset bits and pushes them globally
  indexes = (cachesize/blocksize)/ways; // calculates the number of indexes in the cache
  index_bits = log2(indexes); // calculate the number of index bits based on the number of indexes
  tag_bits = 32 - offset_bits - index_bits; //assuming a 32 bit system, the leftover bits are the tag.
  global_ways = ways;
  //allocating memory for cache and every element inside cache
  cache = (cacheStruct*) malloc(indexes * sizeof(cacheStruct));

  for(int i = 0; i < indexes; i++){
    cache[i].tag = (addr_t*) malloc(indexes * sizeof(addr_t));
    cache[i].valid_bit = (int*) malloc(indexes * sizeof(int));
    cache[i].dirty_bit = (int*) malloc(indexes * sizeof(int));
    cache[i].Lru = (int*) malloc(indexes * sizeof(int));


  }


//  printf("%d\n", cachesize);
}



void cachesim_access(addr_t physical_addr, int write) {
  accesses += 1; // everytime this function is called, accesses is incremented by one.

  //calculating the offset number in bits
  addr_t offset_mask = (1 << offset_bits) - 1; // creates a mask of the bottom offset bits.
  addr_t offset = physical_addr & offset_mask; // calculate the offset number

  addr_t index_mask = ((1 << index_bits) -1 )<< offset_bits;
  addr_t index = physical_addr & index_mask;
  index = index >> offset_bits;

  addr_t tag = physical_addr >> (index_bits+offset_bits);
  if (write == 0){
    read_op += 1;
  }
  else{
    write_op += 1;
  }

  for(int i = 0; i < global_ways; i++ ){
    if (write == 0){ //read / instruction  only
      if(cache[index].valid_bit[i] == 0 ){ // empty
        misses += 1;
        read_misses += 1;
        cache[index].valid_bit[i] = 1; // update the valid bit
        cache[index].tag[i] = tag; // update the tag
        cache[index].Lru[i] = 0;//Update LRU
        cache[index].dirty_bit[i] = 0;
        for(int j = 0; j < global_ways; j++){
          if(i != j){
            cache[index].Lru[j] += 1;
          }
        } // increments the rest of the LRUS in the way
        break;
      }

      else if(cache[index].valid_bit[i] == 1 && cache[index].tag[i] == tag){
        hits += 1;
        cache[index].Lru[i] = 0;//Update LRU
        for(int j = 0; j < global_ways; j++){
          if(i != j){
            cache[index].Lru[j] += 1;
          }
        } // increments the rest of the LRUS in the way
        break;
      }

      else if(cache[index].valid_bit[i] == 1 && cache[index].tag[i] != tag && i == (global_ways -1)){
        misses += 1;
        read_misses+=1;
        //finding the LRU
        int LRU_index = 0;
        int LRU = cache[index].Lru[0]; //intializing the for loop to look for Lru
        for(int j = 0; j < global_ways; j++){
          if(cache[index].Lru[j] > LRU){
            LRU = cache[index].Lru[j];
            LRU_index = j;
          }
        }
        cache[index].tag[LRU_index] = tag; // update the tag
        cache[index].Lru[LRU_index] = 0;//Update LRU
        if(cache[index].dirty_bit[LRU_index] == 1){
          writebacks += 1;
        }
        cache[index].dirty_bit[LRU_index] = 0; //changes the dirty bit
        for(int j = 0; j < global_ways; j++){
          if(LRU_index != j){
            cache[index].Lru[j] += 1;
          }
        } // increments the rest of the LRUS in the way
        break;
      }




    }// end of write = 0

//
    else if (write == 1){ // write only
      if(cache[index].valid_bit[i] == 0){ // empty
        misses += 1;
        write_misses+=1;
        cache[index].valid_bit[i] = 1; // update the valid bit
        cache[index].tag[i] = tag; // update the tag
        cache[index].dirty_bit[i] = 1; // dirty since its been written to mem
        cache[index].Lru[i] = 0;//Update LRU
        for(int j = 0; j < global_ways; j++){
          if(i != j){
            cache[index].Lru[j] += 1;
          }
        } // increments the rest of the LRUS in the way
        break;
      }

      else if(cache[index].valid_bit[i] == 1 && cache[index].tag[i] == tag){
        hits += 1;
        cache[index].dirty_bit[i] = 1; // always one now
        cache[index].Lru[i] = 0;//Update LRU
        for(int j = 0; j < global_ways; j++){
          if(i != j){
            cache[index].Lru[j] += 1;
          }
        } // increments the rest of the LRUS in the way
        break;
      }

      else if(cache[index].valid_bit[i] == 1 && cache[index].tag[i] != tag && i == (global_ways -1)){
        misses += 1;
        write_misses += 1;


        //finding the Lru
        int LRU_index = 0;
        int LRU = cache[index].Lru[0]; //intializing the for loop to look for Lru
        for(int j = 0; j < global_ways; j++){
          if(cache[index].Lru[j] > LRU){
            LRU = cache[index].Lru[j];
            LRU_index = j;
          }
        }

        cache[index].tag[LRU_index] = tag; // update the tag
        cache[index].Lru[LRU_index] = 0;//Update LRU
        if(cache[index].dirty_bit[LRU_index] == 1){
          writebacks += 1;
        }
        cache[index].dirty_bit[LRU_index] = 1; // always one now
        for(int j = 0; j < global_ways; j++){
          if(LRU_index != j){
            cache[index].Lru[j] += 1;
          }
        } // increments the rest of the LRUS in the way
        break;
      }

    }


  }

}

void cachesim_print_stats() {
  printf("%llu, %llu, %llu, %llu\n", accesses, hits, misses, writebacks);
  // printf("%d, %d\n",read_op, write_op);
  // printf("%d, %d\n",read_misses, write_misses);

}
