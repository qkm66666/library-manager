#include <crow.h>
#include <sqlite3.h>
#include <string>
#include <mutex>
#include <iostream>
#include <locale>


struct CORSHandler {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context&) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        
        // 处理 OPTIONS 请求，直接返回 204
        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            res.end();
        }
    }

    void after_handle(crow::request&, crow::response& res, context&) {
        // 确保所有响应都包含 CORS 头
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }
};

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

// 添加CORS头部的函数
void addCORSHeaders(crow::response& res) {

}

int main() {
    // 设置控制台编码为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    std::cout << "中文测试" << std::endl;
    crow::App<CORSHandler> app; 

    // 添加在 main() 函数开头
    app.loglevel(crow::LogLevel::Debug);  // 启用调试日志

    

    // 打开数据库
    if (sqlite3_open("library.db", &db) != SQLITE_OK) {
        return -1;
    }
    if (!initDatabase()) {
        return -1;
    }

    // 获取所有图书
    CROW_ROUTE(app, "/api/books").methods("GET"_method)([]() {
        auto res = crow::response();
        addCORSHeaders(res);
        
        std::cout << "收到 GET 请求" << std::endl;

        std::lock_guard<std::mutex> lock(dbMutex);

        std::vector<crow::json::wvalue> booksArray;

        const char* selectSQL = "SELECT id, title, author, isbn, category, status, publishDate, description FROM books";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                crow::json::wvalue book;
                book["id"] = sqlite3_column_int(stmt, 0);
                book["title"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                book["author"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                book["isbn"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                book["category"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
                book["status"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
                book["publishDate"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
                book["description"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
                booksArray.push_back(std::move(book));
            }
        }
        sqlite3_finalize(stmt);

        crow::json::wvalue result;
        result["data"] = std::move(booksArray); // 将数组放在data字段中
        res = crow::response(result);
        addCORSHeaders(res);
        return res;
    });

    // // 添加新图书
    CROW_ROUTE(app, "/api/books").methods("POST"_method)([](const crow::request& req) {
        auto res = crow::response();
        addCORSHeaders(res);

        std::cout << "收到 POST 请求, body: " << req.body << std::endl;
        
        auto body = crow::json::load(req.body);
        if (!body) {
            res.code = 400;
            return res;
        }
    
        std::lock_guard<std::mutex> lock(dbMutex);
        const char* insertSQL =
            "INSERT INTO books (title, author, isbn, category, status, publishDate, description) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) == SQLITE_OK) {
            // 绑定参数...
            
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                res.code = 500;
                return res;
            }
    
            // 获取最后插入的ID
            int newId = sqlite3_last_insert_rowid(db);
            sqlite3_finalize(stmt);
    
            // 查询新创建的图书
            const char* selectSQL = "SELECT id, title, author, isbn, category, status, publishDate, description FROM books WHERE id = ?";
            if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, newId);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    crow::json::wvalue book;
                    book["id"] = sqlite3_column_int(stmt, 0);
                    book["title"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                    book["author"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                    book["isbn"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                    book["category"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
                    book["status"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
                    book["publishDate"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
                    book["description"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
                    
                    res.code = 201;
                    res.body = book.dump();
                    res.set_header("Content-Type", "application/json");
                    return res;
                }
            }
            sqlite3_finalize(stmt);
        }
        
        res.code = 500;
        return res;
    });

    // 编辑图书
    CROW_ROUTE(app, "/api/books/<int>").methods("PUT"_method)([](const crow::request& req, int id) {
        auto res = crow::response();
        addCORSHeaders(res);
        
        auto body = crow::json::load(req.body);
        std::cout << "收到 PUT 请求, body: " << req.body << std::endl;
        if (!body) {
            std::cout << "??" << std::endl;
            auto res = crow::response(400);
            addCORSHeaders(res);
            return res;
        }

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
                auto res = crow::response(200);
                addCORSHeaders(res);
                return res;
            }
            auto res = crow::response(404);
            addCORSHeaders(res);
            return res;
        }
        res = crow::response(500);
        addCORSHeaders(res);
        return res;
    });

    // 删除图书
    CROW_ROUTE(app, "/api/books/<int>").methods("DELETE"_method)([](int id) {
        auto res = crow::response();
        addCORSHeaders(res);
        
        std::cout << "收到 DELETE 请求, id: " << id << std::endl;

        std::lock_guard<std::mutex> lock(dbMutex);
        const char* deleteSQL = "DELETE FROM books WHERE id=?;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, deleteSQL, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, id);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc == SQLITE_DONE && sqlite3_changes(db) > 0) {
                auto res = crow::response(200);
                addCORSHeaders(res);
                return res;
            }
            auto res = crow::response(404);
            addCORSHeaders(res);
            return res;
        }
        res = crow::response(500);
        addCORSHeaders(res);
        return res;
    });

    // 启动服务
    app.port(8080).run();

    // 关闭数据库
    sqlite3_close(db);
    return 0;
}