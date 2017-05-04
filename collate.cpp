#include <string>
#include <mysql/mysql.h>
#include "collate.h"

using namespace slave;

collate_map_t slave::readCollateMap(nanomysql::Connection& conn)
{
	collate_map_t res;
	nanomysql::Connection::result_t nanores;
	std::unordered_map<std::string, unsigned> charsets;

	conn.query("SHOW CHARACTER SET");
	conn.store(nanores);

	for (const auto& row : nanores)
	{
		const std::string& name = row.at("Charset").data;
		const unsigned maxlen = atoi(row.at("Maxlen").data.c_str());

		charsets.emplace(name, maxlen);
	}

	nanores.clear();
	conn.query("SHOW COLLATION");
	conn.store(nanores);

	for (const auto& row : nanores)
	{
		const unsigned charsetnr = atoi(row.at("Id").data.c_str());
		const std::string& name = row.at("Collation").data;
		const std::string& charset = row.at("Charset").data;

		const unsigned maxlen = charsets.at(charset);

		res.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(charsetnr),
			std::forward_as_tuple(name, charset, maxlen)
		);
	}

	return res;
}
