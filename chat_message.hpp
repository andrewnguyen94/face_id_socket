#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

using namespace std;

typedef enum QUERYNAME{
  ENGINEID,
  SEARCH,
  ADD,
  DELETE
}QUERYNAME;

class chat_message
{
public:
  enum { header_length = 4 };
  enum { max_body_length = 5120 };

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

  void set_number_of_vectors_request(int n)
  {
    number_of_vectors_request = n;
  }

  int get_number_of_vectors_request()
  {
    return number_of_vectors_request;
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
    if(!get_content_vec().empty()){
      std::vector<float> cv = get_content_vec();
      std::ostringstream oss;
      std::copy(cv.begin(), cv.end() - 1, std::ostream_iterator<float>(oss, ","));
      oss << cv.back();
      jval["contentVec"] = oss.str();
    }
    return jval;
  }

private:

  QUERYNAME queryName;
  int face_id;
  int mes_id;
  int engineId;
  std::vector<float> contentVec;
  int number_of_vectors_request;
};

#endif // CHAT_MESSAGE_HPP
