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

#include <cstdio>
#include <stdexcept>
#include <sstream>

#include "field.h"
#include "Logging.h"

// -------------------------------------------------------------------------------------------------

namespace slave {

// ----- numbers -----------------------------------------------------------------------------------

template<> uint16 Field_num<uint16, 1>::get_value(const char *from) {
	return *(const uchar*)from;
}
template<> uint16 Field_num<uint16>::get_value(const char *from) {
	return uint2korr(from);
}
template<> uint32 Field_num<uint32, 3>::get_value(const char *from) {
	return uint3korr(from);
}
template<> uint32 Field_num<uint32>::get_value(const char *from) {
	return uint4korr(from);
}
template<> ulonglong Field_num<ulonglong>::get_value(const char *from) {
	return uint8korr(from);
}

template<> int16 Field_num<int16, 1>::get_value(const char *from) {
	return *from;
}
template<> int16 Field_num<int16>::get_value(const char *from) {
	return sint2korr(from);
}
template<> int32 Field_num<int32, 3>::get_value(const char *from) {
	return sint3korr(from);
}
template<> int32 Field_num<int32>::get_value(const char *from) {
	return sint4korr(from);
}
template<> longlong Field_num<longlong>::get_value(const char *from) {
	return sint8korr(from);
}

template<> float Field_num<float>::get_value(const char *from) {
	return *(float*)from;
}
template<> double Field_num<double>::get_value(const char *from) {
	return *(double*)from;
}

template<typename T, const unsigned length> const char* Field_num<T, length>::unpack(const char* from)
{
	T value = get_value(from);

	LOG_TRACE(log, "  num size " << length << ": " << value);

	field_data = value;
	return from + length;
}

template<typename T, const unsigned length> void Field_num<T, length>::unpack_str(const std::string &from)
{
	if (from.size()) {
		T value;
		std::istringstream s(from);
		s >> value;
		field_data = value;
	} else {
		field_data = boost::any();
	}
}


Field_decimal::Field_decimal(const std::string& name, const unsigned length_, const unsigned scale_, const bool is_unsigned)
	: Field(name), scale(scale_),
	// see my_decimal_length_to_precision() @ my_decimal.h
	precision(length_ - (scale_ ? 1 : 0) - (is_unsigned || !length_ ? 0 : 1)),
	length(::decimal_bin_size(precision, scale_)) {}

const char* Field_decimal::unpack(const char *from)
{
	::decimal_digit_t buf[ precision ];
	::decimal_t dec;
	dec.len = precision;
	dec.buf = buf;

	if (::bin2decimal((const uchar*)from, &dec, precision, scale) != E_DEC_OK) {
		throw std::runtime_error("Field_decimal::unpack(): bin2decimal() failed");
	}

	int value_length = decimal_string_size(&dec);
	char buffer[ value_length ];

	if (::decimal2string(&dec, (char*)&buffer, &value_length, zerofill ? precision : 0, scale, '0') != E_DEC_OK) {
		throw std::runtime_error("Field_decimal::unpack(): decimal2string() failed");
	}

	std::string value((char*)buffer, value_length);
	field_data = std::move(value);

	return from + length;
}

// ----- date & time -------------------------------------------------------------------------------

void Field_timestamp::reset(bool is_old_storage_, bool ctor_call)
{
	if (!ctor_call && is_old_storage == is_old_storage_) {
		return;
	}
	if ((is_old_storage = is_old_storage_)) {
		length = 4;
	} else {
		length = ::my_timestamp_binary_length(precision);
	}
}
void Field_timestamp::sec_to_TIME(::MYSQL_TIME& my_time, const struct ::timeval& tv)
{
	if (tv.tv_sec) {
		// see localtime_to_TIME() @ sql_time.cc
		struct ::tm tm_;
		time_t ts = tv.tv_sec;
		::localtime_r(&ts, &tm_);

		my_time.neg = 0;
		my_time.second_part = tv.tv_usec;
		my_time.year = (tm_.tm_year + 1900) % 10000;
		my_time.month = tm_.tm_mon + 1;
		my_time.day = tm_.tm_mday;
		my_time.hour = tm_.tm_hour;
		my_time.minute = tm_.tm_min;
		my_time.second = std::min(tm_.tm_sec, 59); // see adjust_leap_second() @ tztime.cc
	} else {
		::memset(&my_time, 0, sizeof(my_time));
	}
	my_time.time_type = MYSQL_TIMESTAMP_DATETIME;
}
const char* Field_timestamp::unpack(const char* from)
{
	::MYSQL_TIME my_time;
	struct ::timeval tv;

	if (is_old_storage) {
		tv.tv_sec = uint4korr(from);
		tv.tv_usec = 0;
	} else {
		::my_timestamp_from_binary(&tv, (const uchar*)from, precision);
	}

	sec_to_TIME(my_time, tv);
	char buffer[ MAX_DATE_STRING_REP_LENGTH ];
	int value_length = ::my_TIME_to_str(&my_time, (char*)&buffer, precision);

	std::string value((char*)&buffer, value_length);

	LOG_TRACE(log, "  timestamp: '" << value << "' // " << length);

	field_data = std::move(value);
	return from + length;
}


void Field_time::reset(bool is_old_storage_, bool ctor_call)
{
	if (!ctor_call && is_old_storage == is_old_storage_) {
		return;
	}
	if ((is_old_storage = is_old_storage_)) {
		length = 3;
	} else {
		length = ::my_time_binary_length(precision);
	}
}
const char* Field_time::unpack(const char* from)
{
	// !! we ignore fractional part
	// 3 bytes + fractional-seconds storage, big endian
	// ---------------------
	//  1 bit sign    (1= non-negative, 0= negative)
	//  1 bit unused  (reserved for future extensions)
	// 10 bits hour   (0-838)
	//  6 bits minute (0-59)
	//  6 bits second (0-59)
	// ---------------------
	// 24 bits = 3 bytes

	::MYSQL_TIME my_time = {0};

	if (is_old_storage) {
		longlong nr = uint3korr(from);
		int warnings;
		if (::number_to_time(nr, &my_time, &warnings) || warnings) {
			throw std::runtime_error("Field_time::unpack(): number_to_time() failed");
		}
	} else {
		longlong nr = ::my_time_packed_from_binary((const uchar*)from, precision);
		::TIME_from_longlong_time_packed(&my_time, nr);
	}

	char buffer[ MAX_DATE_STRING_REP_LENGTH ];
	int value_length = ::my_TIME_to_str(&my_time, (char*)&buffer, precision);

	std::string value((char*)&buffer, value_length);

	LOG_TRACE(log, "  time: '" << value << "' // " << length);

	field_data = std::move(value);
	return from + length;
}


void Field_datetime::reset(bool is_old_storage_, bool ctor_call)
{
	if (!ctor_call && is_old_storage == is_old_storage_) {
		return;
	}
	if ((is_old_storage = is_old_storage_)) {
		length = 8;
	} else {
		length = ::my_datetime_binary_length(precision);
	}
}
const char* Field_datetime::unpack(const char* from)
{
	// !! we ignore fractional part
	// 5 bytes + fractional-seconds storage, big endian
	// ---------------------------
	//  1 bit  sign           (1= non-negative, 0= negative)
	// 17 bits year*13+month  (year 0-9999, month 0-12)
	//  5 bits day            (0-31)
	//  5 bits hour           (0-23)
	//  6 bits minute         (0-59)
	//  6 bits second         (0-59)
	// ---------------------------
	// 40 bits = 5 bytes

	::MYSQL_TIME my_time = {0};

	if (is_old_storage) {
		longlong nr = uint8korr(from);
		int warnings;
		if (::number_to_datetime(nr, &my_time, TIME_FUZZY_DATE, &warnings) == -1 || warnings) {
			throw std::runtime_error("Field_datetime::unpack(): number_to_datetime() failed");
		}
	} else {
		longlong nr = ::my_datetime_packed_from_binary((const uchar*)from, precision);
		::TIME_from_longlong_datetime_packed(&my_time, nr);
	}

	char buffer[ MAX_DATE_STRING_REP_LENGTH ];
	int value_length = ::my_TIME_to_str(&my_time, (char*)&buffer, precision);

	std::string value((char*)&buffer, value_length);

	LOG_TRACE(log, "  datetime: '" << value << "' // " << length);

	field_data = std::move(value);
	return from + length;
}


const char* Field_date::unpack(const char* from)
{
	::MYSQL_TIME my_time = {0};

	// see Field_newdate::get_date_internal() @ field.cc
	longlong nr = uint3korr(from);
	my_time.day = nr & 31;
	my_time.month = (nr >> 5) & 15;
	my_time.year = (nr >> 9);
	my_time.time_type = MYSQL_TIMESTAMP_DATE;

	char buffer[ MAX_DATE_STRING_REP_LENGTH ];
	int value_length = ::my_TIME_to_str(&my_time, (char*)&buffer, 0);

	std::string value((char*)&buffer, value_length);

	LOG_TRACE(log, "  date: '" << value << "'");

	field_data = std::move(value);
	return from + 3;
}


const char* Field_year::unpack(const char* from)
{
	uint16 value = *(const uchar*)from + 1900;
	field_data = value;

	LOG_TRACE(log, "  year: " << value);
	return from + 1;
}
void Field_year::unpack_str(const std::string &from)
{
	if (from.size()) {
		uint16 value;
		std::istringstream s(from);
		s >> value;
		field_data = value;
	} else {
		field_data = boost::any();
	}
}

// ----- string ------------------------------------------------------------------------------------

Field_string::Field_string(const std::string& name, const unsigned length_, const collate_info& collate)
	: Field(name), length(length_ * collate.maxlen) {}

const char* Field_string::unpack(const char* from)
{
	size_t value_length;
	// see calc_pack_length() @ field.cc
	if (length < 256) {
		value_length = *(const uchar*)from;
		from ++;
	} else {
		value_length = uint2korr(from);
		from += 2;
	}

	std::string value(from, value_length);

	LOG_TRACE(log, "  string size " << length << ": '" << value << "' // " << value_length);

	field_data = std::move(value);
	return from + value_length;
}

// ----- enums -------------------------------------------------------------------------------------

Field_bitset::Field_bitset(const std::string& name, const std::string& type) : Field(name)
{
	char v[ type.size() ], *pv;
	const char *pt = (const char*)type.c_str(), *const pe = pt + type.size();
	bool instr = false;

	while (pt < pe)
	if (instr) {
		if (*pt == '\'') {
			pt++;
			if (*pt++ == '\'') {
				*pv++ = '\'';
			} else {
				instr = false;
				str_values.emplace_back((char*)v, pv - (char*)v);
			}
		} else {
			*pv++ = *pt++;
		}
	} else if (*pt++ == '\'') {
		instr = true;
		pv = (char*)v;
	}
}


Field_enum::Field_enum(const std::string& name, const std::string& type) : Field_bitset(name, type)
{
	// see get_enum_pack_length() @ field.h
	length = str_values.size() < 256 ? 1 : 2;
}
const char* Field_enum::unpack(const char* from)
{
	ulonglong nr = length == 1 ? *(const uchar*)from : uint2korr(from);

	std::string value;
	if (nr) value.assign(str_values.at(nr - 1));

	LOG_TRACE(log, "  enum size " << length << ": " << value);

	field_data = std::move(value);
	return from + length;
}


Field_set::Field_set(const std::string& name, const std::string& type) : Field_bitset(name, type)
{
	// see get_set_pack_length() @ field.h
	const unsigned len = (str_values.size() + 7) / 8;
	length = len > 4 ? 8 : len;
}
const char* Field_set::unpack(const char* from)
{
	ulonglong nr = 0;

	switch (length) {
		case 1: nr = *(const uchar*)from; break;
		case 2: nr = uint2korr(from); break;
		case 3: nr = uint3korr(from); break;
		case 4: nr = uint4korr(from); break;
		case 8: nr = uint8korr(from); break;
	}

	std::string value;

	if (nr) {
		for (unsigned bit_n = 0; nr; nr >>= 1, ++bit_n)
		if (nr & 1) {
			value += str_values.at(bit_n);
			value += ',';
		}
		value.pop_back();
	}

	LOG_TRACE(log, "  set size " << length << ": " << value);

	field_data = std::move(value);
	return from + length;
}


const char* Field_bit::unpack(const char *from)
{
	unsigned long long value = 0;

	for (unsigned length_ = (length + 7) / 8; length_; --length_, ++from) {
		value = (value << 8) | *from;
	}

	LOG_TRACE(log, "  bit size " << length << ": " << value);

	field_data = value;
	return from;
}
void Field_bit::unpack_str(const std::string &from)
{
	unsigned long long value = 0;

	for (const auto b : from) {
		value = (value << 8) | b;
	}

	field_data = value;
}

// ----- blob --------------------------------------------------------------------------------------

Field_blob::Field_blob(const std::string& name, const unsigned length) : Field(name)
{
	// see get_blob_type_from_length() @ field.cc
	if (length < 256)
		type = 1;
	else if (length < 65536)
		type = 2;
	else if (length < 256L*256L*256L)
		type = 3;
	else
		type = 4;
}
const char* Field_blob::unpack(const char* from)
{
	size_t value_length;

	switch (type) {
		case 1: value_length = *(const uchar*)from++; break;
		case 2: value_length = uint2korr(from); from += 2; break;
		case 3: value_length = uint3korr(from); from += 3; break;
		case 4: value_length = uint4korr(from); from += 4; break;
	}

	std::string value(from, value_length);

	LOG_TRACE(log, "  blob size " << type << ": '" << value << "' // " << value_length);

	field_data = std::move(value);
	return from + value_length;
}
// -------------------------------------------------------------------------------------------------
} // namespace slave
