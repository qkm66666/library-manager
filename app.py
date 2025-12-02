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

# --- Database Configuration ---
# IMPORTANT: Make sure these details match your SQL Server configuration.


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

@app.route('/api/books', methods=['POST'])
def add_book():
    """Add a new book to the database."""
    data = request.get_json()
    if not data or not data.get('book_id'):
        return jsonify({'error': 'Missing book_id'}), 400

    conn = get_db_connection()
    cursor = conn.cursor()
    
    # 检查ID是否重复
    cursor.execute("SELECT COUNT(*) FROM book WHERE book_id = %s", (data['book_id'],))
    if cursor.fetchone()[0] > 0:
        conn.close()
        return jsonify({'error': f"Book with ID {data['book_id']} already exists."}), 409 # 409 Conflict

    try:
        cursor.execute(
            "INSERT INTO book (book_id, book_name, book_author, book_isbn, book_publisher, interview_times, book_price) VALUES (%s, %s, %s, %s, %s, %s, %s)",
            (data['book_id'], data['book_name'], data['book_author'], data['book_isbn'], data['book_publisher'], data.get('interview_times', 0), data['book_price'])
        )
        conn.commit()
        return jsonify({'message': 'Book added successfully'}), 201
    except pymssql.Error as e:
        conn.rollback()
        return jsonify({'error': str(e)}), 500
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
            return jsonify({"message": "Book updated successfully"})
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
    cursor.execute("SELECT COUNT(*) FROM reader WHERE reader_id = %s", (data['reader_id'],))
    if cursor.fetchone()[0] > 0:
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
            cursor.execute("SELECT reader_id, book_id, borrow_date, return_date, notes FROM record")
            records = cursor.fetchall()
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
    if not data or not data.get('record_id'):
        return jsonify({'error': 'Missing record_id'}), 400

    conn = get_db_connection()
    cursor = conn.cursor()

    # 检查ID是否重复
    cursor.execute("SELECT COUNT(*) FROM record WHERE record_id = %s", (data['record_id'],))
    if cursor.fetchone()[0] > 0:
        conn.close()
        return jsonify({'error': f"Record with ID {data['record_id']} already exists."}), 409

    try:
        cursor.execute(
            "INSERT INTO record (record_id, book_id, reader_id, borrow_date, return_date) VALUES (%s, %s, %s, %s, %s)",
            (data['record_id'], data['book_id'], data['reader_id'], data.get('borrow_date'), data.get('return_date'))
        )
        conn.commit()
        return jsonify({'message': 'Record added successfully'}), 201
    except pymssql.Error as e:
        conn.rollback()
        return jsonify({'error': str(e)}), 500
    finally:
        conn.close()

@app.route('/api/records/<int:record_id>', methods=['PUT'])
def update_record(record_id):
    """Update a borrow record."""
    data = request.get_json()
    if not data:
        return jsonify({"error": "Invalid data"}), 400

    conn = get_db_connection()
    cursor = conn.cursor()

    try:
        update_fields = []
        params = []
        # 动态构建SET子句
        for key, value in data.items():
            if key in ['book_id', 'reader_id', 'borrow_date', 'return_date']:
                update_fields.append(f"{key} = %s")
                params.append(value)

        if not update_fields:
            return jsonify({"error": "No fields to update"}), 400

        sql = f"UPDATE record SET {', '.join(update_fields)} WHERE record_id = %s"
        params.append(record_id)
        
        cursor.execute(sql, tuple(params))
        conn.commit()

        if cursor.rowcount == 0:
            return jsonify({'error': 'Record not found or data not changed'}), 404
        
        return jsonify({'message': 'Record updated successfully'}), 200
    except pymssql.Error as e:
        conn.rollback()
        return jsonify({'error': str(e)}), 500
    finally:
        conn.close()

@app.route('/api/records/<int:record_id>', methods=['DELETE'])
def delete_record(record_id):
    """Delete a borrow record."""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        sql = "DELETE FROM record WHERE record_id = %s"
        cursor.execute(sql, (record_id,))
        conn.commit()
        if cursor.rowcount == 0:
            return jsonify({'error': 'Record not found'}), 404
        return jsonify({'message': 'Record deleted successfully'}), 200
    except pymssql.Error as e:
        conn.rollback()
        return jsonify({'error': str(e)}), 500
    finally:
        conn.close()

if __name__ == '__main__':
    # Runs the app on http://127.0.0.1:8080
    app.run(host='0.0.0.0', port=8080, debug=True)
