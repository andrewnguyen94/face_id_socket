#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string.h>
#include <sstream>
#include <opencv2/core.hpp>
#include "jsoncpp/json/json.h"

using namespace std;
using namespace cv;

typedef enum QUERYNAME{
  ENGINEID,
  SEARCH,
  ADD,
  DELETE,
  SEND
}QUERYNAME;

class chat_message
{
public:
  enum { header_length = 4 };
  enum { max_body_length = 0xA0000 };

  chat_message()
    : body_length_(0)
  {
  }

  const char* data() const
  {
    return data_;
  }

  char* data()
  {
    return data_;
  }

  size_t length() const
  {
    return header_length + body_length_;
  }

  const char* body() const
  {
    return data_ + header_length;
  }

  char* body()
  {
    return data_ + header_length;
  }

  size_t body_length() const
  {
    return body_length_;
  }

  void body_length(size_t new_length)
  {
    body_length_ = new_length;
    if (body_length_ > max_body_length)
      body_length_ = max_body_length;
  }

  bool decode_header()
  {
    using namespace std; // For strncat and atoi.
    char header[header_length + 1] = "";
    strncat(header, data_, header_length);
    body_length_ = atoi(header);
    if (body_length_ > max_body_length)
    {
      body_length_ = 0;
      return false;
    }
    return true;
  }

  void encode_header()
  {
    using namespace std; // For sprintf and memcpy.
    char header[header_length + 1] = "";
    sprintf(header, "%4d", static_cast<int>(body_length_));
    memcpy(data_, header, header_length);
  }

private:
  char data_[header_length + max_body_length];
  size_t body_length_;
};

class Pair
{
public:
  Pair(int face_id, std::vector<float> vector_content)
    :face_id(face_id), vector_content(vector_content)
  {

  }
  ~Pair()
  {
    
  }

  void set_face_id(int face_id)
  {
    face_id = face_id;
  }

  int get_face_id()
  {
    return face_id;
  }

  void set_content_vec(std::vector<float> v)
  {
    std::vector<float>::iterator it;
    it = v.begin();
    vector_content.assign(it, v.end());
  }

  std::vector<float> get_content_vec()
  {
    return vector_content;
  }

  Json::Value parse_content_to_json(){
    Json::Value val;
    val["id"] = get_face_id();
    std::vector<float> tmp = get_content_vec();
    std::ostringstream oss;
    std::copy(tmp.begin(), tmp.end() - 1, std::ostream_iterator<float>(oss, " "));
    oss << tmp.back();

    val["content"] = oss.str();
    return val;
  }
private:
  int face_id;
  std::vector<float> vector_content;
};

class response
{
public:
  response()
  {

  }

  void set_face_id(int id)
  {
    face_id = id;
  }

  int get_face_id()
  {
    return face_id;
  }

  void set_mes_id(int id)
  {
    mes_id = id;
  }

  int get_mes_id()
  {
    return mes_id;
  }

  void set_engine_id(int id)
  {
    engineId = id;
  }

  int get_engine_id()
  {
    return engineId;
  }

  void set_query_name(QUERYNAME name){
    queryName = name;
  }

  QUERYNAME get_query_name(){
    return queryName;
  }

  void set_result_res(std::vector<Pair> res)
  {
    std::vector<Pair>::iterator it;
    it = res.begin();
    result_res.assign(it, res.end());
  }

  std::vector<Pair> get_result_res()
  {
    return result_res;
  }

  void set_content(std::string c)
  {
    content.assign(c);
  }

  std::string get_content()
  {
    return content;
  }

private:
  int mes_id;
  int engineId;
  QUERYNAME queryName;
  std::vector<Pair> result_res;
  std::string content;
  int face_id;
};

class image
{
public:
  image(int w, int h, int d, int t, int st)
    : width(w), height(h), dim(d), type(t), step(st)
  {

  }

  void set_width(int w)
  {
    width = w;
  }

  int get_width()
  {
    return width;
  }

  void set_height(int h)
  {
    height = h;
  }

  int get_height()
  {
    return height;
  }

  void set_dim(int d)
  {
    dim = d;
  }

  int get_dim()
  {
    return dim;
  }

  void set_type(int t)
  {
    type = t;
  }

  int get_type()
  {
    return type;
  }

  void set_step(int st)
  {
    step = st;
  }

  int get_step()
  {
    return step;
  }

  void set_data(unsigned char *d, size_t l)
  {
    data = (unsigned char *)malloc(sizeof(unsigned char) * l);
    memcpy(data, d, l);
  }

  unsigned char *get_data()
  {
    return data;
  }

  Json::Value parse_image_to_json()
  {
    Json::Value val;
    val["width"] = get_width();
    val["height"] = get_height();
    val["dim"] = get_dim();
    val["type"] = get_type();
    val["step"] = get_step();
    std::string data_str(reinterpret_cast<char*>(get_data()));
    val["content"] = data_str;

    return val;
  }

private:
  unsigned char *data;
  int width;
  int height;
  int dim;
  int step;
  int type;

};

class request
{
public:
  request(int engineId, int mes_id, QUERYNAME queryName)
    : engineId(engineId), mes_id(mes_id), queryName(queryName)
  {

  }

  void set_face_id(int id)
  {
    face_id = id;
  }

  int get_face_id()
  {
    return face_id;
  }

  void set_mes_id(int id)
  {
    mes_id = id;
  }

  int get_mes_id()
  {
    return mes_id;
  }

  void set_engine_id(int id)
  {
    engineId = id;
  }

  int get_engine_id()
  {
    return engineId;
  }

  void set_query_name(QUERYNAME name){
    queryName = name;
  }

  QUERYNAME get_query_name()
  {
    return queryName;
  }

  void set_content_vec(std::vector<float> v)
  {
    std::vector<float>::iterator it;
    it = v.begin();
    contentVec.assign(it, v.end());
  }

  std::vector<float> get_content_vec()
  {
    return contentVec;
  }

  void set_content_img_vec(std::vector<cv::Mat> img_vec)
  {
    std::vector<cv::Mat>::iterator it;
    it = img_vec.begin();
    image_vec.assign(it, img_vec.end());
  }

  std::vector<cv::Mat> get_content_img_vec()
  {
    return image_vec;
  }

  void set_number_of_vectors_request(int n)
  {
    number_of_vectors_request = n;
  }

  int get_number_of_vectors_request()
  {
    return number_of_vectors_request;
  }

  void set_number_of_image_request(int n)
  {
    number_of_image_request = n;
  }

  int get_number_of_image_request()
  {
    return number_of_image_request;
  }

  Json::Value parse_request_to_json()
  {
    Json::Value jval;
    jval["engineId"] = get_engine_id();
    jval["queryName"] = get_query_name();
    if(jval["queryName"] == SEARCH){
      jval["nov"] = get_number_of_vectors_request();
    }
    jval["mes_id"] = get_mes_id();
    if(jval["queryName"] == SEND){
      std::vector<cv::Mat> cv = get_content_img_vec();
      jval["noi"] = get_number_of_image_request();
      Json::Value j_tmp;
      std::string key = "image_";
      for(size_t i = 0; i < cv.size(); ++i){
        image *img = new image(cv[i].cols, cv[i].rows, cv[i].channels(), cv[i].type(), cv[i].step);
        size_t size = (size_t)(cv[i].cols * cv[i].rows * cv[i].channels());
        img->set_data(cv[i].data, size);
        std::stringstream ss;
        std::ostringstream oss;
        oss << i;
        ss << key << oss.str();
        std::string key_el = ss.str();
        
        j_tmp[key_el] = img->parse_image_to_json();       
      }

      jval["contentVec"] = j_tmp;
    }else{
      if(!get_content_vec().empty()){  
        std::vector<float> cv = get_content_vec();
        std::ostringstream oss;
        std::copy(cv.begin(), cv.end() - 1, std::ostream_iterator<float>(oss, " "));
        oss << cv.back();
        jval["contentVec"] = oss.str();      
      }
    }
    return jval;
  }

private:

  QUERYNAME queryName;
  int face_id;
  int mes_id;
  int engineId;
  std::vector<float> contentVec;
  std::vector<cv::Mat> image_vec;
  int number_of_image_request;
  int number_of_vectors_request;
};

#endif // CHAT_MESSAGE_HPP
