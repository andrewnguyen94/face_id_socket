#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>

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
  enum { max_body_length = 512 };

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

  void set_content(std::string c)
  {
    content.assign(c);
  }

  std::string get_content()
  {
    return content;
  }

private:
  int face_id;
  int mes_id;
  int engineId;
  QUERYNAME queryName;
  std::vector<float> contentVec;
  std::string content;
};

class request
{
public:
  request(int engineId, int mes_id, QUERYNAME queryName, int face_id)
    : engineId(engineId), mes_id(mes_id), face_id(face_id), queryName(queryName)
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

private:

  Json::Value parse_request_to_json()
  {
    Json::Value jval;
    jval["engineId"] = get_engine_id();
    jval["queryName"] = get_query_name();
    jval["mes_id"] = get_mes_id();
    if(!get_content_vec().empty()){
      jval["contentVec"] = get_content_vec();
    }
    return jval;
  }

  QUERYNAME queryName;
  int face_id;
  int mes_id;
  int engineId;
  std::vector<float> contentVec;

};

#endif // CHAT_MESSAGE_HPP
