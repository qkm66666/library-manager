import os
from flask import Flask, jsonify, request
from flask_cors import CORS
import pymssql
import decimal
import datetime
from config import DB_SERVER, DB_DATABASE, DB_USER, DB_PASSWORD

app = Flask(__name__)
# Enable CORS for all routes and origins, which is convenient for development.
CORS(app)


def get_db_connection():
    """Establishes a connection to the database."""
    try:
        conn = pymssql.connect(
            server=DB_SERVER,
            user=DB_USER,
            password=DB_PASSWORD,
            database=DB_DATABASE,
            as_dict=True  # Returns rows as dictionaries, which is perfect for JSON
        )
        return conn
    except pymssql.Error as e:
        print(f"Error connecting to database: {e}")
        return None

# --- API Routes for Books ---

@app.route('/api/books', methods=['GET'])
def get_books():
    """Get all books from the database."""
    conn = get_db_connection()
    if not conn:
        return jsonify({"error": "Database connection failed"}), 500
    
    try:
        with conn.cursor() as cursor:
            cursor.execute("SELECT book_id, book_name, book_isbn, book_author, book_publisher, interview_times, book_price FROM book")
            books = cursor.fetchall()
            # Convert Decimal type to float for JSON serialization
            for book in books:
                if 'book_price' in book and isinstance(book['book_price'], decimal.Decimal):
                    book['book_price'] = float(book['book_price'])
            print(f"Found {len(books)} books.")
            return jsonify({"data": books})
    except Exception as e:
        print(f"Database query failed: {e}")
        return jsonify({"error": f"Database query failed: {e}"}), 500
    finally:
        if conn:
            conn.close()

# 修改 add_book 函数，添加更友好的错误信息
@app.route('/api/books', methods=['POST'])
def add_book():
    """Add a new book to the database."""
    data = request.get_json()
    if not data or not data.get('book_id'):
        return jsonify({'error': '图书ID不能为空'}), 400

    conn = get_db_connection()
    cursor = conn.cursor()

    # 检查ID是否重复
    cursor.execute("SELECT COUNT(*) as count FROM book WHERE book_id = %s", (data['book_id'],))
    if cursor.fetchone()['count'] > 0:
        conn.close()
        return jsonify({
            'error': '主键重复',
            'message': f"图书ID '{data['book_id']}' 已存在，请使用其他ID",
            'code': 'DUPLICATE_KEY'
        }), 409  # 409 Conflict

    try:
        cursor.execute(
            "INSERT INTO book (book_id, book_name, book_author, book_isbn, book_publisher, interview_times, book_price) VALUES (%s, %s, %s, %s, %s, %s, %s)",
            (data['book_id'], data['book_name'], data['book_author'], data['book_isbn'], data['book_publisher'], data.get('interview_times', 0), data['book_price'])
        )
        conn.commit()
        return jsonify({
            'message': '图书添加成功',
            'book_id': data['book_id'],
            'book_name': data['book_name']
        }), 201
    except pymssql.Error as e:
        conn.rollback()
        error_message = str(e)
        if 'PRIMARY KEY' in error_message or 'duplicate' in error_message.lower():
            return jsonify({
                'error': '主键重复',
                'message': '图书ID已存在，请使用其他ID',
                'code': 'DUPLICATE_KEY'
            }), 409
        else:
            return jsonify({
                'error': '数据库错误',
                'message': error_message,
                'code': 'DATABASE_ERROR'
            }), 500
    finally:
        conn.close()

@app.route('/api/books/<string:book_id>', methods=['PUT'])
def update_book(book_id):
    """Update an existing book."""
    body = request.get_json()
    if not body:
        return jsonify({"error": "Invalid request body"}), 400

    sql = """
        UPDATE book 
        SET book_name=%s, book_isbn=%s, book_author=%s, book_publisher=%s, interview_times=%d, book_price=%s 
        WHERE book_id=%s
    """

    conn = get_db_connection()
    if not conn:
        return jsonify({"error": "Database connection failed"}), 500

    try:
        with conn.cursor() as cursor:
            cursor.execute(sql, (
                body['book_name'],
                body['book_isbn'],
                body['book_author'],
                body['book_publisher'],
                body.get('interview_times', 0),
                body.get('book_price', 0.0),
                book_id
            ))
            conn.commit()
            
            if cursor.rowcount == 0:
                return jsonify({"error": "Book to update not found"}), 404
            
            print(f"Book updated with ID: {book_id}")
            return jsonify({"message": "更新成功"})
    except Exception as e:
        print(f"Database update failed: {e}")
        return jsonify({"error": f"Database update failed: {e}"}), 500
    finally:
        if conn:
            conn.close()

@app.route('/api/books/<string:book_id>', methods=['DELETE'])
def delete_book(book_id):
    """Delete a book from the database."""
    sql = "DELETE FROM book WHERE book_id = %s"
    
    conn = get_db_connection()
    if not conn:
        return jsonify({"error": "Database connection failed"}), 500

    try:
        with conn.cursor() as cursor:
            cursor.execute(sql, (book_id,))
            conn.commit()

            if cursor.rowcount == 0:
                return jsonify({"error": "Book to delete not found"}), 404

            print(f"Book deleted with ID: {book_id}")
            return jsonify({"message": "Book deleted successfully"})
    except Exception as e:
        print(f"Database delete failed: {e}")
        return jsonify({"error": f"Database delete failed: {e}"}), 500
    finally:
        if conn:
            conn.close()

# --- API Routes for Readers ---

@app.route('/api/readers', methods=['GET'])
def get_readers():
    """Get all readers from the database."""
    conn = get_db_connection()
    if not conn:
        return jsonify({"error": "Database connection failed"}), 500
    
    try:
        with conn.cursor() as cursor:
            cursor.execute("SELECT reader_id, reader_name, reader_sex, reader_department FROM reader")
            readers = cursor.fetchall()
            return jsonify({"data": readers})
    except Exception as e:
        return jsonify({"error": f"Database query failed: {e}"}), 500
    finally:
        if conn:
            conn.close()

@app.route('/api/readers', methods=['POST'])
def add_reader():
    """Add a new reader."""
    data = request.get_json()
    if not data or not data.get('reader_id'):
        return jsonify({'error': 'Missing reader_id'}), 400

    conn = get_db_connection()
    cursor = conn.cursor()

    # 检查ID是否重复
    cursor.execute("SELECT COUNT(*) as count FROM reader WHERE reader_id = %s", (data['reader_id'],))
    if cursor.fetchone()['count'] > 0:
        conn.close()
        return jsonify({'error': f"Reader with ID {data['reader_id']} already exists."}), 409

    try:
        cursor.execute(
            "INSERT INTO reader (reader_id, reader_name, reader_sex, reader_department) VALUES (%s, %s, %s, %s)",
            (data['reader_id'], data['reader_name'], data['reader_sex'], data['reader_department'])
        )
        conn.commit()
        return jsonify({'message': 'Reader added successfully'}), 201
    except pymssql.Error as e:
        conn.rollback()
        return jsonify({'error': str(e)}), 500
    finally:
        conn.close()

@app.route('/api/readers/<string:reader_id>', methods=['PUT'])
def update_reader(reader_id):
    """Update an existing reader."""
    body = request.get_json()
    if not body:
        return jsonify({"error": "Invalid request body"}), 400

    sql = "UPDATE reader SET reader_name=%s, reader_sex=%s, reader_department=%s WHERE reader_id=%s"

    conn = get_db_connection()
    if not conn:
        return jsonify({"error": "Database connection failed"}), 500

    try:
        with conn.cursor() as cursor:
            cursor.execute(sql, (body['reader_name'], body['reader_sex'], body['reader_department'], reader_id))
            conn.commit()
            if cursor.rowcount == 0:
                return jsonify({"error": "Reader not found"}), 404
            return jsonify({"message": "Reader updated successfully"})
    except Exception as e:
        return jsonify({"error": f"Database update failed: {e}"}), 500
    finally:
        if conn:
            conn.close()

@app.route('/api/readers/<string:reader_id>', methods=['DELETE'])
def delete_reader(reader_id):
    """Delete a reader."""
    sql = "DELETE FROM reader WHERE reader_id = %s"
    conn = get_db_connection()
    if not conn:
        return jsonify({"error": "Database connection failed"}), 500

    try:
        with conn.cursor() as cursor:
            cursor.execute(sql, (reader_id,))
            conn.commit()
            if cursor.rowcount == 0:
                return jsonify({"error": "Reader not found"}), 404
            return jsonify({"message": "Reader deleted successfully"})
    except Exception as e:
        return jsonify({"error": f"Database delete failed: {e}"}), 500
    finally:
        if conn:
            conn.close()

# --- API Routes for Records ---

@app.route('/api/records', methods=['GET'])
def get_records():
    """Get all borrow records."""
    conn = get_db_connection()
    if not conn:
        return jsonify({"error": "Database connection failed"}), 500
    
    try:
        with conn.cursor() as cursor:
            cursor.execute("SELECT TRIM(reader_id) as reader_id, TRIM(book_id) as book_id, borrow_date, return_date, notes FROM record")
            records_raw = cursor.fetchall()
            records = []
            # Manually add a unique ID for frontend purposes
            for i, record_raw in enumerate(records_raw):
                record_raw['record_id'] = f"temp_{i}"
                records.append(record_raw)
            # Convert date objects to ISO format strings for JSON
            for record in records:
                if 'borrow_date' in record and isinstance(record['borrow_date'], datetime.date):
                    record['borrow_date'] = record['borrow_date'].isoformat()
                if 'return_date' in record and isinstance(record['return_date'], datetime.date):
                    record['return_date'] = record['return_date'].isoformat()
            return jsonify({"data": records})
    except Exception as e:
        return jsonify({"error": f"Database query failed: {e}"}), 500
    finally:
        if conn:
            conn.close()

@app.route('/api/records', methods=['POST'])
def add_record():
    """Add a new borrow record."""
    data = request.get_json()
    if not data or not data.get('book_id') or not data.get('reader_id'):
        return jsonify({'error': 'Missing book_id or reader_id'}), 400

    conn = get_db_connection()
    if not conn:
        return jsonify({"error": "Database connection failed"}), 500
    
    try:
        with conn.cursor() as cursor:
            # 检查联合主键是否重复
            cursor.execute(
                "SELECT COUNT(*) as count FROM record WHERE book_id = %s AND reader_id = %s",
                (data['book_id'], data['reader_id'])
            )
            if cursor.fetchone()['count'] > 0:
                conn.close()
                return jsonify({'error': f"Record with Book ID {data['book_id']} and Reader ID {data['reader_id']} already exists."}), 409

            cursor.execute(
                "INSERT INTO record (book_id, reader_id, borrow_date, return_date, notes) VALUES (%s, %s, %s, %s, %s)",
                (data['book_id'], data['reader_id'], data.get('borrow_date'), data.get('return_date'), data.get('notes'))
            )
            conn.commit()
            return jsonify({'message': 'Record added successfully'}), 201
    except pymssql.Error as e:
        conn.rollback()
        return jsonify({'error': str(e)}), 500
    finally:
        if conn:
            conn.close()

@app.route('/api/records/<string:book_id>/<string:reader_id>', methods=['PUT'])
def update_record(book_id, reader_id):
    """Update a borrow record using book_id and reader_id as composite key."""
    data = request.get_json()
    if not data:
        return jsonify({"error": "Invalid data"}), 400

    # --- 调试日志 ---
    print(f"--- DEBUG: Updating Record ---")
    print(f"Received book_id: '{book_id}'")
    print(f"Received reader_id: '{reader_id}'")
    print(f"Received data: {data}")
    # --- 结束调试日志 ---

    conn = get_db_connection()
    if not conn:
        return jsonify({"error": "Database connection failed"}), 500

    try:
        with conn.cursor() as cursor:
            sql = "UPDATE record SET borrow_date = %s, return_date = %s, notes = %s WHERE TRIM(book_id) = %s AND TRIM(reader_id) = %s"
            params = (
                data.get('borrow_date'),
                data.get('return_date'),
                data.get('notes'),
                book_id,
                reader_id
            )
            
            # --- 调试日志 ---
            print(f"Executing SQL: {sql}")
            print(f"With Params: {params}")
            # --- 结束调试日志 ---

            cursor.execute(sql, params)
            conn.commit()

            # --- 调试日志 ---
            print(f"Update executed. Rows affected: {cursor.rowcount}")
            # --- 结束调试日志 ---

            if cursor.rowcount == 0:
                return jsonify({'error': 'Record not found or data not changed'}), 404
            
            return jsonify({'message': 'Record updated successfully'}), 200
    except pymssql.Error as e:
        conn.rollback()
        return jsonify({'error': str(e)}), 500
    finally:
        if conn:
            conn.close()

@app.route('/api/records/<string:book_id>/<string:reader_id>', methods=['DELETE'])
def delete_record(book_id, reader_id):
    """Delete a borrow record using its composite key."""
    conn = get_db_connection()
    if not conn:
        return jsonify({"error": "Database connection failed"}), 500
    
    try:
        with conn.cursor() as cursor:
            sql = "DELETE FROM record WHERE TRIM(book_id) = %s AND TRIM(reader_id) = %s"
            cursor.execute(sql, (book_id, reader_id))
            conn.commit()
            if cursor.rowcount == 0:
                return jsonify({'error': 'Record not found'}), 404
            return jsonify({'message': 'Record deleted successfully'}), 200
    except pymssql.Error as e:
        conn.rollback()
        return jsonify({'error': str(e)}), 500
    finally:
        if conn:
            conn.close()

# 添加高级搜索图书的API端点
@app.route('/api/books/search', methods=['GET'])
def search_books():
    """高级搜索图书"""
    # 获取查询参数
    keyword = request.args.get('keyword', '').strip()
    search_by = request.args.get('search_by', 'all')  # all, title, author, publisher, isbn
    min_price = request.args.get('min_price')
    max_price = request.args.get('max_price')

    conn = get_db_connection()
    if not conn:
        return jsonify({"error": "Database connection failed"}), 500

    try:
        with conn.cursor() as cursor:
            # 基础查询
            base_sql = "SELECT book_id, book_name, book_isbn, book_author, book_publisher, interview_times, book_price FROM book WHERE 1=1"
            params = []

            # 添加关键词搜索条件
            if keyword:
                if search_by == 'title':
                    base_sql += " AND book_name LIKE %s"
                    params.append(f'%{keyword}%')
                elif search_by == 'author':
                    base_sql += " AND book_author LIKE %s"
                    params.append(f'%{keyword}%')
                elif search_by == 'publisher':
                    base_sql += " AND book_publisher LIKE %s"
                    params.append(f'%{keyword}%')
                elif search_by == 'isbn':
                    base_sql += " AND book_isbn LIKE %s"
                    params.append(f'%{keyword}%')
                else:  # all
                    base_sql += " AND (book_name LIKE %s OR book_author LIKE %s OR book_isbn LIKE %s OR book_publisher LIKE %s)"
                    params.extend([f'%{keyword}%', f'%{keyword}%', f'%{keyword}%', f'%{keyword}%'])

            # 添加价格区间筛选
            if min_price:
                try:
                    min_price_float = float(min_price)
                    base_sql += " AND book_price >= %s"
                    params.append(min_price_float)
                except ValueError:
                    pass

            if max_price:
                try:
                    max_price_float = float(max_price)
                    base_sql += " AND book_price <= %s"
                    params.append(max_price_float)
                except ValueError:
                    pass

            print(f"Executing search query: {base_sql} with params: {params}")
            cursor.execute(base_sql, tuple(params))
            books = cursor.fetchall()

            # 转换Decimal类型
            for book in books:
                if 'book_price' in book and isinstance(book['book_price'], decimal.Decimal):
                    book['book_price'] = float(book['book_price'])

            return jsonify({
                "data": books,
                "total": len(books),
                "keyword": keyword,
                "search_by": search_by,
                "min_price": min_price,
                "max_price": max_price
            })
    except Exception as e:
        print(f"Database search failed: {e}")
        return jsonify({"error": f"Database search failed: {e}"}), 500
    finally:
        if conn:
            conn.close()

if __name__ == '__main__':
    # Runs the app on http://127.0.0.1:8080
    app.run(host='0.0.0.0', port=8080, debug=True)
