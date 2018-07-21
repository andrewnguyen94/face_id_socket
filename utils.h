#ifndef UTILS_H
#define UTILS_H

/*include
------------------------------
------------------------------*/
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <dlib/matrix.h>
#include <curl/curl.h>
#include <string>
#include <string.h>
#include <opencv2/core.hpp>
#include "jsoncpp/json/json.h"


/*define, constant
------------------------------
------------------------------*/

using namespace std;
using namespace dlib;
using namespace cv;

struct PersonInfo {
  std::string faceId; // faceID (number) of this vector
  std::string faceName;
  std::string faceAge;
  std::string faceGender;
  std::string url;
  dlib::matrix<float, 0, 1> vec128; // vector 128
};

/*functions
------------------------------
------------------------------*/

std::vector<PersonInfo> readFromDatabase();
std::vector<cv::Mat> convert_from_uschar_to_mat(unsigned char *data);
int search_special_char(char *arr, size_t len);
int search_special_char_inverse(char *arr, size_t len);
#endif