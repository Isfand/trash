#pragma once
#ifndef ERASE_HXX
#define ERASE_HXX

#include <string>
#include <vector>

#include "erase_args.hxx"

namespace trash{

class Erase
{
public:
	Erase(std::vector<std::string>& args, EraseOptions& eOpt);
private:
	EraseOptions& m_eOpt;
	void file(std::vector<std::string>& args);
	void allFile();
	void past();
	void previous();
	void sqlInjection();
};
	
} //namespace trash

#endif //ERASE_HXX 
