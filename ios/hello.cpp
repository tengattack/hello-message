
#include <stdio.h>
#include <sqlite3.h>
#include <cinttypes>
#include <vector>
#include <unistd.h>
#include <NFHTTP/NFHTTP.h>
#include <nlohmann/json.hpp>

int callback(void *, int, char **, char **);
typedef struct {
    int64_t message_id;
    nlohmann::json data;
} messagerow;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: hello [endpoint]\n");
        return 1;
    }

    int64_t last_message_id = 0;
    sqlite3 *db;
    sqlite3_stmt *res;

    int rc = sqlite3_open("/private/var/mobile/Library/SMS/sms.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return 1;
    }

    auto client = nativeformat::http::createClient(nativeformat::http::standardCacheLocation(),
                                               "NFHTTP-" + nativeformat::http::version());
    std::string url = argv[1];
    url += "/receive_message";

    while (true) {
        sleep(5);
    
        char *err_msg = NULL;
        std::string sql = "SELECT cm.message_id, m.text, m.type, m.service, m.date, m.date_delivered, m.destination_caller_id AS `to`, cm.chat_id, c.chat_identifier AS `from`"
            " FROM message m"
            " LEFT JOIN chat_message_join cm ON m.ROWID = cm.message_id"
            " LEFT JOIN chat c ON cm.chat_id = c.ROWID";
        if (last_message_id == 0) {
        	sql += " ORDER BY m.ROWID DESC"
                " LIMIT 1";
        } else {
            sql += " WHERE m.ROWID > " + std::to_string(last_message_id) +
        	    " ORDER BY m.ROWID DESC";
        }
        sql += ";";
        std::vector<messagerow> rows;
        rc = sqlite3_exec(db, sql.c_str(), callback, &rows, &err_msg);
        
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            
            sqlite3_free(err_msg);
            continue;
        }
    
        if (rows.size() == 0) {
            continue;
        }
        if (last_message_id == 0) {
            //last_message_id = rows[0].message_id;
            //continue;
        }
    
        nlohmann::json body = nlohmann::json::array();
        for (auto& m : rows) {
           body.push_back(m.data);
        }
    
        auto request = nativeformat::http::createRequest(url, std::unordered_map<std::string, std::string>({{"Content-Type", "application/json"}}));
        request->setMethod(nativeformat::http::PostMethod);
        request->setData((const unsigned char *)body.dump().c_str(), body.dump().length());
        auto response = client->performRequestSynchronously(request);

        if (response->statusCode() == 200) {
            size_t data_length = 0;
            const unsigned char *data = response->data(data_length);
            // handle data
            //body = nlohmann::json::parse(std::string((const char*)data, data_length));
            //printf("response body: %s\n", body.dump().c_str());
            last_message_id = rows[0].message_id;
        } else {
            fprintf(stderr, "http request error: status %d\n", response->statusCode());
        }
    }

    sqlite3_close(db);

    return 0;
}

int callback(void *userData, int argc, char **argv, 
                    char **azColName) {
    
    std::vector<messagerow> *rows = (std::vector<messagerow> *)userData;
    messagerow m;
    
    for (int i = 0; i < argc; i++) {
        if (strcmp(azColName[i], "message_id") == 0) {
            m.message_id = atoll(argv[i]);
        }
        m.data[azColName[i]] = argv[i];
    }
    
    rows->push_back(m);
    
    return 0;
}
