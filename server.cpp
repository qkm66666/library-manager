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
        "title TEXT NOT NULL DEFAULT '',"
        "author TEXT NOT NULL DEFAULT '',"
        "isbn TEXT NOT NULL DEFAULT '',"
        "category TEXT NOT NULL DEFAULT '',"
        "status TEXT NOT NULL DEFAULT '可借阅',"
        "publishDate TEXT NOT NULL DEFAULT '',"
        "description TEXT NOT NULL DEFAULT '');";

    if (sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}


int main() {
    // 设置控制台编码为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    std::cout << "中文测试" << std::endl;
    crow::App<CORSHandler> app; 

    // 添加在 main() 函数开头
    app.loglevel(crow::LogLevel::Debug);  // 启用调试日志

    

    // 打开数据库
    if (sqlite3_open("library.db", &db) != SQLITE_OK || !db) {
        std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }
    if (!initDatabase()) {
        std::cerr << "无法初始化数据库: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    // 检查数据库是否可读写
    if (sqlite3_exec(db, "SELECT count(*) FROM sqlite_master;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "数据库访问测试失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return -1;
    }

    // 获取所有图书
    CROW_ROUTE(app, "/api/books").methods("GET"_method)([]() {
        std::lock_guard<std::mutex> lock(dbMutex);
        
        if (!db) {
            return crow::response(500);
        }
    
        std::vector<crow::json::wvalue> booksArray;
        const char* selectSQL = "SELECT id, title, author, isbn, category, status, publishDate, description FROM books";
        sqlite3_stmt* stmt = nullptr;
    
        if (!db || sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) != SQLITE_OK) {
            return crow::response(500, "数据库查询准备失败: " + std::string(sqlite3_errmsg(db)));
        }
    
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            crow::json::wvalue book;
            
            // 处理每一列，安全处理NULL值
            book["id"] = sqlite3_column_int(stmt, 0);
            
            // 安全获取文本字段
            auto safeGetText = [](sqlite3_stmt* stmt, int col) -> std::string {
                const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
                return text ? text : "";
            };
            
            book["title"] = safeGetText(stmt, 1);
            book["author"] = safeGetText(stmt, 2);
            book["isbn"] = safeGetText(stmt, 3);
            book["category"] = safeGetText(stmt, 4);
            book["status"] = safeGetText(stmt, 5);
            book["publishDate"] = safeGetText(stmt, 6);
            book["description"] = safeGetText(stmt, 7);
    
            booksArray.push_back(std::move(book));
        }
    
        sqlite3_finalize(stmt);
    
        crow::json::wvalue result;
        result["data"] = std::move(booksArray);
        return crow::response(result);
    });

    // 添加新图书
    CROW_ROUTE(app, "/api/books").methods("POST"_method)([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "无效的请求体");
        }
        std::cerr << "收到 POST 请求, body: " << req.body << std::endl;
        // 获取并验证所有必填字段
        if (!body.has("title") || !body.has("author") || !body.has("isbn") || 
            !body.has("category") || !body.has("status") || !body.has("publishDate")) {
            return crow::response(400, "缺少必要字段");
        }
    
        std::lock_guard<std::mutex> lock(dbMutex);
    
        const char* insertSQL = 
            "INSERT INTO books (title, author, isbn, category, status, publishDate, description) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);";
    
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) != SQLITE_OK) {
            return crow::response(500, "SQL准备失败");
        }
    
        try {
            // 绑定参数
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
    
            // 执行插入
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            // 获取新插入的图书ID
            int newId = sqlite3_last_insert_rowid(db);
            crow::json::wvalue result;
            result["id"] = newId;
            result["message"] = "图书添加成功";
            return crow::response(201, result);
    
        } catch (const std::exception& e) {
            if (stmt) sqlite3_finalize(stmt);
            return crow::response(500, std::string("服务器错误: ") + e.what());
        }
    });

    // 编辑图书
    CROW_ROUTE(app, "/api/books/<int>").methods("PUT"_method)([](const crow::request& req, int id) {
        auto res = crow::response();
        
        auto body = crow::json::load(req.body);
        std::cout << "收到 PUT 请求, body: " << req.body << std::endl;
        if (!body) {
            std::cout << "??" << std::endl;
            auto res = crow::response(400);
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
                crow::json::wvalue result;
                result["message"] = "图书添加成功";
                auto res = crow::response(200, result);
                return res;
            }
            auto res = crow::response(404);
            return res;
        }
        res = crow::response(500);
        return res;
    });

    // 删除图书
    CROW_ROUTE(app, "/api/books/<int>").methods("DELETE"_method)([](int id) {
        auto res = crow::response();
        
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
                return res;
            }
            auto res = crow::response(404);
            return res;
        }
        res = crow::response(500);
        return res;
    });

    // 启动服务
    app.port(8080).multithreaded().run();

    // 关闭数据库
    sqlite3_close(db);
    return 0;
}