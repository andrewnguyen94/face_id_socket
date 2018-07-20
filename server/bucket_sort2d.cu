#include "bucket_sort2d.cuh"

std::vector<Pair> get_search_query(std::vector<float> search_vector,
									std::vector<PersonInfo> personDatas)
{
	std::vector<Pair> result;
/*do something here*/
	int num_dimension = 128;
	int num_reference = 0;
	size_t size_of_database = personDatas.size();
	float *distance = new float[size_of_database];
	
	return result;
}