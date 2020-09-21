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

#ifndef __SLAVE_FIELD_H_
#define __SLAVE_FIELD_H_

#include <string>
#include <vector>
#include <boost/any.hpp>

#include <mysql/my_time.h>
#include <mysql/m_string.h>
#include <mysql/binary_log_funcs.h>

extern "C" {
#include <mysql/decimal.h>
}

#undef min
#undef max
#undef test

// -------------------------------------------------------------------------------------------------

namespace slave {

// ----- base --------------------------------------------------------------------------------------

class Field
{
	public:
		const std::string field_name;
		boost::any field_data;

		Field(const std::string& name) : field_name(name) {}
		virtual ~Field() {}

		virtual const char* unpack(const char *from) = 0;
		virtual void unpack_str(const std::string &from) { field_data = from; }
};

// ----- numbers -----------------------------------------------------------------------------------

template<typename T, const unsigned length = sizeof(T)> class Field_num : public Field {
	using Field::Field;

	public:
		const char* unpack(const char* from);
		void unpack_str(const std::string &from);

	private:
		inline T get_value(const char *from);
};

template class Field_num<uint16, 1>;
template class Field_num<uint16>;
template class Field_num<uint32, 3>;
template class Field_num<uint32>;
template class Field_num<ulonglong>;

template class Field_num<int16, 1>;
template class Field_num<int16>;
template class Field_num<int32, 3>;
template class Field_num<int32>;
template class Field_num<longlong>;

template class Field_num<float>;
template class Field_num<double>;


class Field_decimal : public Field {
	public:
		Field_decimal(const std::string& name, const unsigned length_, const unsigned scale_, const bool is_unsigned)
			: Field(name), scale(scale_),
			// see my_decimal_length_to_precision() @ my_decimal.h
			precision(length_ - (scale_ ? 1 : 0) - (is_unsigned || !length_ ? 0 : 1)),
			length(::decimal_bin_size(precision, scale_))
		{}

		const char* unpack(const char *from);

	private:
		const unsigned scale, precision, length;
		static const bool zerofill = false;
};

// ----- date & time -------------------------------------------------------------------------------

class Field_temporal : public Field {
	public:
		Field_temporal(const std::string& name, const unsigned precision_)
			: Field(name), precision(precision_) {}

		// set length
		virtual void reset(const bool is_old_storage_, const bool ctor_call = false) = 0;

	protected:
		bool is_old_storage;
		const unsigned precision;
		unsigned length;
};


class Field_timestamp : public Field_temporal {
	public:
		Field_timestamp(const std::string& name, const unsigned precision_, const bool is_old_storage_)
			: Field_temporal(name, precision_) { reset(is_old_storage_, true); }

		const char* unpack(const char* from);
		void reset(const bool is_old_storage_, const bool ctor_call);
};


class Field_time : public Field_temporal {
	public:
		Field_time(const std::string& name, const unsigned precision_, const bool is_old_storage_)
			: Field_temporal(name, precision_) { reset(is_old_storage_, true); }

		const char* unpack(const char* from);
		void reset(const bool is_old_storage_, const bool ctor_call);
};


class Field_datetime : public Field_temporal {
	public:
		Field_datetime(const std::string& name, const unsigned precision_, const bool is_old_storage_)
			: Field_temporal(name, precision_) { reset(is_old_storage_, true); }

		const char* unpack(const char* from);
		void reset(const bool is_old_storage_, const bool ctor_call);
};


class Field_date : public Field {
	using Field::Field;
	public:
		const char* unpack(const char* from);
};


class Field_year : public Field {
	using Field::Field;
	public:
		const char* unpack(const char* from);
		void unpack_str(const std::string &from);
};

// ----- string ------------------------------------------------------------------------------------

class Field_string : public Field {
	public:
		Field_string(const std::string& name, const unsigned length_, const unsigned maxlen)
			: Field(name), length(length_ * maxlen) {}

		const char* unpack(const char* from);

	private:
		const unsigned length;
};

// ----- enums -------------------------------------------------------------------------------------

class Field_bitset : public Field {
	public:
		Field_bitset(const std::string& name, const std::string& type);

	protected:
		unsigned length;
		std::vector<std::string> str_values;
};

class Field_enum : public Field_bitset {
	public:
		Field_enum(const std::string& name, const std::string& type);
		const char* unpack(const char* from);
};

class Field_set: public Field_bitset {
	public:
		Field_set(const std::string& name, const std::string& type);
		const char* unpack(const char* from);
};


class Field_bit : public Field {
	public:
		Field_bit(const std::string& name, const unsigned length_) : Field(name), length(length_) {}
		const char* unpack(const char *from);
		void unpack_str(const std::string &from);

	private:
		const unsigned length;
};

// ----- blob --------------------------------------------------------------------------------------

class Field_blob : public Field {
	public:
		Field_blob(const std::string& name, const unsigned length);
		const char* unpack(const char* from);

	private:
		unsigned type;
};
// -------------------------------------------------------------------------------------------------
}
#endif // __SLAVE_FIELD_H_
