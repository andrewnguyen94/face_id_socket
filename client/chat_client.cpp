#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <sstream>
#include "jsoncpp/json/json.h"
#include "../chat_message.hpp"

using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

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
  if(name.compare("search") == 0 || name.compare("SEARCH") == 0) return SEARCH;
  else if(name.compare("engine_Id") == 0 || name.compare("ENGINEID") == 0) return ENGINEID;
  else if(name.compare("add") == 0 || name.compare("ADD") == 0) return ADD;
  else if(name.compare("delete") == 0 || name.compare("DELETE") == 0) return DELETE;
  else{
    std::cout << "query name not found!" << std::endl;
    return 1;
  }
}

class chat_client
{
public:
  chat_client(boost::asio::io_service& io_service,
      tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service),
      socket_(io_service)
  {
    boost::asio::async_connect(socket_, endpoint_iterator,
        boost::bind(&chat_client::handle_connect, this,
          boost::asio::placeholders::error));
  }

  void write(const chat_message& msg)
  {
    io_service_.post(boost::bind(&chat_client::do_write, this, msg));
  }

  void close()
  {
    io_service_.post(boost::bind(&chat_client::do_close, this));
  }

  void set_engine_Id(int id)
  {
    engineId = id;
  }

  int get_engine_Id()
  {
    return engineId;
  }

  void set_flag(bool f)
  {
    flag = f;
  }

  bool get_flag()
  {
    return flag;
  }

  void set_flag_engine(bool f)
  {
    flag_engine = f;
  }

  bool get_flag_engine()
  {
    return flag_engine;
  }

  void wait_for_response()
  {
    while(get_flag() != true){
      //do nothting
    }
    set_flag(false);
  }

  void wait_for_get_engine_id()
  {
    while(get_flag_engine() != true){
      // do nothing here
    }
  }

  void set_response(response response)
  {
    res = response;
  } 

  response get_response()
  {
    return res;
  }

private:

  void handle_connect(const boost::system::error_code& error)
  {
    if (!error)
    {
      boost::asio::async_read(socket_,
          boost::asio::buffer(read_msg_.data(), chat_message::header_length),
          boost::bind(&chat_client::handle_read_header, this,
            boost::asio::placeholders::error));
    }
  }

  void handle_read_header(const boost::system::error_code& error)
  {
    if (!error && read_msg_.decode_header())
    {
      boost::asio::async_read(socket_,
          boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
          boost::bind(&chat_client::handle_read_body, this,
            boost::asio::placeholders::error));
    }
    else
    {
      do_close();
    }
  }

  void handle_read_body(const boost::system::error_code& error)
  {
    if (!error)
    {
      Json::Value jsonValue;
      Json::Reader jsonReader;

      char *ret = get_json_value(read_msg_.body(), read_msg_.body_length());
      bool parsingSuccessful = jsonReader.parse(ret, jsonValue); // parse process
      if (!parsingSuccessful) {
          std::cout << "Failed to parse" << jsonReader.getFormattedErrorMessages();
      }
      response resp = new response();
      std::string queryName_str = jsonValue["queryName"].asString();
      QUERYNAME queryName = get_query_name_from_string(queryName_str);
      if (queryName == ENGINEID){
        set_engine_Id(jsonValue["engineId"].asInt());
        resp.set_engine_id(jsonValue["engineId"].asInt());
        set_response(resp);
        set_flag_engine(true);
        std::cout << "This engine is assinged to ID: " <<get_engine_Id()<< std::endl;
      }
      else if (queryName == SEARCH){
        int face_id = jsonValue["face_id"].asInt();
        int mes_id = jsonValue["mes_id"].asInt();
        int engine_id = jsonValue["engineId"].asInt();
        std::string contentResponse = jsonValue["responseContent"].asString();

        std::stringstream ss(contentResponse);
        float number;
        std::vector<float> content
        while(ss >> number)
        {
          content.push_back(number);
        }

        resp.set_face_id(face_id);
        resp.set_mes_id(mes_id);
        resp.set_engine_id(engine_id);
        resp.set_query_name(queryName);
        resp.set_content(contentResponse);
        resp.set_content_vec(content);
        set_flag(true);
      }
      else if(queryName == ADD){
        int mes_id = jsonValue["mes_id"].asInt();
        int engine_id = jsonValue["engineId"].asInt();
        int face_id = jsonValue["face_id"].asInt();
        std::string contentResponse = jsonValue["responseContent"].asString();
        resp.set_engine_id(engine_id);
        resp.set_mes_id(mes_id);
        resp.set_content(contentResponse);
        resp.set_face_id(face_id);
        set_response(resp);
        set_flag(true);
      }
      else if(queryName == DELETE){
        int mes_id = jsonValue["mes_id"].asInt();
        int engine_id = jsonValue["engineId"].asInt();
        int face_id = jsonValue["face_id"].asInt();
        std:string contentResponse = jsonValue["responseContent"].asString();

        resp.set_engine_id(engine_id);
        resp.set_mes_id(mes_id);
        resp.set_content(contentResponse);
        resp.set_face_id(face_id);
        set_response(resp);
        set_flag(true);
      }

      boost::asio::async_read(socket_,
          boost::asio::buffer(read_msg_.data(), chat_message::header_length),
          boost::bind(&chat_client::handle_read_header, this,
            boost::asio::placeholders::error));
    }
    else
    {
      do_close();
    }
  }

  void do_write(chat_message msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      boost::asio::async_write(socket_,
          boost::asio::buffer(write_msgs_.front().data(),
            write_msgs_.front().length()),
          boost::bind(&chat_client::handle_write, this,
            boost::asio::placeholders::error));
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
            boost::bind(&chat_client::handle_write, this,
              boost::asio::placeholders::error));
      }
    }
    else
    {
      do_close();
    }
  }

  void do_close()
  {
    socket_.close();
  }

private:
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  int engineId;
  bool flag = false;
  bool flag_engine = false;
  response res;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: chat_client <host> <port>\n";
      return 1;
    }
    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(argv[1], argv[2]);
    tcp::resolver::iterator iterator = resolver.resolve(query);

    chat_client c(io_service, iterator);

    boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

    int message_id = 0;

    for(int i = 0; i < 1000; ++i){
      chat_message msg;
      Json::Value jval;

      c.wait_for_get_engine_id();
      int face_id = i;
      request res = new request(c.get_engine_Id(), message_id, SEARCH, face_id);
      message_id ++;
      jval = res.parse_request_to_json();
      // jval["engineId"] = c.get_engine_Id();
      // jval["queryName"] = SEARCH;
      // jval["contentVec"] = i;

      std::string lineSend = jval.toStyledString();

      msg.body_length(lineSend.length());
      memcpy(msg.body(), lineSend.c_str(), msg.body_length());
      msg.encode_header();
      c.write(msg);
      c.wait_for_response();

    }

    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
