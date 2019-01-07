#include <iostream>

#include "message_cache.h"

MessageCache::MessageCache (int cache_size):m_size(cache_size),m_out(0),m_in(0)
{
	m_map.clear ();
}

void MessageCache::Insert (uint256 val)
{
	m_map.insert ({val, m_in});
	m_in++;

	if (m_map.size() >= m_size)
	{
		/*
		auto it = m_map.right.find(m_out);

		if (it != m_map.right.end ())
		{
			m_map.erase (it);
			m_out ++;
		}
		*/

		auto it = m_map.right.find(m_out);

		m_map.right.erase (it);
		//m_map.left.erase (it);
		/*
		m_map.right.erase (it);
		m_map.left.erase (m_map.left.count(it.left));
		*/
		m_out ++;

	}
	//std::cout << m_map.size() << std::endl;
}

bool MessageCache::Exist (uint256 val)
{
	auto it = m_map.left.find(val);

	if (it != m_map.left.end ())
		return true;
	else
		return false;
}


