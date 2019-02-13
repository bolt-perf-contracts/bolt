#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "x86intrin.h"
#include "lib/flowmanager.h"
#include "lib/expirator.h"
#include <limits.h>
#define capacity 65536
#define occupancy 65536
#define nfreed 65536
#define max_size 41943040

long long* a;

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

  hash += ik->protocol;
  hash *= 31;

  hash += ik->dst_ip;
  hash *= 31;

  hash += ik->int_device_id;
  hash *= 31;

  //hash += ik->protocol;

  hash = wrap(hash);
  //return 0;
  //return ik->int_src_port;
  return (int) hash;
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

  hash += ek->protocol;
  hash *= 31;

  hash += ek->dst_ip;
  hash *= 31;

  hash += ek->ext_device_id;
  hash *= 31;

  //hash += ek->protocol;

  hash = wrap(hash);
  return 0;
  //return ek->ext_src_port;
  return (int) hash;
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
   struct timespec gNeverZero;
   int ret = clock_gettime(CLOCK_MONOTONIC,&gNeverZero);
  struct FlowManager* flow_manager; 
  int alloc_rez=1; 
  flow_manager  = allocate_flowmanager(0,0,0,0,0,capacity);
  struct int_key *keys = malloc(sizeof(*keys)*occupancy);
  printf("%lu\n",sizeof(*keys));
  struct flow flow;
   for (int i = 0; i < occupancy; i++)
  {  
     keys[i].int_src_port = 1;
     keys[i].dst_port =1;
     keys[i].int_src_ip = i;
     keys[i].dst_ip = 1;
     keys[i].int_device_id = 1;
     keys[i].protocol = 1;
     alloc_rez = allocate_flow(flow_manager,&keys[i],occupancy-i,&flow);
     if (0 == alloc_rez) printf("Out of resources \n"); //Out of resources.
     
  }
 
  printf("First allocation done \n");
  for (int i = occupancy-1; i >=0 ; i--)
  {
     alloc_rez = get_flow_by_int_key(flow_manager,&keys[i],occupancy-i,&flow);
     if (0 == alloc_rez) printf("Failed to rejuvenate \n"); //Failed to expire
      
  }

  printf("Rejuvenation done \n");
  
  struct timespec start_time, end_time;

  clear_cache(gNeverZero.tv_nsec);
  printf("Clearing cache \n");
 int y= clock_gettime(CLOCK_MONOTONIC,&start_time);



  alloc_rez = expire_flows(flow_manager,nfreed+1);
 

  y= clock_gettime(CLOCK_MONOTONIC,&end_time);

  
  if (0 == alloc_rez) printf("Test Failed to expire \n"); //Failed to expire

  printf("Time is %ld \n",((end_time.tv_sec - start_time.tv_sec)*1000000000 + (end_time.tv_nsec - start_time.tv_nsec)));


}
