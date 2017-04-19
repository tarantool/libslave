#ifndef __SLAVE_COLLATE_H
#define __SLAVE_COLLATE_H

#include <unordered_map>
#include <string>
#include "nanomysql.h"

namespace slave
{
	struct collate_info
	{
		const std::string name;
		const std::string charset;
		const unsigned maxlen;

		collate_info(
			const std::string& name_,
			const std::string& charset_,
			const unsigned maxlen_
		) :
			name(name_),
			charset(charset_),
			maxlen(maxlen_)
		{}
	};

	typedef std::unordered_map<unsigned, collate_info> collate_map_t;

	collate_map_t readCollateMap(nanomysql::Connection& conn);
}// slave

#endif
