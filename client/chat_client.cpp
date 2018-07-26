#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/function.hpp>
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
  else if(name.compare("4") == 0) return SEND;
  else if(name.compare("5") == 0) return BRIGHTNESS;
  else if(name.compare("6") == 0) return PUSH_BRIGHTNESS;
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
      socket_(io_service),
      t(io_service)
  {
    boost::asio::async_connect(socket_, endpoint_iterator,
        boost::bind(&chat_client::handle_connect, this,
          boost::asio::placeholders::error));
  }

  void set_timer(boost::posix_time::ptime time, boost::function<void(const boost::system::error_code&)> handler)
  {
    t.expires_at(time);
    t.async_wait(handler);
  }

  void write(const chat_message& msg)
  {
    // set_timer(boost::posix_time::microsec_clock::universal_time() + boost::posix_time::seconds(1),
    //   boost::bind(&chat_client::do_write, this, msg, boost::asio::placeholders::error));
    io_service_.post(boost::bind(&chat_client::do_write, this, msg));
    // io_service_.run();
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
    wait_for_get_engine_id();
    int engineId = get_engine_Id();
    request *req = new request(engineId, message_id, SEND);
    req->set_content_img_vec(imgVector);
    req->set_number_of_image_request((int)(imgVector.size()));
    jval = req->parse_request_to_json();
    // delete[] req;
    std::string lineSend = jval.toStyledString();

    msg.body_length(lineSend.length());
    std::cout << "msg body_length : " << msg.body_length() << std::endl;
    memcpy(msg.body(), lineSend.c_str(), msg.body_length());
    msg.encode_header();
    write(msg);
    wait_for_response();
  }

  void get_brightness_from_server(int message_id)
  {
    chat_message msg;
    Json::Value jval;
    wait_for_get_engine_id();
    int engineId = get_engine_Id();
    request *req = new request(engineId, message_id, BRIGHTNESS);
    jval = req->parse_request_to_json();
    std::string lineSend = jval.toStyledString();
    msg.body_length(lineSend.length());
    memcpy(msg.body(), lineSend.c_str(), msg.body_length());
    msg.encode_header();
    write(msg);
    wait_for_response();
  }

  void push_brightness_to_server(int message_id, int brightness)
  {
    chat_message msg;
    Json::Value jval;
    wait_for_get_engine_id();
    int engineId = get_engine_Id();
    request *req = new request(engineId, message_id, PUSH_BRIGHTNESS);
    req->set_brightness(brightness);
    jval = req->parse_request_to_json();
    std::string lineSend = jval.toStyledString();
    msg.body_length(lineSend.length());
    memcpy(msg.body(), lineSend.c_str(), msg.body_length());
    msg.encode_header();
    write(msg);
    wait_for_response();
  }

  void search_closest_faces(int message_id, std::vector<float> content_search, int number_of_vectors_request)
  {
    chat_message msg;
    Json::Value jval;
    wait_for_get_engine_id();
    int engineId = get_engine_Id();
    request *req = new request(engineId, message_id, SEARCH);
    req->set_number_of_vectors_request(number_of_vectors_request);
    req->set_content_vec(content_search);
    jval = req->parse_request_to_json();
    std::string lineSend = jval.toStyledString();
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
    }else
    {
      exit(0);
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
          std::string id = v["id"].asString();
          std::string content_string = v["content"].asString();
          std::string name = v["name"].asString();
          std::stringstream ssv(content_string);
          float number;
          std::vector<float> content;
          while(ssv >> number)
          {
            content.push_back(number);
          }
          Pair *p = new Pair(id, content);
          p->set_face_name(name);
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
      else if(queryName == BRIGHTNESS)
      {
        int mes_id = jsonValue["mes_id"].asInt();
        int engine_id = jsonValue["engineId"].asInt();
        int contentResponse = jsonValue["responseContent"].asInt();
        resp->set_engine_id(engine_id);
        resp->set_mes_id(mes_id);
        resp->set_brightness(contentResponse);
        set_response(*resp);
        set_flag(true);
      }
      else if(queryName == PUSH_BRIGHTNESS)
      {
        int mes_id = jsonValue["mes_id"].asInt();
        int engine_id = jsonValue["engineId"].asInt();
        std::string contentResponse = jsonValue["responseContent"].asString();
        resp->set_engine_id(engine_id);
        resp->set_mes_id(mes_id);
        resp->set_content(contentResponse);
        std::cout << "contentResponse :" << contentResponse << std::endl;
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
  boost::asio::deadline_timer t;
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
    // boost::asio::basic_deadline_timer t{io_service};

    chat_client c(io_service, iterator);

    boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

    int message_id = 0;
    std::vector<float> v_req;
    std::string vector_str = "-0.12588 0.145605 0.0612512 0.021806 -0.0539667 -0.0167877 -0.0422762 -0.136958 0.156892 -0.0498365 0.242933 -0.0189076 -0.205557 -0.139949 0.00934789 0.0982862 -0.204675 -0.176596 -0.00246833 -0.0230572 0.150514 -0.0162264 0.0575154 0.00912343 -0.240553 -0.326662 -0.0786249 -0.146404 0.00839708 -0.0305699 -0.0846974 -0.0371725 -0.237279 -0.115487 0.00491643 0.0386038 0.0127023 -0.0545972 0.183566 -0.0418008 -0.175711 -0.0118175 0.0508087 0.257534 0.138337 0.0299526 0.0282778 -0.10691 0.111127 -0.124791 0.0955359 0.169931 0.0911321 0.0144956 0.067949 -0.0991931 0.0229642 0.0917953 -0.1718 0.043487 0.0793465 -0.0046594 -0.0278172 -0.0457467 0.173969 0.0735607 -0.0223107 -0.20668 0.101049 -0.0881511 -0.0907413 0.0849444 -0.183887 -0.191998 -0.333641 -0.00940656 0.403201 0.0561405 -0.212871 0.0479829 -0.0624894 -0.0344558 0.186586 0.0768432 0.00817938 0.0551326 -0.175503 0.0667851 0.231384 -0.0710716 -0.0763211 0.169907 0.0238278 -0.0220806 0.0717712 0.0385832 -0.0961652 0.0660534 -0.174015 -0.0369121 0.0962858 0.0413359 -0.0463808 0.130266 -0.132869 -0.0016307 0.0453584 -0.0234381 -0.0342149 -0.064769 -0.165772 -0.112658 0.0856344 -0.215222 0.149355 0.136477 0.000701869 0.0892593 0.0473727 0.0154274 -0.00157596 0.0242948 -0.196107 -0.034731 0.0962107 0.0205367 0.0882704 -0.00928565";
    float f;
    std::stringstream vector_stream(vector_str);
    while(vector_stream >> f)
    {
      v_req.push_back(f); 
    }
    std::cout << "v_req" << v_req[2] << std::endl;
    std::vector<cv::Mat> src;
    cv::Mat img = imread("/home/anhnt/Pictures/test.png", CV_LOAD_IMAGE_COLOR);
    src.push_back(img);
    // c.push_image_to_server(src, "test", message_id);
    // c.get_brightness_from_server(message_id);
    c.search_closest_faces(message_id, v_req, 10);
    response res = c.get_response();
    std::vector<Pair> search_content = res.get_result_res();
    for(size_t i = 0; i < search_content.size(); ++i){
      std::string faceid = search_content[i].get_face_id();
      std::string faceid_name = search_content[i].get_face_name();
      std::cout << "faceid : " << faceid << std::endl;
      std::cout << "faceid_name : " << faceid_name <<std::endl;
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
