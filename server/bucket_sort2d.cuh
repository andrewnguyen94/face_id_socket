#ifndef BUCKET_SORT2D_CUH
#define BUCKET_SORT2D_CUH

/*include
---------------------------
---------------------------*/
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include<iostream>
#include<string>
#include <iostream>
#include <iomanip>

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/generate.h>
#include <thrust/sort.h>
#include <thrust/binary_search.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/random.h>

#include <thrust/execution_policy.h>
#include "../chat_message.hpp"
#include "../utils.h"

using namespace std;

/*define, constant
---------------------------
---------------------------*/


/*functions
---------------------------
---------------------------*/

std::vector<Pair> get_search_query(std::vector<float> search_vector,
 									std::vector<PersonInfo> personDatas);

#endif //end bucket_sort2d