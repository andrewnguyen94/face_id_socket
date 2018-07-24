#ifndef IODATA_H
#define IODATA_H

/*include
--------------------------------
--------------------------------*/
#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>

/*define, constants
--------------------------------
--------------------------------*/

using namespace std;

/*functions
--------------------------------
--------------------------------*/
class IOData
{
public:
	IOData()
	{

	}

	std::string GetCongfigData(std::string key);

private:
};

#endif