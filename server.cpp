#include <crow.h>
#include <nanodbc/nanodbc.h>
#include <string>
#include <mutex>
#include <iostream>
#include <vector>
#include <memory>
#include <windows.h>
#include <locale>
#include <codecvt>

// Helper function to convert UTF-8 std::string to std::wstring
std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Helper function to convert std::wstring to UTF-8 std::string
std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Database connection string for SQL Server
// IMPORTANT: 
// 1. Make sure "ODBC Driver 17 for SQL Server" is installed on your system.
// 2. Change Server, UID, and PWD to match your SQL Server configuration.
const nanodbc::string connection_string = NANODBC_TEXT("Driver={ODBC Driver 17 for SQL Server};Server=localhost;Database=JY;UID=sa;PWD=Eld_4ever;");

// A simple connection pool
class ConnectionPool {
public:
    ConnectionPool(const nanodbc::string& conn_str, size_t size) : connection_string_(conn_str) {
        for (size_t i = 0; i < size; ++i) {
            try {
                pool_.push_back(std::make_unique<nanodbc::connection>(connection_string_));
            } catch (const nanodbc::database_error& e) {
                std::cerr << "Error creating connection for pool: " << e.what() << std::endl;
            }
        }
    }

    std::unique_ptr<nanodbc::connection> get_connection() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.empty()) {
            // Or wait, or throw an exception
            return std::make_unique<nanodbc::connection>(connection_string_);
        }
        auto conn = std::move(pool_.back());
        pool_.pop_back();
        return conn;
    }

    void return_connection(std::unique_ptr<nanodbc::connection> conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push_back(std::move(conn));
    }

private:
    std::mutex mutex_;
    nanodbc::string connection_string_;
    std::vector<std::unique_ptr<nanodbc::connection>> pool_;
};

// Global connection pool
std::unique_ptr<ConnectionPool> db_pool;


struct CORSHandler {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context&) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        
        // Handle OPTIONS requests, return 204 directly
        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 204;
            res.end();
        }
    }

    void after_handle(crow::request&, crow::response& res, context&) {
        // Ensure all responses include CORS headers
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }
};

// Initialize database connection pool
bool initDatabase() {
    try {
        db_pool = std::make_unique<ConnectionPool>(connection_string, 5); // Pool with 5 connections
        // Test getting a connection
        auto conn = db_pool->get_connection();
        if (conn && conn->connected()) {
            std::cout << "Database connection pool created successfully." << std::endl;
            db_pool->return_connection(std::move(conn));
            return true;
        }
    } catch (const nanodbc::database_error& e) {
        std::cerr << "Database connection failed: " << e.what() << std::endl;
        return false;
    }
    return false;
}

int main() {
    crow::App<CORSHandler> app;
    app.loglevel(crow::LogLevel::Debug);

    if (!initDatabase()) {
        return -1;
    }

    // Get all books
    CROW_ROUTE(app, "/api/books").methods("GET"_method)([]() {
        CROW_LOG_INFO << "Received request for GET /api/books";
        try {
            auto conn = db_pool->get_connection();
            if (!conn || !conn->connected()) {
                CROW_LOG_ERROR << "Failed to get a valid database connection from pool.";
                return crow::response(500, "Failed to get database connection.");
            }

            auto result = nanodbc::execute(*conn, NANODBC_TEXT("SELECT book_id, book_name, book_isbn, book_author, book_publisher, interview_times, book_price FROM book"));

            std::vector<crow::json::wvalue> booksArray;
            long long count = 0;
            while (result.next()) {
                count++;
                crow::json::wvalue book;
                
                nanodbc::string book_id_w = result.get<nanodbc::string>(0);
                book["book_id"] = wstring_to_utf8(book_id_w);

                nanodbc::string book_name_w = result.get<nanodbc::string>(1);
                book["book_name"] = wstring_to_utf8(book_name_w);

                nanodbc::string book_isbn_w = result.get<nanodbc::string>(2);
                book["book_isbn"] = wstring_to_utf8(book_isbn_w);

                nanodbc::string book_author_w = result.get<nanodbc::string>(3);
                book["book_author"] = wstring_to_utf8(book_author_w);

                nanodbc::string book_publisher_w = result.get<nanodbc::string>(4);
                book["book_publisher"] = wstring_to_utf8(book_publisher_w);

                book["interview_times"] = result.get<int>(5, 0);
                book["book_price"] = result.get<double>(6, 0.0);
                booksArray.push_back(std::move(book));
            }
            CROW_LOG_INFO << "Found " << count << " books in the database.";

            db_pool->return_connection(std::move(conn));
            crow::json::wvalue response;
            response["data"] = std::move(booksArray);
            return crow::response(response);
        } catch (const nanodbc::database_error& e) {
            CROW_LOG_ERROR << "Database query failed for GET /api/books: " << e.what();
            return crow::response(500, "Database query failed: " + std::string(e.what()));
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "An unexpected error occurred in GET /api/books: " << e.what();
            return crow::response(500, "An unexpected error occurred: " + std::string(e.what()));
        }
    });

    // Add a new book
    CROW_ROUTE(app, "/api/books").methods("POST"_method)([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "Invalid request body");

        // Field validation
        const std::vector<std::string> required_fields = {"book_id", "book_name", "book_isbn", "book_author", "book_publisher"};
        for (const auto& field : required_fields) {
            if (!body.has(field)) {
                return crow::response(400, "Missing required field: " + field);
            }
        }

        try {
            auto conn = db_pool->get_connection();
            nanodbc::statement stmt(*conn);
            nanodbc::prepare(stmt, NANODBC_TEXT("INSERT INTO book (book_id, book_name, book_isbn, book_author, book_publisher, interview_times, book_price) VALUES (?, ?, ?, ?, ?, ?, ?)"));

            const auto book_id = utf8_to_wstring(body["book_id"].s());
            const auto book_name = utf8_to_wstring(body["book_name"].s());
            const auto book_isbn = utf8_to_wstring(body["book_isbn"].s());
            const auto book_author = utf8_to_wstring(body["book_author"].s());
            const auto book_publisher = utf8_to_wstring(body["book_publisher"].s());
            int interview_times = body.has("interview_times") ? body["interview_times"].i() : 0;
            double book_price = body.has("book_price") ? body["book_price"].d() : 0.0;

            stmt.bind(0, book_id.c_str());
            stmt.bind(1, book_name.c_str());
            stmt.bind(2, book_isbn.c_str());
            stmt.bind(3, book_author.c_str());
            stmt.bind(4, book_publisher.c_str());
            stmt.bind(5, &interview_times);
            stmt.bind(6, &book_price);

            nanodbc::execute(stmt);
            
            db_pool->return_connection(std::move(conn));
            
            crow::json::wvalue result;
            result["message"] = "Book added successfully";
            result["book_id"] = body["book_id"].s();
            return crow::response(201, result);
        } catch (const nanodbc::database_error& e) {
            return crow::response(500, "Database insert failed: " + std::string(e.what()));
        }
    });

    // Edit a book
    CROW_ROUTE(app, "/api/books/<string>").methods("PUT"_method)([](const crow::request& req, std::string book_id_str) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "Invalid request body");

        try {
            auto conn = db_pool->get_connection();
            nanodbc::statement stmt(*conn);
            nanodbc::prepare(stmt, NANODBC_TEXT("UPDATE book SET book_name=?, book_isbn=?, book_author=?, book_publisher=?, interview_times=?, book_price=? WHERE book_id=?"));

            const auto book_name = body["book_name"].s();
            const auto book_isbn = body["book_isbn"].s();
            const auto book_author = body["book_author"].s();
            const auto book_publisher = body["book_publisher"].s();
            int interview_times = body["interview_times"].i();
            double book_price = body["book_price"].d();
            const auto book_id = book_id_str;

            stmt.bind(0, book_name);
            stmt.bind(1, book_isbn);
            stmt.bind(2, book_author);
            stmt.bind(3, book_publisher);
            stmt.bind(4, &interview_times);
            stmt.bind(5, &book_price);
            stmt.bind(6, book_id);

            auto result = nanodbc::execute(stmt);

            db_pool->return_connection(std::move(conn));

            if (result.affected_rows() == 0) {
                return crow::response(404, "Book to update not found");
            }

            crow::json::wvalue response_body;
            response_body["message"] = "Book updated successfully";
            return crow::response(200, response_body);
        } catch (const nanodbc::database_error& e) {
            return crow::response(500, "Database update failed: " + std::string(e.what()));
        }
    });

    // Delete a book
    CROW_ROUTE(app, "/api/books/<string>").methods("DELETE"_method)([](std::string book_id_str) {
        try {
            auto conn = db_pool->get_connection();
            nanodbc::statement stmt(*conn);
            nanodbc::prepare(stmt, NANODBC_TEXT("DELETE FROM book WHERE book_id = ?"));
            
            const auto book_id = book_id_str;
            stmt.bind(0, book_id);

            auto result = nanodbc::execute(stmt);

            db_pool->return_connection(std::move(conn));

            if (result.affected_rows() == 0) {
                return crow::response(404, "Book to delete not found");
            }

            crow::json::wvalue response_body;
            response_body["message"] = "Book deleted successfully";
            return crow::response(200, response_body);
        } catch (const nanodbc::database_error& e) {
            return crow::response(500, "Database delete failed: " + std::string(e.what()));
        }
    });

    app.port(8080).multithreaded().run();

    return 0;
}