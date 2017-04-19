#ifndef __SLAVE_NANO_FIELD_H__
#define __SLAVE_NANO_FIELD_H__

#include <map>
#include <string>
#include <sstream>
#include <mysql/mysql.h>

namespace nanomysql
{
    struct field
    {
        const std::string name;
        const unsigned type;
        const unsigned long length;
        const unsigned flags;
        const unsigned decimals;
        const unsigned charsetnr;
        std::string data;
        bool is_null;

        field(
            const std::string& name_,
            const unsigned type_,
            const unsigned long length_,
            const unsigned flags_,
            const unsigned decimals_,
            const unsigned charsetnr_
        ) :
            name(name_),
            type(type_),
            length(length_),
            flags(flags_),
            decimals(decimals_),
            charsetnr(charsetnr_)
        {}

        template<typename T> inline operator T () const {
            T ret;
            std::istringstream s;
            s.str(data);
            s >> ret;
            return ret;
        }

        inline static field from_mysql_field(::MYSQL_FIELD* ff) {
            return field(
                ff->name,
                ff->type,
                ff->length,
                ff->flags,
                ff->decimals,
                ff->charsetnr
            );
        }
    };

    typedef std::map<std::string, field> fields_t;

}// nanomysql

#endif
// vim: et ts=4
