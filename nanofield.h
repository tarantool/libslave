#ifndef __SLAVE_NANO_FIELD_H__
#define __SLAVE_NANO_FIELD_H__

#include <map>
#include <string>
#include <sstream>
#include <mysql/mysql.h>

namespace nanomysql
{
    inline static std::string field_name(const ::MYSQL_FIELD* ff) {
        return std::string(ff->name, ff->name_length);
    }

    struct field
    {
        const std::string name;
        const unsigned type;
        const unsigned long length;
        const unsigned flags;
        const unsigned decimals;
        std::string data;
        bool is_null;

        field(const std::string& n, unsigned int t, unsigned long l, unsigned int f, unsigned int d)
            : name(n), type(t), length(l), flags(f), decimals(d) {}

        template <typename T>
        operator T() const
        {
            T ret;
            std::istringstream s;
            s.str(data);
            s >> ret;
            return ret;
        }

        inline static field from_mysql_field(const ::MYSQL_FIELD* ff) {
            return field(
                field_name(ff),
                ff->type,
                ff->length,
                ff->flags,
                ff->decimals
            );
        }
    };

    typedef std::map<std::string, field> fields_t;
}// nanomysql

#endif
// vim: et ts=4
