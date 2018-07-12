#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
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
      std::string queryName = jsonValue["queryName"].asString();
      if (queryName == "engineID"){
        set_engine_Id(jsonValue["engineID"].asInt());
        set_flag_engine(true);
        std::cout << "This engine is assinged to ID: " <<get_engine_Id()<< std::endl;
      }
      else if (queryName == "search"){
        std::cout << "content Vector is :" << jsonValue["contentVec"] << std::endl;
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

    char line[chat_message::max_body_length + 1];
    for(int i = 0; i < 100000; ++i){
      chat_message msg;
      Json::Value jval;

      while(c.get_flag_engine() != true){
        //do nothing
      }
      jval["engineId"] = c.get_engine_Id();
      jval["queryName"] = "search";
      jval["contentVec"] = i;

      std::string lineSend = jval.toStyledString();

      msg.body_length(lineSend.length());
      memcpy(msg.body(), lineSend.c_str(), msg.body_length());
      msg.encode_header();
      c.write(msg);
      while(c.get_flag() != true){
        //do nothting
      }
      c.set_flag(false);
    }
    // while (std::cin.getline(line, chat_message::max_body_length + 1))
    // {
    //   using namespace std; // For strlen and memcpy.
    //   chat_message msg;

    //   //json
    //   Json::Value jval;
    //   jval["engineID"] = c.get_engine_Id();
    //   jval["queryName"] = "search";
    //   jval["contentVec"] = "0.1 0.2 0.3";
    //   std::string lineSend = jval.toStyledString();
    //   //

    //   msg.body_length(lineSend.length());
    //   memcpy(msg.body(), lineSend.c_str(), msg.body_length());
    //   msg.encode_header();
    //   c.write(msg);
    // }

    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
