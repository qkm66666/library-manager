#include <crow.h>
#include <sqlite3.h>
#include <string>
#include <mutex>

// 全局互斥锁
std::mutex dbMutex;
// 全局 SQLite 连接
sqlite3* db = nullptr;

// 初始化数据库，创建表
bool initDatabase() {
    char* errMsg = nullptr;
    const char* createTableSQL =
        "CREATE TABLE IF NOT EXISTS books ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT,"
        "author TEXT,"
        "isbn TEXT,"
        "category TEXT,"
        "status TEXT,"
        "publishDate TEXT,"
        "description TEXT );";

    if (sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

int main() {
    crow::SimpleApp app;

    // 打开数据库
    if (sqlite3_open("library.db", &db) != SQLITE_OK) {
        return -1;
    }
    if (!initDatabase()) {
        return -1;
    }

    // 获取所有图书
    CROW_ROUTE(app, "/api/books").methods("GET"_method)([]() {
        std::lock_guard<std::mutex> lock(dbMutex);

        crow::json::wvalue booksJson;
        std::vector<crow::json::wvalue> booksArray;

        const char* selectSQL = "SELECT id, title, author, isbn, category, status, publishDate, description FROM books";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                crow::json::wvalue bookJson;
                bookJson["id"] = sqlite3_column_int(stmt, 0);
                bookJson["title"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
                bookJson["author"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
                bookJson["isbn"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
                bookJson["category"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
                bookJson["status"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
                bookJson["publishDate"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
                bookJson["description"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
                booksArray.push_back(std::move(bookJson));
            }
        }
        sqlite3_finalize(stmt);

        booksJson["books"] = std::move(booksArray);
        return crow::response(booksJson);
    });

    // 添加新图书（修正部分）
    CROW_ROUTE(app, "/api/books").methods("POST"_method)([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400);

        std::lock_guard<std::mutex> lock(dbMutex);
        const char* insertSQL =
            "INSERT INTO books (title, author, isbn, category, status, publishDate, description) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) == SQLITE_OK) {
            // 修正：使用 .s() 获取 std::string，再调用 c_str()，并指定字符串长度
            std::string title = body["title"].s();
            sqlite3_bind_text(stmt, 1, title.c_str(), title.length(), SQLITE_TRANSIENT);
            std::string author = body["author"].s();
            sqlite3_bind_text(stmt, 2, author.c_str(), author.length(), SQLITE_TRANSIENT);
            std::string isbn = body["isbn"].s();
            sqlite3_bind_text(stmt, 3, isbn.c_str(), isbn.length(), SQLITE_TRANSIENT);
            std::string category = body["category"].s();
            sqlite3_bind_text(stmt, 4, category.c_str(), category.length(), SQLITE_TRANSIENT);
            std::string status = body["status"].s();
            sqlite3_bind_text(stmt, 5, status.c_str(), status.length(), SQLITE_TRANSIENT);
            std::string publishDate = body["publishDate"].s();
            sqlite3_bind_text(stmt, 6, publishDate.c_str(), publishDate.length(), SQLITE_TRANSIENT);
            std::string description = body["description"].s();
            sqlite3_bind_text(stmt, 7, description.c_str(), description.length(), SQLITE_TRANSIENT);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return crow::response(201);
        }
        return crow::response(500);
    });

    // 编辑图书（修正部分）
    CROW_ROUTE(app, "/api/books/<int>").methods("PUT"_method)([](const crow::request& req, int id) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400);

        std::lock_guard<std::mutex> lock(dbMutex);
        const char* updateSQL =
            "UPDATE books SET title=?, author=?, isbn=?, category=?, status=?, publishDate=?, description=? "
            "WHERE id=?;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, updateSQL, -1, &stmt, nullptr) == SQLITE_OK) {
            std::string title = body["title"].s();
            sqlite3_bind_text(stmt, 1, title.c_str(), title.length(), SQLITE_TRANSIENT);
            std::string author = body["author"].s();
            sqlite3_bind_text(stmt, 2, author.c_str(), author.length(), SQLITE_TRANSIENT);
            std::string isbn = body["isbn"].s();
            sqlite3_bind_text(stmt, 3, isbn.c_str(), isbn.length(), SQLITE_TRANSIENT);
            std::string category = body["category"].s();
            sqlite3_bind_text(stmt, 4, category.c_str(), category.length(), SQLITE_TRANSIENT);
            std::string status = body["status"].s();
            sqlite3_bind_text(stmt, 5, status.c_str(), status.length(), SQLITE_TRANSIENT);
            std::string publishDate = body["publishDate"].s();
            sqlite3_bind_text(stmt, 6, publishDate.c_str(), publishDate.length(), SQLITE_TRANSIENT);
            std::string description = body["description"].s();
            sqlite3_bind_text(stmt, 7, description.c_str(), description.length(), SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 8, id);

            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE && sqlite3_changes(db) > 0) {
                return crow::response(200);
            }
            return crow::response(404);
        }
        return crow::response(500);
    });

    // 删除图书
    CROW_ROUTE(app, "/api/books/<int>").methods("DELETE"_method)([](int id) {
        std::lock_guard<std::mutex> lock(dbMutex);
        const char* deleteSQL = "DELETE FROM books WHERE id=?;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, deleteSQL, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, id);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE && sqlite3_changes(db) > 0) {
                return crow::response(200);
            }
            return crow::response(404);
        }
        return crow::response(500);
    });

    // 启动服务
    app.port(8080).multithreaded().run();

    // 关闭数据库
    sqlite3_close(db);
    return 0;
}