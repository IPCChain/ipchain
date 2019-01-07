#ifndef __MESSAGE_CACHE_H__
#define __MESSAGE_CACHE_H__

#include <boost/bimap.hpp>
#include "uint256.h"

class MessageCache 
{
public:
	MessageCache (int cache_size);
	void Insert (uint256 val);
	bool Exist (uint256 val);

private:
	boost::bimap<uint256, unsigned int> m_map;
	unsigned int m_in;
	unsigned int m_out;
	const int m_size;
};

#endif /*__MESSAGE_CACHE_H__*/
