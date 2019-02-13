
#include <stdio.h>
#include <stdlib.h>
#include "lib/containers/map-impl.h"
#include "lib/containers/map.h"
#include "lib/flow.h"
#include <limits.h>
#include <time.h>
#include <assert.h>
#include "x86intrin.h"
#define capacity 65536
#define max_size 41943040

long long* a;
struct timespec gNeverZero;

static long long wrap(long long x)
//@ requires true;
//@ ensures result == _wrap(x) &*& INT_MIN <= result &*& result <= INT_MAX;
{
  //@ div_rem(x, INT_MAX);
  return x % INT_MAX;
}

int int_key_hash(void* key)
//@ requires [?f]int_k_p(key, ?k);
//@ ensures [f]int_k_p(key, k) &*& result == int_hash(k);
{
  struct int_key* ik = key;

  long long hash = ik->int_src_port;
  hash *= 31;

  hash += ik->dst_port;
  hash *= 31;

  hash += ik->int_src_ip;
  hash *= 31;

  hash += ik->dst_ip;
  hash *= 31;

  hash += ik->int_device_id;
  hash *= 31;

  hash += ik->protocol;

  hash = wrap(hash);
  //if(gNeverZero.tv_nsec==0)
  //	return (int) hash;
  return ik ->int_src_port;
}

int ext_key_hash(void* key)
//@ requires [?f]ext_k_p(key, ?k);
//@ ensures [f]ext_k_p(key, k) &*& result == ext_hash(k);
{
  struct ext_key* ek = key;

  long long hash = ek->ext_src_port;
  hash *= 31;

  hash += ek->dst_port;
  hash *= 31;

  hash += ek->ext_src_ip;
  hash *= 31;

  hash += ek->dst_ip;
  hash *= 31;

  hash += ek->ext_device_id;
  hash *= 31;

  hash += ek->protocol;

  hash = wrap(hash);

  //if(gNeverZero.tv_nsec==0)
  //	return (int) hash;
  return ek -> ext_src_port;
}


void clear_cache(long time)
{
  long long sum=0;
  for(long long i=0;i<max_size;i++)
  { a[i] = i;
  }

   for(long long i=0;i<max_size;i++)
  { sum+=a[i];
  }
  if(time ==0) printf("Sum is %lld \n",sum);
}

int main()
{

a = malloc(sizeof(*a)*max_size);
if(a==NULL) printf("malloc_failed\n");


 struct Map* map;
 map_allocate(int_key_eq,int_key_hash,capacity,&map);
 struct int_key *k1 = malloc(sizeof(*k1)*capacity);
 long value,a,hash,erase_key ;
 int y,res;
 int* val = malloc(sizeof *val);
 void** b = malloc(sizeof(void*));
 struct int_key* k2 = malloc(sizeof *k2);

 struct timespec start_time, end_time;
 int ret;
 ret = clock_gettime(CLOCK_MONOTONIC,&gNeverZero);

for (int i = 0; i < capacity; i++)
{
 k1[i].int_src_port = i;
 k1[i].dst_port =i;
 k1[i].int_src_ip = i;
 k1[i].dst_ip = i;
 k1[i].int_device_id = i;
 k1[i].protocol = i;
 value = 50*i;
 map_put(map,&k1[i],value);

}
 int i = 2*capacity-1;
 k2->int_src_port = i;
 k2->dst_port =i;
 k2->int_src_ip = i;
 k2->dst_ip = i;
 k2->int_device_id = i;
 k2->protocol = i;
 value = 50*i;
	
 long total_time = 0;

#if defined(CLEARED_CACHE) || defined(ARTIFICIAL)
 clear_cache(gNeverZero.tv_nsec);
#endif

/* Measuring map_get*/
 y= clock_gettime(CLOCK_MONOTONIC,&start_time);

 map_erase(map,&k1[capacity-1],b);

#ifdef ARTIFICIAL
 y= clock_gettime(CLOCK_MONOTONIC,&end_time);
 total_time += (end_time.tv_sec - start_time.tv_sec)*1000000000 + (end_time.tv_nsec - start_time.tv_nsec);
 clear_cache(gNeverZero.tv_nsec);
 y= clock_gettime(CLOCK_MONOTONIC,&start_time);
#endif //ARTIFICAL 

/* Measuring map_erase*/

 res = map_get(map,k2,val);

#ifdef ARTIFICIAL
 y= clock_gettime(CLOCK_MONOTONIC,&end_time);
 total_time += (end_time.tv_sec - start_time.tv_sec)*1000000000 + (end_time.tv_nsec - start_time.tv_nsec);
 clear_cache(gNeverZero.tv_nsec);
 y= clock_gettime(CLOCK_MONOTONIC,&start_time);
#endif //ARTIFICAL

/* Measuring map_put*/

  map_put(map,k2,value);

  y= clock_gettime(CLOCK_MONOTONIC,&end_time);

  total_time += (end_time.tv_sec - start_time.tv_sec)*1000000000 + (end_time.tv_nsec - start_time.tv_nsec);
  printf("Time is %ld\n",total_time);
}
