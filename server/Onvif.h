#ifndef ONVIFCONTROLLER_H
#define ONVIFCONTROLLER_H

#include <iostream>
#include <string>
#include <curl/curl.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>

using namespace std;

class OnvifController
{
  private:
    std::string cameraToken = "MainStream";
    std::string camUrl = "";
    std::string getToken();
    std::string post(string content);
  public:
    OnvifController(string ipAndPort)
    {
        camUrl = "http://" + ipAndPort + "/onvif/device_service";
    }
    int getBrightness();
    int getExposureTime();
    int getColorSaturation();
    int getContrast();
    void setContrast(int level, string videoSourceToken);
    void setBrightness(int level, string videoSourceToken);
    void setExposureTime(int level, string videoSourceToken);
    void setColorSaturation(int level, string videoSourceToken);
};

#endif /* ONVIFCONTROLLER_H */