/*
Site:		http://hlspx.ocry.com/mysqlconnestion/

History:
			VERSION
			1.0.0.0
Author:
		Alexey Tretyakov	hlspx@mail.ru
*/

#pragma once

#include <mysql/mysql.h>
#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include "db/mysql/TmDateTime.h"
#include "db/mysql/MysqlConnConfig.h"

#pragma GCC diagnostic ignored "-Wconversion"


namespace SqlConn
{

    enum class MySqlDbType
    {
        //
// Summary:
//     Kiff.Data.MySqlClient.MySqlDbType.Decimal
//     A fixed precision and scale numeric value between -1038 -1 and 10 38 -1.
        //Decimal = 0,
        //
        // Summary:
        //     Kiff.Data.MySqlClient.MySqlDbType.Byte
        //     The signed range is -128 to 127. The unsigned range is 0 to 255.
        Byte = 1,
        //
        // Summary:
        //     Kiff.Data.MySqlClient.MySqlDbType.Int16
        //     A 16-bit signed integer. The signed range is -32768 to 32767. The unsigned range
        //     is 0 to 65535
        Int16 = 2,
        //
        // Summary:
        //     Kiff.Data.MySqlClient.MySqlDbType.Int32
        //     A 32-bit signed integer
        Int32 = 3,
        //
        // Summary:
        //     System.Single
        //     A small (single-precision) floating-point number. Allowable values are -3.402823466E+38
        //     to -1.175494351E-38, 0, and 1.175494351E-38 to 3.402823466E+38.
        Float = 4,
        //
        // Summary:
        //     Kiff.Data.MySqlClient.MySqlDbType.Double
        //     A normal-size (double-precision) floating-point number. Allowable values are
        //     -1.7976931348623157E+308 to -2.2250738585072014E-308, 0, and 2.2250738585072014E-308
        //     to 1.7976931348623157E+308.
        Double = 5,
        //
        // Summary:
        //     A timestamp. The range is '1970-01-01 00:00:00' to sometime in the year 2037
        //Timestamp = 7,
        //
        // Summary:
        //     Kiff.Data.MySqlClient.MySqlDbType.Int64
        //     A 64-bit signed integer.
        Int64 = 8,
        //
        // Summary:
        //     Specifies a 24 (3 byte) signed or unsigned value.
        //Int24 = 9,
        //
        // Summary:
        //     Date The supported range is '1000-01-01' to '9999-12-31'.
        //Date = 10,
        //
        // Summary:
        //     Time
        //     The range is '-838:59:59' to '838:59:59'.
        //Time = 11,
        //
        // Summary:
        //     DateTime The supported range is '1000-01-01 00:00:00' to '9999-12-31 23:59:59'.
        DateTime = 12,
        //
        // Summary:
        //     A year in 2- or 4-digit format (default is 4-digit). The allowable values are
        //     1901 to 2155, 0000 in the 4-digit year format, and 1970-2069 if you use the 2-digit
        //     format (70-69).
        //Year = 13,
        //
        // Summary:
        //     Obsolete Use Datetime or Date type
        //Newdate = 14,
        //
        // Summary:
        //     A variable-length string containing 0 to 65535 characters
        VarString = 15,
        //
        // Summary:
        //     Bit-field data type
        //Bit = 16,
        //
        // Summary:
        //     JSON
        //JSON = 245,
        //
        // Summary:
        //     New Decimal
        //NewDecimal = 246,
        //
        // Summary:
        //     An enumeration. A string object that can have only one value, chosen from the
        //     list of values 'value1', 'value2', ..., NULL or the special "" error value. An
        //     ENUM can have a maximum of 65535 distinct values
        //Enum = 247,
        //
        // Summary:
        //     A set. A string object that can have zero or more values, each of which must
        //     be chosen from the list of values 'value1', 'value2', ... A SET can have a maximum
        //     of 64 members.
        //Set = 248,
        //
        // Summary:
        //     A binary column with a maximum length of 255 (2^8 - 1) characters
        TinyBlob = 249,
        //
        // Summary:
        //     A binary column with a maximum length of 16777215 (2^24 - 1) bytes.
        MediumBlob = 250,
        //
        // Summary:
        //     A binary column with a maximum length of 4294967295 or 4G (2^32 - 1) bytes.
        LongBlob = 251,
        //
        // Summary:
        //     A binary column with a maximum length of 65535 (2^16 - 1) bytes.
        Blob = 252,
        //
        // Summary:
        //     A variable-length string containing 0 to 255 bytes.
        VarChar = 253,
        //
        // Summary:
        //     A fixed-length string.
        String = 254,
        //
        // Summary:
        //     Geometric (GIS) data type.
        //Geometry = 255,
        //
        // Summary:
        //     Unsigned 8-bit value.
        UByte = Byte | 0x200,
        //
        // Summary:
        //     Unsigned 16-bit value.
        UInt16 = Int16 | 0x200,
        //
        // Summary:
        //     Unsigned 32-bit value.
        UInt32 = Int32 | 0x200,
        //
        // Summary:
        //     Unsigned 64-bit value.
        UInt64 = Int64 | 0x200,
        //
        // Summary:
        //     Unsigned 24-bit value.
        //UInt24 = 509,

        Unspecified = -1
    };

    ////////////////////////////////////////////////////////////
    class DataStore
    {
        friend class MySqlDataReader;
        friend class MySqlCommand;

        DataStore(const DataStore&) {}
    protected:
        void *buffer = nullptr;			//buffer to get/put data
        unsigned long buffer_length = 0;
        unsigned long length = 0;       /* output length  */
        MySqlDbType buffer_type = MySqlDbType::Unspecified;		//
        bool is_null = 0;			/* Pointer to null indicator */
        bool error = 0;				/* set this if you want to track data truncations happened during fetch */

        void Init(MYSQL_FIELD &field, MYSQL_BIND &resbind);
        DataStore() {}
        ~DataStore() {
            if (buffer != nullptr) free(buffer);
        }
    };

    class MySqlCommand;

    class MySqlDataReader
    {
        friend class MySqlConnection;
        friend class MySqlCommand;

        MYSQL_STMT *smnt;
        MYSQL_BIND *resultBind = nullptr;		// output
        DataStore *results;					// real results
        uint32_t fieldCount = 0;
        MySqlDataReader(const MySqlDataReader&) {}

        template<typename T>
        void GetRefValue(uint32_t pos, T& value) const
        {
            value = GetFieldValue<T>(pos);
        }

        void GetRefValues(uint32_t) const {}

        template<typename T, typename... Targs>
        void GetRefValues(uint32_t pos, T&& val, Targs&& ... Fargs) const
        {
            GetRefValue(pos, val);
            GetRefValues(++pos, Fargs...);
        }

        uint32_t PosFromName(const std::string &name) const;

    protected:
        MySqlDataReader(MYSQL_STMT *ismnt);
        MySqlCommand *rdCmd = nullptr;
    public:
        ~MySqlDataReader();
        bool Read();

        bool IsNull(uint32_t pos) const
        {
            if (pos >= fieldCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in IsNull");
            return results[pos].is_null;
        }

        bool IsNull(const std::string &name) const
        {
            return IsNull(PosFromName(name));
        }

        template<typename T>
        T GetFieldValue(uint32_t pos) const
        {
            if (pos >= fieldCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in GetFieldValue");
            if (results[pos].is_null) throw std::runtime_error("Field '" + std::to_string(pos) + "' is NULL");
            return *reinterpret_cast<T*>(results[pos].buffer);
        }

        template<typename T>
        T GetFieldValue(const std::string &name) const
        {
            return GetFieldValue<T>(PosFromName(name));
        }

        void GetFieldValue(uint32_t pos, void **obuf, uint32_t *olen) const
        {
            if (pos >= fieldCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in GetFieldValue");
            if (results[pos].is_null) throw std::runtime_error("Field '" + std::to_string(pos) + "' is NULL");
            *obuf = results[pos].buffer;
            *olen = results[pos].length;
        }

        template<typename... Targs>
        void GetValues(Targs&& ... Fargs) const
        {
            GetRefValues(0, Fargs...);
        }
    };

    template<>
    void MySqlDataReader::GetRefValue<std::string>(uint32_t pos, std::string& value) const;
    template<>
    void MySqlDataReader::GetRefValue<std::vector<uint8_t>>(uint32_t pos, std::vector<uint8_t>& value) const;

    template<>
    std::string MySqlDataReader::GetFieldValue<std::string>(uint32_t pos) const;
    template<>
    TmDateTime MySqlDataReader::GetFieldValue<TmDateTime>(uint32_t pos) const;
    template<>
    std::vector<uint8_t> MySqlDataReader::GetFieldValue<std::vector<uint8_t>>(uint32_t pos) const;

    //////////////////////////////////////////////////////////////
    class MySqlCommand
    {
        friend class MySqlConnection;

        MYSQL_STMT *smnt = nullptr;
        MYSQL_BIND *paramBind = nullptr;		// input
        DataStore *bindings;					// real data
        uint32_t paramCount;
        MySqlCommand(const MySqlCommand&) {}
        void Execute();

        template<typename T>
        MySqlDbType Typ2My() const
        {
            throw std::runtime_error("Typ2My : unknown type");
        }

        void SetValues(uint32_t) {}

        template<typename T, typename... Targs>
        void SetValues(uint32_t pos, T&& val, Targs&& ... Fargs)
        {
            SetValue(pos, val);
            SetValues(++pos, Fargs...);
        }

    protected:
        MySqlCommand(MYSQL *con, const char *query);

    public:

        ~MySqlCommand();

        void SetNull(uint32_t pos)
        {
            if (pos >= paramCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in SetNull");
            bindings[pos].is_null = true;
        }

        // bind  value
        void BindParam(uint32_t pos, MySqlDbType type);

        // default SetValue - size=8
        template<typename T>
        void SetValue(uint32_t pos, const T& value)
        {
            if (pos >= paramCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in SetValue");
            if (bindings[pos].buffer_type == MySqlDbType::Unspecified)
                BindParam(pos, Typ2My<T>());
            SetValue(pos, &value, (uint32_t)sizeof(T));
        }

        template<size_t N>
        void SetValue(uint32_t pos, const char(&value)[N])
        {
            SetValue(pos, std::string(value));
        }

        void SetValue(uint32_t pos, const void *value, size_t length);

        template<typename... Targs>
        void BindParams(Targs&& ... Fargs)
        {
            SetValues(0, Fargs...);
        }

        size_t ExecuteNonQuery();

        template<typename... Targs>
        size_t ExecuteNonQuery(Targs&& ... Fargs)
        {
            SetValues(0, Fargs...);
            return ExecuteNonQuery();
        }

        MySqlDataReader *ExecuteReader()
        {
            Execute();
            return new MySqlDataReader(smnt);
        }

        template<typename... Targs>
        MySqlDataReader *ExecuteReader(Targs&& ... Fargs)
        {
            SetValues(0, Fargs...);
            return ExecuteReader();
        }

        void Cancel() { if (mysql_stmt_free_result(smnt)) throw std::runtime_error(mysql_stmt_error(smnt)); }
    };

    template<> inline MySqlDbType MySqlCommand::Typ2My<bool>()   const { return MySqlDbType::UByte; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<int8_t>()   const { return MySqlDbType::Byte; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<uint8_t>()  const { return MySqlDbType::UByte; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<int16_t>()  const { return MySqlDbType::Int16; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<uint16_t>() const { return MySqlDbType::UInt16; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<int32_t>()  const { return MySqlDbType::Int32; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<uint32_t>() const { return MySqlDbType::UInt32; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<int64_t>()  const { return MySqlDbType::Int64; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<uint64_t>() const { return MySqlDbType::UInt64; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<float>()    const { return MySqlDbType::Float; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<double>()   const { return MySqlDbType::Double; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<TmDateTime>() const { return MySqlDbType::DateTime; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<std::string>() const { return MySqlDbType::VarChar; }
    template<> inline MySqlDbType MySqlCommand::Typ2My<std::vector<uint8_t>>() const { return MySqlDbType::LongBlob; }


    template<>
    inline void MySqlCommand::SetValue(uint32_t pos, const nullptr_t& )
    {
        if (pos >= paramCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in SetValue");
        SetNull(pos);
    }

    template<>
    inline void MySqlCommand::SetValue(uint32_t pos, const std::string &value)
    {
        if (pos >= paramCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in SetValue");
        if (bindings[pos].buffer_type == MySqlDbType::Unspecified)
            BindParam(pos, MySqlDbType::VarChar);
        SetValue(pos, reinterpret_cast<const void*>(value.data()), value.length());
    }

    template<>
    inline void MySqlCommand::SetValue(uint32_t pos, const char* const  &value )
    {
        SetValue(pos, std::string(value));
    }

    template<>
    inline void MySqlCommand::SetValue(uint32_t pos, const std::vector<uint8_t> &value)
    {
        if (pos >= paramCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in SetValue");
        if (bindings[pos].buffer_type == MySqlDbType::Unspecified)
            BindParam(pos, MySqlDbType::LongBlob);
        SetValue(pos, reinterpret_cast<const void*>(value.data()), value.size());
    }

    template<>
    void MySqlCommand::SetValue(uint32_t pos, const TmDateTime& value);

    /////////////////////////////////////////////////////////////////////////
    class MySqlConnection
    {
        MySqlConnection(const MySqlConnection&) {}
        MYSQL *mysql = nullptr;
        clock_t aliveTime;    //记录进入空闲状态后的起始时间点
    public:

        MySqlConnection();
        virtual ~MySqlConnection();

        bool connect(const std::string& ip, const std::string& user, const std::string& password, const std::string& dbName,
                    unsigned int port = 3306);
        
        bool connect(const MysqlConnConfig& config);

        virtual void Close() { throw std::runtime_error("Close is not supported"); }

        void refreshAliveTime() { aliveTime = clock(); }
        clock_t getAliveTime() const { return clock() - aliveTime; }   // 返回存活的时间

        inline void Ping()
        {
            if (mysql_ping(mysql)) throw std::runtime_error("mysql_ping");
        }

        MySqlCommand *CreateCommand(const std::string &query) { return new MySqlCommand(mysql, query.c_str()); }

        size_t ExecuteNonQuery(const std::string &query);

        template<typename... Targs>
        size_t ExecuteNonQuery(const std::string &query, Targs&& ... Fargs)
        {
            MySqlCommand *cmd = CreateCommand(query);
            cmd->BindParams(Fargs...);
            size_t affRws = cmd->ExecuteNonQuery();
            delete cmd;
            return affRws;
        }

        MySqlDataReader *ExecuteReader(const std::string &query);

        template<typename... Targs>
        MySqlDataReader *ExecuteReader(const std::string &query, Targs&& ... Fargs)
        {
            MySqlCommand *cmd = CreateCommand(query);
            cmd->BindParams(Fargs...);
            MySqlDataReader *rd = cmd->ExecuteReader();
            rd->rdCmd = cmd;
            return rd;
        }

        virtual void ChangeDatabase(const std::string &dbname);
    };
}
