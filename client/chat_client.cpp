#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <sstream>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "jsoncpp/json/json.h"
#include "../chat_message.hpp"

using boost::asio::ip::tcp;
using namespace cv;

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
  if(name.compare("1") == 0) return SEARCH;
  else if(name.compare("0") == 0) return ENGINEID;
  else if(name.compare("2") == 0) return ADD;
  else if(name.compare("3") == 0) return DELETE;
  else if(name.compare("4" )== 0) return SEND;
  else{
    std::cout << "query name not found!" << std::endl;
    exit(0);
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

  void set_response(response resp)
  {
    res = resp;
  } 

  response get_response()
  {
    return res;
  }

  void push_image_to_server(std::vector<cv::Mat> imgVector, std::string key, int message_id)
  {
    chat_message msg;
    Json::Value jval;
    unsigned char *src = (unsigned char *)imgVector.data();
    wait_for_get_engine_id();
    int engineId = get_engine_Id();
    request *req = new request(engineId, message_id, SEND);
    req->set_content_img_vec(imgVector);
    req->set_number_of_image_request((int)(imgVector.size()));
    jval = req->parse_request_to_json();

    std::string lineSend = jval.toStyledString();
    std::string tmp = jval["contentVec"]["image_0"]["content"].asString();
    char *tmp_char = new char[tmp.length() + 1];
    strcpy(tmp_char, tmp.c_str());
    tmp_char[tmp.length()] = '\0';
    // std::cout << "2" <<std::endl;
    int h = jval["contentVec"]["image_0"]["height"].asInt();
    int w = jval["contentVec"]["image_0"]["width"].asInt();
    int t = jval["contentVec"]["image_0"]["type"].asInt();
    cv::Mat tmp_mat = cv::Mat(h,w, t,
      tmp_char);

    // std::cout << "3" <<std::endl;
    namedWindow("test", WINDOW_AUTOSIZE);
    imshow("test", tmp_mat);
    waitKey(0);
    destroyAllWindows();
    // std::cout << "4" <<std::endl;

    msg.body_length(lineSend.length());
    memcpy(msg.body(), lineSend.c_str(), msg.body_length());
    msg.encode_header();
    write(msg);
    wait_for_response();
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
      response *resp = new response();
      std::string queryName_str = jsonValue["queryName"].asString();
      QUERYNAME queryName = get_query_name_from_string(queryName_str);
      if (queryName == ENGINEID){
        set_engine_Id(jsonValue["engineId"].asInt());
        resp->set_engine_id(jsonValue["engineId"].asInt());
        set_response(*resp);
        set_flag_engine(true);
        std::cout << "This engine is assinged to ID: " <<get_engine_Id()<< std::endl;
      }
      else if (queryName == SEARCH){
        int face_id = jsonValue["face_id"].asInt();
        int mes_id = jsonValue["mes_id"].asInt();
        int engine_id = jsonValue["engineId"].asInt();
        int number_of_vectors_request = jsonValue["nov"].asInt();
        Json::Value contentResponse = jsonValue["responseContent"];
        
        std::string key = "Pair";
        std::vector<Pair> vector_p;
        for(int i = 0; i < number_of_vectors_request; ++i){
          std::stringstream ss;
          std::ostringstream ssi;
          ssi << i;
          ss << key << "_" << ssi.str();
          std::string key_el = ss.str();
          Json::Value v = contentResponse[key_el];
          int id = v["id"].asInt();
          std::string content_string = v["content"].asString();
          std::stringstream ssv(content_string);
          float number;
          std::vector<float> content;
          while(ssv >> number)
          {
            content.push_back(number);
          }
          Pair *p = new Pair(id, content);
          vector_p.push_back(*p);

        }
        resp->set_mes_id(mes_id);
        resp->set_engine_id(engine_id);
        resp->set_query_name(queryName);
        resp->set_result_res(vector_p);
        set_response(*resp);
        set_flag(true);
      }
      else if(queryName == ADD){
        int mes_id = jsonValue["mes_id"].asInt();
        int engine_id = jsonValue["engineId"].asInt();
        int face_id = jsonValue["face_id"].asInt();
        std::string contentResponse = jsonValue["responseContent"].asString();
        resp->set_engine_id(engine_id);
        resp->set_mes_id(mes_id);
        resp->set_content(contentResponse);
        resp->set_face_id(face_id);
        set_response(*resp);
        set_flag(true);
      }
      else if(queryName == DELETE){
        int mes_id = jsonValue["mes_id"].asInt();
        int engine_id = jsonValue["engineId"].asInt();
        int face_id = jsonValue["face_id"].asInt();
        std:string contentResponse = jsonValue["responseContent"].asString();

        resp->set_engine_id(engine_id);
        resp->set_mes_id(mes_id);
        resp->set_content(contentResponse);
        resp->set_face_id(face_id);
        set_response(*resp);
        set_flag(true);
      }
      else if(queryName == SEND)
      {
        int mes_id = jsonValue["mes_id"].asInt();
        int engine_id = jsonValue["engineId"].asInt();
        std::string contentResponse = jsonValue["responseContent"].asString();
        resp->set_engine_id(engine_id);
        resp->set_mes_id(mes_id);
        resp->set_content(contentResponse);
        set_response(*resp);
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
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
  }

  void handle_write(const boost::system::error_code& error, size_t bytes_transferred)
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
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred));
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
    /*fix host here
    example:


    */

    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(argv[1], argv[2]);
    tcp::resolver::iterator iterator = resolver.resolve(query);

    chat_client c(io_service, iterator);

    boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

    int message_id = 0;
    std::vector<float> v_req;
    for(int i = 0; i < 128; ++i){
      float ele = (float) i;
      v_req.push_back(ele);
    }
    std::vector<cv::Mat> src;
    cv::Mat img = imread("/home/anhnt/Pictures/test.png", CV_LOAD_IMAGE_COLOR);
    src.push_back(img);
    c.push_image_to_server(src, "test", message_id);

    // for(int i = 0; i < 1000; ++i){
    //   chat_message msg;
    //   Json::Value jval;
      
    //   c.wait_for_get_engine_id();
    //   request *req = new request(c.get_engine_Id(), message_id, SEARCH);
    //   req->set_number_of_vectors_request(10);
    //   req->set_content_vec(v_req);
    //   message_id ++;
    //   jval = req->parse_request_to_json();

    //   std::string lineSend = jval.toStyledString();

    //   msg.body_length(lineSend.length());
    //   memcpy(msg.body(), lineSend.c_str(), msg.body_length());
    //   std::cout << "length" << msg.body_length() << std::endl;
    //   msg.encode_header();
    //   c.write(msg);
    //   c.wait_for_response();
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
