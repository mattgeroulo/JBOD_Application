#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;
int created =0;
static int inserts =0;




int cache_create(int num_entries) {
  if(created==1){
    return -1;
  }
  if (num_entries< 2 || num_entries>4096){
    return -1;
  }
  cache = malloc(num_entries*sizeof(cache_entry_t));
  cache_size = num_entries;
  created =1;
  clock=0;
  inserts=0;
  return 1;
}

int cache_destroy(void) {
  if (created ==0){
    return -1;
  }
  
  free(cache);
  cache = NULL;
  cache_size =0;
  created =0;
  
  return 1;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
  if (!created){
    return -1;
  }
  if (cache ==NULL){
    return -1;
  }
  if (inserts ==0){
    return -1;
  }


  int found=0;
  num_queries+=1;
  for (int i=0; i<cache_size; i++){

    if (cache[i].valid && cache[i].disk_num==disk_num && cache[i].block_num == block_num){
      memcpy(buf,cache[i].block,JBOD_BLOCK_SIZE);
      cache[i].access_time = clock;
      clock+=1;
      found =1;
      num_hits+=1;
    }

    }
  
  if (found==1){
    return 1;
  }
  else{
    return -1;
  }

}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {
for (int i=0 ; i<cache_size; i++){
      if (cache[i].disk_num == disk_num && cache[i].block_num == block_num && cache[i].valid){
          memcpy(cache[i].block,buf,JBOD_BLOCK_SIZE);
          cache[i].access_time = clock;
          clock+=1;
          inserts+=1;
      }
}

}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  if(buf ==NULL){
    return -1;
  }
  if (cache ==NULL || buf ==NULL || cache_size <1){
    return -1;
  }
  if (disk_num >16 ||  disk_num <0 || block_num>256 || block_num <0){
    return -1;
  }
  if( inserts>0){
  for (int i=0; i<=cache_size;i++){
    
    if (cache[i].valid && cache[i].disk_num == disk_num && cache[i].block_num == block_num){
          cache_update(disk_num, block_num,buf);
          return -1;
    }
  }
  }

  
  


  if (inserts==0){

    cache[0].disk_num = disk_num;
    cache[0].block_num = block_num;
    cache[0].access_time =0;
    cache[0].valid =true;
    memcpy(cache[0].block,buf,JBOD_BLOCK_SIZE);
    clock+=1;
    inserts+=1;

  }
  else if (inserts<cache_size){
    cache[inserts].disk_num = disk_num;
    cache[inserts].block_num = block_num;
    memcpy(cache[inserts].block,buf,JBOD_BLOCK_SIZE);
    cache[inserts].access_time=clock;
    cache[inserts].valid =true;
    clock+=1;
    inserts+=1;
    
  }
  else if (inserts>=cache_size){
      int last_out =0;
      int last = cache[0].access_time;
      for (int i =0; i<=cache_size; i++){
        if (cache[i].access_time < last){
          last = cache[i].access_time;
          last_out = i;
        }
      }
        cache[last_out].disk_num = disk_num;
        cache[last_out].block_num = block_num;
        cache[last_out].access_time = clock;
        cache[last_out].valid = true;
        memcpy(cache[last_out].block,buf,JBOD_BLOCK_SIZE);
        clock+=1;
        inserts+=1;
    }

return 1;
  }


bool cache_enabled(void) {
  if (created ==1){
    return true;
  }
  else{
  return false;
  }
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}
