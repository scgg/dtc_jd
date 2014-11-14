#ifndef DATA_CENTER_H
#define DATA_CENTER_H

#include <map>
#include "pod.h"

typedef std::map<int,unsigned>::iterator time_iterator;
typedef std::map<int,MSG>::iterator do_iterator;

class Curve
{
public:
	int ProcessData(const MSG& data);
private:
	bool Expire(long last, long now);
private:
	std::map<int,unsigned int> time_map;
	std::map<int,MSG> do_map;
};

#endif
