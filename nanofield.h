#ifndef __SLAVE_NANO_FIELD_H__
#define __SLAVE_NANO_FIELD_H__

#include <map>
#include <string>
#include <sstream>

namespace nanomysql
{
    struct field
    {
        std::string name;
        unsigned int type;
        unsigned long length;
        unsigned int flags;
        unsigned int decimals;
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
    };

    typedef std::map<std::string, field> fields_t;
}// nanomysql

#endif
// vim: et ts=4
