#include "db/mysql/MysqlConnection.h"
#include <regex>

#ifdef _WIN32
#pragma comment(lib, "mariadbclient.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#endif

namespace SqlConn
{

    const char* ws = " \t\n\r\f\v";

    // trim from end of string (right)
    inline std::string& rtrim(std::string& s, const char* t = ws)
    {
        s.erase(s.find_last_not_of(t) + 1);
        return s;
    }

    // trim from beginning of string (left)
    inline std::string& ltrim(std::string& s, const char* t = ws)
    {
        s.erase(0, s.find_first_not_of(t));
        return s;
    }

    // trim from both ends of string (right then left)
    inline std::string& trim(std::string& s, const char* t = ws)
    {
        return ltrim(rtrim(s, t), t);
    }


    MySqlConnection::MySqlConnection()
    {

        if (!(mysql = mysql_init(NULL))) throw std::runtime_error("can't init Kiff");

    }

    bool MySqlConnection::connect(const std::string& ip, const std::string& user, const std::string& password, const std::string& dbName,
                                    unsigned int port) {
        MYSQL* p = mysql_real_connect(mysql, ip.c_str(), user.c_str(), password.c_str(),
                                      dbName.c_str(), port, nullptr, 0);
        if(p == nullptr) {
            return false;
        }
        return true;
    }

    bool MySqlConnection::connect(const MysqlConnConfig& config)
    {
        return connect(config.ip, config.username, config.password, config.dbname, config.port);
    }


    MySqlConnection::~MySqlConnection()
    {
        if (mysql == nullptr) return;
        mysql_close(mysql);
    }

    size_t MySqlConnection::ExecuteNonQuery(const std::string &query)
    {
        if (mysql_query(mysql, query.c_str()))
            throw std::runtime_error(std::string(query).append(" mysql_query : ").append(mysql_error(mysql)));

        size_t affRws = 0;
        MYSQL_RES *result = nullptr;

        do
        {
            result = mysql_store_result(mysql);
            if (result)
            {
                affRws += (size_t)mysql_num_rows(result);
                mysql_free_result(result);
                result = nullptr;
            }
            else
            {
                affRws += (size_t)mysql_affected_rows(mysql);
            }
        } while (!mysql_next_result(mysql));

        return affRws;
    }

    MySqlDataReader *MySqlConnection::ExecuteReader(const std::string & query)
    {
        MySqlCommand *cmd = CreateCommand(query);
        MySqlDataReader *rd = cmd->ExecuteReader();
        rd->rdCmd = cmd;
        return rd;
    }

    void MySqlConnection::ChangeDatabase(const std::string &db)
    {
        if (mysql_select_db(mysql, db.c_str()))
            throw std::runtime_error(std::string(db).append(" mysql_select_db : ").append(mysql_error(mysql)));
    }

    ///////////////////////////////////////////
    MySqlCommand::MySqlCommand(MYSQL * con, const char *query)
    {
        if (!(smnt = mysql_stmt_init(con)))
            throw std::runtime_error("can't init smnt");
        if (mysql_stmt_prepare(smnt, query, static_cast<unsigned long>(strlen(query))))
            throw std::runtime_error(std::string(query).append(" MYSQL_STMT : ").append(mysql_stmt_error(smnt)));

        paramCount = mysql_stmt_param_count(smnt);
        if (paramCount > 0)
        {
            paramBind = new MYSQL_BIND[paramCount];
            memset(paramBind, 0, sizeof(MYSQL_BIND) * paramCount);
            bindings = new DataStore[paramCount];

            for (uint32_t pos = 0; pos < paramCount; pos++)
            {
                paramBind[pos].length = &bindings[pos].length;

                *((bool**)&paramBind[pos].is_null) = &bindings[pos].is_null;
                bindings[pos].buffer_type = MySqlDbType::Unspecified;
            }
        }
    }

    MySqlCommand::~MySqlCommand()
    {
        if (smnt != nullptr)
        {
            mysql_stmt_free_result(smnt);
            mysql_stmt_close(smnt);
        }
        if (paramBind != nullptr)
        {
            delete[] paramBind;
            delete[] bindings;
        }
    }

    // bind
    void MySqlCommand::BindParam(uint32_t pos, MySqlDbType type)
    {
        if (pos >= paramCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in BindParam");

        bindings[pos].buffer_type = type;

        enum_field_types mysqlType = (enum_field_types)((int)type & 0xff);
        bool is_unsigned = ((int)type & 0x200) != 0;

        paramBind[pos].buffer_type = mysqlType;
        paramBind[pos].is_unsigned = is_unsigned;
    }

    void MySqlCommand::SetValue(uint32_t pos, const void * value, size_t length)
    {
        if (pos >= paramCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in SetValue");

        if (bindings[pos].buffer_type == MySqlDbType::Unspecified)
            BindParam(pos, MySqlDbType::Blob);

        size_t bufLen = (length < 8) ? 8 : length;
        if ((bindings[pos].buffer != nullptr) && (bindings[pos].buffer_length < bufLen))
        {
            free(bindings[pos].buffer);
            bindings[pos].buffer = nullptr;
        }

        if (bindings[pos].buffer == nullptr)
        {
            bindings[pos].buffer = malloc(bufLen);
            bindings[pos].buffer_length = (unsigned long)bufLen;

            paramBind[pos].buffer = bindings[pos].buffer;
            paramBind[pos].buffer_length = bindings[pos].buffer_length;
        }
        if (bufLen == 8) memset(bindings[pos].buffer, 0, 8);					// malloc || ���� ����� ������ � MySqlDbType �� ���������(���� int8_t-> Int64) (?)

        bindings[pos].length = static_cast<unsigned long>(length);				// ��� ���� < 8 �� 8 ?
        memcpy(bindings[pos].buffer, value, length);

        bindings[pos].is_null = false;
    }

    size_t MySqlCommand::ExecuteNonQuery()
    {
        Execute();

        size_t affRws = 0;

        do
        {
            if (mysql_stmt_store_result(smnt)) throw std::runtime_error("ExecuteNonQuery : mysql_stmt_store_result failed");

            size_t nrws = (size_t)mysql_stmt_num_rows(smnt);
            if (nrws > 0) affRws += nrws;
            else
            {
                nrws = (size_t)mysql_stmt_affected_rows(smnt);
                affRws += nrws;
            }
        } while (!mysql_stmt_next_result(smnt));

        return affRws;
    }

    template<>
    void MySqlCommand::SetValue(uint32_t pos, const TmDateTime& value)
    {
        if (pos >= paramCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in SetValue");

        if (bindings[pos].buffer_type == MySqlDbType::Unspecified)
            BindParam(pos, MySqlDbType::DateTime);
        tm intim = value.ToTm();

        MYSQL_TIME mtim;

        uint64_t mks = (std::abs(value.Ticks()) % 1000000000LL) / 1000;

        mtim.year = intim.tm_year + 1900;
        mtim.month = intim.tm_mon + 1;
        mtim.day = intim.tm_mday;
        mtim.hour = intim.tm_hour;
        mtim.minute = intim.tm_min;
        mtim.second = intim.tm_sec;
        mtim.second_part = (unsigned long)mks;
        mtim.time_type = enum_mysql_timestamp_type::MYSQL_TIMESTAMP_DATETIME;

        SetValue(pos, &mtim, sizeof(MYSQL_TIME));
    }

    void MySqlCommand::Execute()
    {
        if (paramCount != 0)
        {
            for (uint32_t i = 0; i < paramCount; i++)
            {
                if ((bindings[i].buffer_type == MySqlDbType::Unspecified) && (bindings[i].is_null == false))
                    throw std::runtime_error("Unspecified parametr in MySqlCommand");
            }
            if (mysql_stmt_bind_param(smnt, paramBind)) throw std::runtime_error(std::string("mysql_stmt_bind_param : ").append(mysql_stmt_error(smnt)));
        }
        if (mysql_stmt_execute(smnt)) throw std::runtime_error(std::string("mysql_stmt_execute : ").append(mysql_stmt_error(smnt)));
    }

    uint32_t MySqlDataReader::PosFromName(const std::string &name) const
    {
        MYSQL_FIELD* fld = smnt->fields;
        for (uint32_t i = 0; i < smnt->field_count; i++)
        {
            if (name.compare(0, name.length(), fld->name, fld->name_length) == 0) return i;
        }
        throw std::runtime_error("Field '" + name + "' not found");
    }

    //////////////////////////////////////////////
    MySqlDataReader::MySqlDataReader(MYSQL_STMT * istmt)
            :smnt(istmt)
    {
        fieldCount = mysql_stmt_field_count(smnt);
        if (fieldCount > 0)
        {
            MYSQL_RES *meta_result = mysql_stmt_result_metadata(smnt);
            resultBind = new MYSQL_BIND[fieldCount];
            memset(resultBind, 0, sizeof(MYSQL_BIND) * fieldCount);
            results = new DataStore[fieldCount];
            for (uint32_t i = 0; i < fieldCount; i++)
            {
                results[i].Init(meta_result->fields[i], resultBind[i]);
            }
            mysql_free_result(meta_result);

            if (mysql_stmt_bind_result(smnt, resultBind)) throw std::runtime_error(mysql_stmt_error(smnt));

            if (mysql_stmt_store_result(smnt)) throw std::runtime_error(mysql_stmt_error(smnt));
        }
    }

    MySqlDataReader::~MySqlDataReader()
    {
        mysql_stmt_free_result(smnt);
        if (resultBind != nullptr)
        {
            delete[] resultBind;
            delete[] results;
        }
        if (rdCmd != nullptr) delete rdCmd;
    }

    bool MySqlDataReader::Read()
    {
        if (fieldCount == 0) return false;

        int rc = mysql_stmt_fetch(smnt);
        if (rc == 0) return true;
        if (rc != MYSQL_NO_DATA) throw std::runtime_error(mysql_stmt_error(smnt));
        return false;
    }

    template<>
    std::string MySqlDataReader::GetFieldValue<std::string>(uint32_t pos) const
    {
        if (pos >= fieldCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in GetFieldValue");
        if (results[pos].is_null) throw std::runtime_error(std::string("Field '").append(std::to_string(pos)).append("' is NULL"));
        return std::string(reinterpret_cast<const char*>(results[pos].buffer), results[pos].length);
    }

    template<>
    void MySqlDataReader::GetRefValue<std::string>(uint32_t pos, std::string& value) const
    {
        if (pos >= fieldCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in GetValues");
        if (results[pos].is_null) throw std::runtime_error(std::string("Field '").append(std::to_string(pos)).append("' is NULL"));
        value.assign(reinterpret_cast<const char*>(results[pos].buffer), results[pos].length);
    }

    template<>
    std::vector<uint8_t> MySqlDataReader::GetFieldValue<std::vector<uint8_t>>(uint32_t pos) const
    {
        if (pos >= fieldCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in GetFieldValue");
        if (results[pos].is_null) throw std::runtime_error(std::string("Field '").append(std::to_string(pos)).append("' is NULL"));
        return std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(results[pos].buffer), reinterpret_cast<const uint8_t*>(results[pos].buffer) + results[pos].length);
    }

    template<>
    void MySqlDataReader::GetRefValue<std::vector<uint8_t>>(uint32_t pos, std::vector<uint8_t>& value) const
    {
        if (pos >= fieldCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in GetValues");
        if (results[pos].is_null) throw std::runtime_error(std::string("Field '").append(std::to_string(pos)).append("' is NULL"));
        value.assign(reinterpret_cast<const uint8_t*>(results[pos].buffer), reinterpret_cast<const uint8_t*>(results[pos].buffer) + results[pos].length);
    }

    template<>
    TmDateTime MySqlDataReader::GetFieldValue<TmDateTime>(uint32_t pos) const
    {
        if (pos >= fieldCount)	throw std::runtime_error("MySqlCommand:: Wrong param index '" + std::to_string(pos) + "' in GetFieldValue");
        const MYSQL_TIME *sqtm = reinterpret_cast<const MYSQL_TIME*>(results[pos].buffer);
        uint32_t mls = sqtm->second_part / 1000;
        uint32_t mks = sqtm->second_part % 1000;
        return TmDateTime (sqtm->year, sqtm->month, sqtm->day, sqtm->hour, sqtm->minute, sqtm->second, mls, mks, 0);
    }

    void DataStore::Init(MYSQL_FIELD &field, MYSQL_BIND &resbind)
    {
        uint32_t bufLen = 0;
        enum_field_types bufferType;

        switch (field.type)
        {
            case enum_field_types::MYSQL_TYPE_DATE:
            case enum_field_types::MYSQL_TYPE_TIME:
            case enum_field_types::MYSQL_TYPE_DATETIME:
            case enum_field_types::MYSQL_TYPE_YEAR:
            case enum_field_types::MYSQL_TYPE_NEWDATE:
                bufferType = enum_field_types::MYSQL_TYPE_DATETIME;
                bufLen = sizeof(MYSQL_TIME);
                break;
            case enum_field_types::MYSQL_TYPE_VARCHAR:
            case enum_field_types::MYSQL_TYPE_TINY_BLOB:
            case enum_field_types::MYSQL_TYPE_MEDIUM_BLOB:
            case enum_field_types::MYSQL_TYPE_LONG_BLOB:
            case enum_field_types::MYSQL_TYPE_BLOB:
            case enum_field_types::MYSQL_TYPE_VAR_STRING:
            case enum_field_types::MYSQL_TYPE_STRING:
            case enum_field_types::MYSQL_TYPE_GEOMETRY:
                bufferType = field.type;
                bufLen = field.length;
                break;
            default:
                bufferType = field.type;
                bufLen = 8;
                break;
        }

        if (bufLen > 0xffffff) bufLen = 0xffffff;

        buffer = malloc(bufLen);
        buffer_length = bufLen;
        resbind.buffer = buffer;
        resbind.buffer_length = bufLen;
        resbind.buffer_type = bufferType;
        resbind.length = &length;
        *((bool**)&resbind.is_null) = &is_null;
        *((bool**)&resbind.error) = &error;
    }
}