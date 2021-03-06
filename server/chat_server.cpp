#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <dlib/matrix.h>
#include <curl/curl.h>
#include <string.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "jsoncpp/json/json.h"
#include "../chat_message.hpp"
#include "../utils.h"
#include "IOData.h"
#include "Onvif.h"
#include "bucket_sort2d.cuh"


using boost::asio::ip::tcp;
using namespace dlib;
using namespace cv;
//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------


char *get_json_value(char *val, size_t size){
  int count = 0;
  int count_ret = 0;
  char *ret;
  char s;
  char key = '{';
  for(size_t i = 0; i < size; ++i){
      s = val[i];
      if(s != '{') {
          count++;
      }else{
          break;
      }
  }   
  ret = new char[(int)(size - count)];
  for(size_t i = count; i < size; ++i){
      ret[count_ret] = val[i];
      count_ret ++;
  }
  return ret;
}

QUERYNAME get_query_name_from_string(std::string name){
  if(name.compare("1") == 0) return SEARCH;
  else if(name.compare("0") == 0) return ENGINEID;
  else if(name.compare("2") == 0) return ADD;
  else if(name.compare("3") == 0) return DELETE;
  else if(name.compare("4") == 0) return SEND;
  else if(name.compare("5") == 0) return BRIGHTNESS;
  else if(name.compare("6") == 0) return PUSH_BRIGHTNESS;
  else{
    std::cout << "query name not found!" << std::endl;
    exit(0);
  }
}

class chat_participant
{
public:
  int remoteEngineId;
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
};

typedef boost::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
  void join(chat_participant_ptr participant)
  {
    participants_.insert(participant);
    participant->remoteEngineId = globalId;
    globalId ++;
    chat_message msg;
    Json::Value jsonValue;
    jsonValue["engineId"] = participant->remoteEngineId;;
    jsonValue["queryName"] = ENGINEID;
    jsonValue["contentVec"] = "";
    std::string str = jsonValue.toStyledString();
    msg.body_length(str.length());
    std::memcpy(msg.body(), str.c_str(), msg.body_length());
    msg.encode_header();

    participant->deliver(msg);
    // std::for_each(recent_msgs_.begin(), recent_msgs_.end(),
    //     boost::bind(&chat_participant::deliver, participant, _1));
  }

  void leave(chat_participant_ptr participant)
  {
    participants_.erase(participant);
  }

  void deliver(const chat_message& msg)
  {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    std::for_each(participants_.begin(), participants_.end(),
        boost::bind(&chat_participant::deliver, _1, boost::ref(msg)));
  }

  void deliverToOneEngine(const chat_message& msg, int engineId)
  {
    if(participants_.empty() != 1){
      BOOST_FOREACH(chat_participant_ptr ptc, participants_){
        if(ptc->remoteEngineId == engineId){
          ptc->deliver(boost::ref(msg));
        }
      }
    }
  }

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;
  int globalId = 0;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public boost::enable_shared_from_this<chat_session>
{
public:
  chat_session(boost::asio::io_service& io_service, chat_room& room, std::vector<PersonInfo> personDatas)
    : socket_(io_service),
      room_(room)
  {
    set_personDatas(personDatas);
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void set_personDatas(std::vector<PersonInfo> p)
  {
    std::vector<PersonInfo>::iterator it;
    it = p.begin();
    personDatas.assign(it, p.end());
  }

  void start()
  {
    room_.join(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        boost::bind(
          &chat_session::handle_read_header, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void deliver(const chat_message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      boost::asio::async_write(socket_,
          boost::asio::buffer(write_msgs_.front().data(),
            write_msgs_.front().length()),
          boost::bind(&chat_session::handle_write, shared_from_this(),
            boost::asio::placeholders::error));
    }
  }


  void handle_read_header(const boost::system::error_code& error, size_t bytes_transferred)
  {
    if (!error && read_msg_.decode_header())
    {
      boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        boost::bind(&chat_session::handle_read_body, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      room_.leave(shared_from_this());
    }
  }

  void handle_read_body(const boost::system::error_code& error, size_t bytes_transferred)
  {
    if(!error){
    
      Json::Value jval;
      Json::Reader jsonReader;
      chat_message msg;     
      //
      char *ret = get_json_value(read_msg_.body(), read_msg_.body_length());
      bool parsingSuccessful = jsonReader.parse(ret, jval); // parse process
      if (!parsingSuccessful) {
          std::cout << "Failed to parse" << jsonReader.getFormattedErrorMessages();
      }
      int engineId = jval["engineId"].asInt();
      int mes_id = jval["mes_id"].asInt();
      std::string queryName_str = jval["queryName"].asString();
      QUERYNAME queryName = get_query_name_from_string(queryName_str);
      Json::Value json_res;
      if(queryName == SEARCH){
        int number_of_vector_request = jval["nov"].asInt();
        std::string contentVec = jval["contentVec"].asString();
        float number;
        std::stringstream ss(contentVec);
        std::vector<float> content;
        while(ss >> number)
        {
          content.push_back(number);
        }

        // thuc hien cau query de search vector va face id o day
        
        std::vector<Pair> contentResponse = get_search_query(content, personDatas, number_of_vector_request);
        std::cout << "contentResponse size :" << contentResponse.size() << std::endl;
        //xu ly contentResponse
        Json::Value result;
        std::string key = "Pair";
        for(size_t i = 0; i < contentResponse.size(); ++i){
          std::stringstream ss;
          std::ostringstream ssi;
          ssi << i;
          ss << key << "_" << ssi.str();
          std::string key_el = ss.str();
          Json::Value val_tmp = contentResponse[i].parse_content_to_json();
          result[key_el] = val_tmp;
        }
        //
        //truyen vao message tra ve client
        json_res["engineId"] = engineId;
        json_res["mes_id"] = mes_id;
        json_res["queryName"] = queryName;
        json_res["nov"] = number_of_vector_request;
        json_res["responseContent"] = result;
        //
      }
      else if(queryName == ADD)
      {
        int face_id = jval["face_id"].asInt();
        std::string contentVec = jval["contentVec"].asString();
        float number;
        std::stringstream ss(contentVec);
        std::vector<float> content;
        while(ss >> number)
        {
          content.push_back(number);
        }
        //thuc hien cau query add o day
        std::string resp = "Add Done";
        json_res["engineId"] = engineId;
        json_res["mes_id"] = mes_id;
        json_res["queryName"] = queryName;
        json_res["face_id"] = face_id;
        json_res["responseContent"] = resp;
        //
      }
      else if(queryName == DELETE)
      {
        int face_id = jval["face_id"].asInt();

        //thuc hien cau query delete o day
        std::string resp = "Delete Done";
        json_res["engineId"] = engineId;
        json_res["mes_id"] = mes_id;
        json_res["queryName"] = queryName;
        json_res["face_id"] = face_id;
        json_res["responseContent"] = resp;
        //

      }
      else if(queryName == SEND)
      {
        std::string resp = "Send Done";
        std::vector<cv::Mat> imgVector;
        Json::Value contentVec = jval["contentVec"];
        int number_of_image_request = jval["noi"].asInt();

        std::string key = "image_";

        for(int i = 0; i < number_of_image_request; ++i){
          std::stringstream ss;
          std::ostringstream ssi;
          ssi << i;
          ss << key << ssi.str();
          std::string key_el = ss.str();

          Json::Value val = contentVec[key_el];
          int w = val["width"].asInt();
          int h = val["height"].asInt();
          int d = val["dim"].asInt();
          int t = val["type"].asInt();
          int s = val["step"].asInt();
          std::string content = val["content"].asString();
          size_t size = 0;
          std::stringstream content_stream(content);
          content_stream.read((char *)(&size), sizeof(size_t));
          char *content_char = new char[size];
          content_stream.read(content_char, size);

          std::cout << "w, h, d, t, s : " << w << "," << h << "," << d << ","<< t << "," << s << "," << std::endl;
          cv::Mat mat_el = cv::Mat(h, w, t, content_char);
          imgVector.push_back(mat_el);
        }
        json_res["engineId"] = engineId;
        json_res["mes_id"] = mes_id;
        json_res["queryName"] = queryName;
        json_res["responseContent"] = resp;
      }
      else if(queryName == BRIGHTNESS)
      {
        // IOData *ioData = new IOData();
        // std::string IPCameraAddress = ioData->GetCongfigData("IPCameraAddress:").c_str();
        // OnvifController onvifController(IPCameraAddress+":2000");
        // std::cout << "IPCameraAddress :" << IPCameraAddress << std::endl;
        OnvifController onvifController("10.13.11.146:2000"); // Cam a Tuc
        int current_brightness = 0;
        current_brightness = onvifController.getBrightness();
        std::cout << "current_brightness" << current_brightness << std::endl;
        json_res["engineId"] = engineId;
        json_res["mes_id"] = mes_id;
        json_res["queryName"] = queryName;
        json_res["responseContent"] = current_brightness;
      }
      else if(queryName == PUSH_BRIGHTNESS)
      {
        std::string success = "push done";
        int brightness_set = jval["brightness"].asInt();

        //xu ly brightness o day

        json_res["engineId"] = engineId;
        json_res["mes_id"] = mes_id;
        json_res["queryName"] = queryName;
        json_res["responseContent"] = success;        
      }
      // xu ly doan query o day
      //sau khi xu ly xong, nhan duoc du lieu
      std::string str = json_res.toStyledString();
      msg.body_length(str.length());
      std::memcpy(msg.body(), str.c_str(), msg.body_length());
      msg.encode_header();
      //
      room_.deliverToOneEngine(msg, engineId);
      boost::asio::async_read(socket_,
          boost::asio::buffer(read_msg_.data(), chat_message::header_length),
          boost::bind(&chat_session::handle_read_header, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      room_.leave(shared_from_this());
    }
  }

  void handle_write(const boost::system::error_code& error)
  {
    if (!error)
    {
      write_msgs_.pop_front();
      if (!write_msgs_.empty())
      {
        boost::asio::async_write(socket_,
            boost::asio::buffer(write_msgs_.front().data(),
              write_msgs_.front().length()),
            boost::bind(&chat_session::handle_write, shared_from_this(),
              boost::asio::placeholders::error));
      }
    }
    else
    {
      room_.leave(shared_from_this());
    }
  }

private:
  tcp::socket socket_;
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  char *body = "";
  size_t body_length = 0;
  std::vector<PersonInfo> personDatas;
};

typedef boost::shared_ptr<chat_session> chat_session_ptr;

//----------------------------------------------------------------------

class chat_server
{
public:
  chat_server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint, std::vector<PersonInfo> personDatas)
    : io_service_(io_service),
      acceptor_(io_service, endpoint)
  {
    set_personDatas(personDatas);
    start_accept();
  }

  void set_personDatas(std::vector<PersonInfo> p)
  {
    std::vector<PersonInfo>::iterator it;
    it = p.begin();
    personDatas.assign(it, p.end());
  }

  void start_accept()
  {
    chat_session_ptr new_session(new chat_session(io_service_, room_, personDatas));
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&chat_server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

  void handle_accept(chat_session_ptr session,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      session->start();
    }

    start_accept();
  }

private:
  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
  chat_room room_;
  std::vector<PersonInfo> personDatas;
};

typedef boost::shared_ptr<chat_server> chat_server_ptr;
typedef std::list<chat_server_ptr> chat_server_list;

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      std::cerr << "Usage: chat_server <port> [<port> ...]\n";
      return 1;
    }

    std::vector<PersonInfo> personDatas = readFromDatabase();
    std::cout << "personDatas size: " << personDatas.size() << std::endl;
    boost::asio::io_service io_service;

    chat_server_list servers;
    for (int i = 1; i < argc; ++i)
    {
      using namespace std; // For atoi.
      tcp::endpoint endpoint(tcp::v4(), atoi(argv[i]));
      chat_server_ptr server(new chat_server(io_service, endpoint, personDatas));
      servers.push_back(server);
    }

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
