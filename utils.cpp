#include "utils.h"

namespace{
  std::size_t callback(const char *in, std::size_t size, std::size_t num, std::string *out){
    const std::size_t totalBytes(size*num);
    out->append(in, totalBytes);
    return totalBytes;
  }
}

std::vector<PersonInfo> readFromDatabase()
{
	bool debug = false;
	int num_count = 0;
	int count;
	int limit = 100;
	int page = 1;
	std::vector<PersonInfo> personDatas;
loophere:
	personDatas.clear();
	CURL *curl2 = curl_easy_init();
	std::string url = "http://api.hrl.vp9.vn/api/members?limit="+std::to_string(limit)+"&page="+std::to_string(page);

	struct curl_slist *chunk = NULL;

	std::string header = "Company:5b05119360458946b7fb7770";
	chunk = curl_slist_append(chunk, header.c_str());

	/* set our custom set of headers */
	curl_easy_setopt(curl2, CURLOPT_HTTPHEADER, chunk);

	curl_easy_setopt(curl2, CURLOPT_URL, url.c_str());

	// Don't bother trying IPv6, which would increase DNS resolution time.
	curl_easy_setopt(curl2, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

	// Don't wait forever, time out after 10 seconds.
	curl_easy_setopt(curl2, CURLOPT_TIMEOUT, 10);

	// Follow HTTP redirects if necessary.
	curl_easy_setopt(curl2, CURLOPT_FOLLOWLOCATION, 1L);

	// Response information.
	int httpCode(0);
	std::unique_ptr<std::string> httpData(new std::string());

	// Hook up data handling function.
	curl_easy_setopt(curl2, CURLOPT_WRITEFUNCTION, callback);

	// Hook up data container (will be passed as the last parameter to the
	// callback handling function).  Can be any pointer type, since it will
	// internally be passed as a void pointer.
	curl_easy_setopt(curl2, CURLOPT_WRITEDATA, httpData.get());

	// Run our HTTP GET command, capture the HTTP response code, and clean up.
	curl_easy_perform(curl2);
	curl_easy_getinfo(curl2, CURLINFO_RESPONSE_CODE, &httpCode);
	curl_easy_cleanup(curl2);
	if (httpCode == 200) {
		if (debug)
	  		std::cout << "\nGot successful response from " << url << std::endl;
		Json::Value jsonData;
		Json::Reader jsonReader;

		if (jsonReader.parse(*httpData.get(), jsonData)) {
		  	if (debug) {
			    std::cout << "Successfully parsed JSON data" << std::endl;
			    std::cout << "\nJSON data received:" << std::endl;
		  	}
		  	if (page == 1)
		    	count = jsonData["count"].asInt();

		  	std::map<string, matrix<float, 0, 1>> gallery_face_descriptor;
		  	for (auto row : jsonData["rows"]) {
		      	num_count++;

			    std::string id = row["id"].asString();
			    std::string name = row["name"].asString();
			    std::string gender = row["gender"].asString();
			    std::string age = row["age"].asString();
			    if (debug)
			      	cout << "faceid: " << id << endl;
		    	for (auto itr : row["photos"]) {
		      		string src = itr["src"].asString();

		      		auto vecto = itr["vector"];

		      		dlib::matrix<float, 128, 1> newVec;
		      		for (int i = 0; i < 128; ++i) {
		        		newVec(i) = vecto.get(i, 0).asFloat();
		      		}
					PersonInfo info;
					info.faceId = id;
					info.faceName = name;
					info.vec128 = newVec;
					info.url = src;
					info.faceAge = age;
					info.faceGender = gender;
					personDatas.push_back(info);
		    	}
		  	}
		  	httpData->clear();
		} else {
		  	if (debug) {
			    std::cout << "Could not parse HTTP data as JSON" << std::endl;
			    std::cout << "HTTP data was:\n" << *httpData.get() << std::endl;
		  	}
		  	exit(0);
		}
	}

  	count = count - 100;
  	if (count > 0) {
		page++;
		goto loophere;
  	}

  	std::cout << "\nNumber of face profiles loaded: " <<num_count<< std::endl;
  	return personDatas;
}

std::vector<cv::Mat> convert_from_uschar_to_mat(unsigned char *data)
{
	
}

int search_special_char(char *arr, size_t len)
{
	int count = 0;
	for(size_t i = 0; i < len; ++i){
		if(arr[i] == '{')
		{
			count ++;
		}
	}
	return count;
}

int search_special_char_inverse(char *arr, size_t len)
{
	int count = 0;
	for(size_t i = 0; i < len; ++i){
		if(arr[i] == '}')
			count++;
		return count;
	}
}