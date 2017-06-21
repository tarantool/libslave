/* Copyright 2011 ZAO "Begun".
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __NANOMYSQL_H
#define __NANOMYSQL_H

#include <stdexcept>
#include <vector>
#include <mysql/mysql.h>
#include "nanofield.h"

namespace nanomysql {

struct mysql_conn_opts
{
    std::string mysql_host;
    int         mysql_port              = 3306;
    std::string mysql_db;
    std::string mysql_user;
    std::string mysql_pass;
    std::string mysql_ssl_ca;
    std::string mysql_ssl_cert;
    std::string mysql_ssl_key;
    unsigned int mysql_connect_timeout  = 10;
    unsigned int mysql_read_timeout     = 60 * 15;
    unsigned int mysql_write_timeout    = 60 * 15;
};

class Connection {

    MYSQL* m_conn;

    void throw_error(std::string msg, const std::string& m2 = "")
    {

        msg += ": ";
        msg += ::mysql_error(m_conn);
        msg += " : ";

        char n[32];
        ::snprintf(n, 31, "%d", ::mysql_errno(m_conn));
        msg += n;

        if (m2.size() > 0) {
            msg += " : [";
            msg += m2;
            msg += "]";
        }

        throw std::runtime_error(msg);
    }

    struct _mysql_res_wrap {
        MYSQL_RES* s;
        _mysql_res_wrap(MYSQL_RES* _s) : s(_s) {}
        ~_mysql_res_wrap() { if (s != NULL) ::mysql_free_result(s); }
    };

    void connect(const mysql_conn_opts& opts)
    {
        m_conn = ::mysql_init(NULL);

        if (!m_conn)
            throw std::runtime_error("Could not mysql_init()");

        setOptions(m_conn, opts);

        if (::mysql_real_connect(m_conn
                               , opts.mysql_host.c_str()
                               , opts.mysql_user.c_str()
                               , opts.mysql_pass.c_str()
                               , opts.mysql_db.c_str()
                               , opts.mysql_port, NULL, 0
                                ) == NULL)
        {
            throw_error("Could not mysql_real_connect()");
        }
    }

    static inline fields_t::iterator push_field(fields_t& field, const ::MYSQL_FIELD* ff) {
        return field.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(ff->name, ff->name_length),
            std::forward_as_tuple(
                std::string(ff->name, ff->name_length),
                ff->type,
                ff->length,
                ff->flags,
                ff->decimals
            )
        ).first;
    }

public:
    static void setOptions(MYSQL* connection, const mysql_conn_opts& opts)
    {
        const unsigned int connect_timeout = opts.mysql_connect_timeout;
        const unsigned int read_timeout = opts.mysql_read_timeout;
        const unsigned int write_timeout = opts.mysql_write_timeout;
        if (connect_timeout > 0)
        {
            mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout);
        }
        /* Timeout for reads from server (works only for TCP/IP connections, and only for Windows prior to MySQL 4.1.22).
         * You can this option so that a lost connection can be detected earlier than the TCP/IP
         * Close_Wait_Timeout value of 10 minutes. Added in 4.1.1.
         */
        if (read_timeout > 0)
        {
            mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &read_timeout);
        }
        if (write_timeout > 0)
        {
            mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &write_timeout);
        }

        mysql_ssl_set( connection
                     , opts.mysql_ssl_key.empty() ? nullptr : opts.mysql_ssl_key.c_str()
                     , opts.mysql_ssl_cert.empty() ? nullptr : opts.mysql_ssl_cert.c_str()
                     , opts.mysql_ssl_ca.empty() ? nullptr : opts.mysql_ssl_ca.c_str()
                     , nullptr
                     , nullptr
                     );
    }

    Connection(const mysql_conn_opts& opts)
    {
        connect(opts);
    }

    ~Connection()
    {
        ::mysql_close(m_conn);
    }

    void select_db(const std::string& db_name)
    {
        if (::mysql_select_db(m_conn, db_name.c_str()))
            throw_error("mysql_select_db() failed");
    }

    void get_fields(const std::string& tbl_name, fields_t& fields)
    {
        _mysql_res_wrap re(::mysql_list_fields(m_conn, tbl_name.c_str(), NULL));
        if (re.s == NULL) {
            throw_error("mysql_list_fields() failed");
        }

        ::MYSQL_FIELD* ff = ::mysql_fetch_fields(re.s);
        for (unsigned cnt = ::mysql_field_count(m_conn); cnt; --cnt, ++ff) {
            push_field(fields, ff);
        }
    }

    void query(const std::string& q)
    {
        if (::mysql_real_query(m_conn, q.data(), q.size()) != 0)
            throw_error("mysql_query() failed", q);
    }

    template <typename F>
    void use(F f)
    {
        _mysql_res_wrap re(::mysql_use_result(m_conn));

        if (re.s == NULL) {
            throw_error("mysql_use_result() failed");
        }

        const size_t num_fields = ::mysql_num_fields(re.s);

        fields_t fields;
        std::vector<fields_t::iterator> fields_n;

        ::MYSQL_FIELD* ff;
        while (ff = ::mysql_fetch_field(re.s)) {
            fields_n.push_back(
                push_field(fields, ff)
            );
        }

        while (1) {
            MYSQL_ROW row = ::mysql_fetch_row(re.s);

            if (row == NULL) {
                if (::mysql_errno(m_conn) != 0) {
                    throw_error("mysql_fetch_row() failed");
                }

                break;
            }

            const unsigned long* lens = ::mysql_fetch_lengths(re.s);

            for (size_t z = 0; z != num_fields; ++z) {
                fields_n[z]->second.data.assign(row[z], lens[z]);
                fields_n[z]->second.is_null = row[z] == 0;
            }

            f(fields);
        }
    }

    typedef std::vector<fields_t> result_t;

    void store(result_t& out) {
        use([&] (const fields_t& row) { out.push_back(row); });
    }
};

}

#endif
// vim: et ts=4
