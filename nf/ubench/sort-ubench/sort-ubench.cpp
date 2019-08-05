#include <list>
#include <cstdlib>
#include <iostream>
#include <ctime>
#include <sys/time.h>

std::ostream &operator<<(std::ostream &ostr, const std::list<int> &list)
{
  for (auto &i : list)
  {
    ostr << " " << i;
  }
  return ostr;
}

int main(int argc, char **argv)
{

  long long list_size = atoi(argv[1]) * 1000;
  std::list<int> my_list;
  for (int i = 0; i < list_size; i++)
  {
    my_list.push_back(rand());
  }

  timespec start_time, end_time;

  // std::time_t start = std::time(nullptr);
  int gettime_result1 = clock_gettime(CLOCK_MONOTONIC, &start_time);

  my_list.sort();

  gettime_result1 = clock_gettime(CLOCK_MONOTONIC, &end_time);

  // std::time_t end = std::time(nullptr);

  std::cout << list_size << "," << ((end_time.tv_sec - start_time.tv_sec) * 1000000000 + (end_time.tv_nsec - start_time.tv_nsec))
            << std::endl;
}