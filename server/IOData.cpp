#include "IOData.h"

std::string IOData::GetCongfigData(std::string key)
{
	std::string data;
	std::string line;
	std::string url;
	std::ifstream config;
	config.open("../data/Config.txt");
	while (std::getline(config, line))
	{
	data += line + "|";
	}
	config.close();
	std::size_t start = data.find(key);
	url = data.substr(start + key.length());
	std::size_t end = url.find("|");
	url = url.substr(0, end);
	return url;
}