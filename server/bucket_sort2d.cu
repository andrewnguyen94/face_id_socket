#include "bucket_sort2d.cuh"

__global__ void bruteforceDistances (float *desQuery, float *desReference , float *distance, int referenceSize , int dim)
{
    //float sdiff[MAX_DATA_DIM];//If put fixed size, then need to care about the sdiff range over tid
    float sdiff, sdiff_sum;

    unsigned int tid = threadIdx.x + blockIdx.x*blockDim.x;
    //printf("Inside KERNEL\n");

    while (tid < referenceSize) {
        sdiff_sum = 0;

        //#pragma unroll: Use to store index, variables in registers, for speeding up
        //But here this does not help since data_dim (also tid, nodeIdx) is not known at compiling time

        int id1 = dim*tid;

#pragma unroll 32
        for (int i=0;i<dim;i++){
            sdiff = desReference[id1+i] - desQuery[i];
            sdiff = sdiff*sdiff;
            sdiff_sum += sdiff;
        }

        distance[tid] = sdiff_sum;

        tid += blockDim.x * gridDim.x;
    }



}

void get_descriptionId_from_db(int *descriptionId, std::vector<PersonInfo> personDatas)
{
	for(size_t i = 0; i < personDatas.size(); ++i){
		descriptionId[i] = (int)i;
	}
}

void get_reference_from_db(float *reference, std::vector<PersonInfo> personDatas)
{
	for(size_t i = 0; i < personDatas.size(); i++){
		dlib::matrix <float, 0, 1> info = personDatas[i].vec128;
		long rows = info.nr();
		long cols = info.nc();
		for(long r = 0; r < info.nr(); ++r){
			for(long c = 0; c < info.nc(); ++c){
				reference[i*rows*cols + r * cols + c] = info(r,c);
			}
		}
	}
}

void get_query_from_db(float *query, std::vector<float> search_vector)
{
	for(size_t i = 0; i < search_vector.size(); ++i){
		query[i] = search_vector[i];
	}
}

std::vector<Pair> get_search_query(std::vector<float> search_vector,
									std::vector<PersonInfo> personDatas, int number_of_vector_request)
{
	std::vector<Pair> result;
	size_t size_of_database = personDatas.size();
/*do something here*/
	int num_dimension = 128;
	int num_reference = size_of_database;
	std::cout << "num_reference :" << num_reference << std::endl;
	int dim = 128;
	
	float *distance = new float[num_reference];
	memset((void*)distance, 0, sizeof(float) * num_reference);
	float *query = new float[num_dimension];
	int *descriptionId = new int[num_reference];
	float *reference = new float[num_dimension * num_reference];
	get_query_from_db(query, search_vector);
	get_descriptionId_from_db(descriptionId, personDatas);
	get_reference_from_db(reference, personDatas);

	float *d_query = NULL;
	CHECK_ERROR(cudaMalloc((void**)&d_query, sizeof(float) * num_dimension));
	CHECK_ERROR(cudaMemcpy(d_query, query, sizeof(float) * num_dimension,cudaMemcpyHostToDevice));

	float *d_reference = NULL;
	CHECK_ERROR(cudaMalloc((void**)&d_reference, sizeof(float) * num_dimension * num_reference));
	CHECK_ERROR(cudaMemcpy(d_reference, reference, sizeof(float) * num_dimension * num_reference, cudaMemcpyHostToDevice));

	int *d_descriptionId = NULL;
	CHECK_ERROR(cudaMalloc((void**)&d_descriptionId, sizeof(int) * num_reference));
	CHECK_ERROR(cudaMemcpy(d_descriptionId, descriptionId, sizeof(int) * num_reference, cudaMemcpyHostToDevice));

	float *d_distance = NULL;
	CHECK_ERROR(cudaMalloc((void **)&d_distance, sizeof(float) * num_reference));
	CHECK_ERROR(cudaMemcpy(d_distance, distance, sizeof(float) * num_reference, cudaMemcpyHostToDevice));

	bruteforceDistances<<< GRID_DIM, BLOCK_DIM >>>(d_query, d_reference, d_distance, num_reference, dim);

	thrust::device_ptr<float> dd_keys(d_distance);
	thrust::device_ptr<int> dd_values(d_descriptionId);

	thrust::sort_by_key(dd_keys, dd_keys + num_reference, dd_values);

	cudaMemcpy(distance, d_distance, sizeof(float) * num_reference, cudaMemcpyDeviceToHost);
	cudaMemcpy(descriptionId, d_descriptionId, sizeof(int) * num_reference, cudaMemcpyDeviceToHost);

	if(number_of_vector_request > num_reference)
	{
		number_of_vector_request = num_reference;
	}

	for(int i = 0; i < number_of_vector_request; ++i){
		dlib::matrix <float, 0, 1> info = personDatas[descriptionId[i]].vec128;
		std::vector<float> content;
		for(long r = 0; r < info.nr(); ++r){
			for(long c = 0; c < info.nc(); ++c){
				content.push_back(info(r,c));
			}
		}
		Pair *p = new Pair(personDatas[descriptionId[i]].faceId, content);
		p->set_face_name(personDatas[descriptionId[i]].faceName);
		result.push_back(*p);
	}

	cudaFree(d_query);
	cudaFree(d_reference);
	cudaFree(d_descriptionId);
	cudaFree(d_distance);
	delete[] distance;
	delete[] query;
	delete[] descriptionId;
	delete[] reference;

	return result;
}