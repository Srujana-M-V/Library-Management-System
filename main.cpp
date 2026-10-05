#include <iostream>
#include <string>
#include <iomanip>
#include <sqlite3.h>

using namespace std;

class Book {
public:
    int id;
    string name;
    string author;
    int copies;

    Book() {
        id = 0;
        copies = 0;
    }

    Book(int id, string name, string author, int copies) {
        this->id = id;
        this->name = name;
        this->author = author;
        this->copies = copies;
    }
};

class Member {
public:
    int id;
    string name;

    Member() {
        id = 0;
    }

    Member(int id, string name) {
        this->id = id;
        this->name = name;
    }
};

sqlite3* db = nullptr;

bool executeSQL(const string& sql) {
    char* errorMessage = nullptr;

    int result = sqlite3_exec(
        db,
        sql.c_str(),
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK) {
        cout << "\nSQL Error: " << errorMessage << endl;
        sqlite3_free(errorMessage);
        return false;
    }

    return true;
}

bool initializeDatabase() {
    string sql = R"(

        PRAGMA foreign_keys = ON;

        CREATE TABLE IF NOT EXISTS books (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            author TEXT NOT NULL,
            copies INTEGER NOT NULL CHECK(copies >= 0)
        );

        CREATE TABLE IF NOT EXISTS members (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS issued_books (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            member_id INTEGER NOT NULL,
            book_id INTEGER NOT NULL,
            issue_date DATETIME DEFAULT CURRENT_TIMESTAMP,
            return_date DATETIME,
            FOREIGN KEY(member_id)
                REFERENCES members(id)
                ON DELETE CASCADE,
            FOREIGN KEY(book_id)
                REFERENCES books(id)
                ON DELETE CASCADE
        );

    )";

    return executeSQL(sql);
}

void addBook() {
    int id, copies;
    string name, author;

    cout << "\nEnter Book ID: ";
    cin >> id;

    cin.ignore();

    cout << "Enter Book Name: ";
    getline(cin, name);

    cout << "Enter Author Name: ";
    getline(cin, author);

    cout << "Enter Number of Copies: ";
    cin >> copies;

    if (copies < 0) {
        cout << "Copies cannot be negative.\n";
        return;
    }

    string sql = R"(
        INSERT INTO books
        (id, name, author, copies)
        VALUES (?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(stmt, 1, id);

    sqlite3_bind_text(
        stmt,
        2,
        name.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        stmt,
        3,
        author.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(stmt, 4, copies);

    int result = sqlite3_step(stmt);

    if (result == SQLITE_DONE)
        cout << "\nBook added successfully!\n";
    else
        cout << "\nBook ID already exists or invalid data.\n";

    sqlite3_finalize(stmt);
}

void displayBooks() {
    string sql = R"(
        SELECT id, name, author, copies
        FROM books
        ORDER BY id;
    )";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr
    );

    cout << "\n";
    cout << "============================================================\n";
    cout << left
         << setw(8) << "ID"
         << setw(25) << "BOOK"
         << setw(25) << "AUTHOR"
         << setw(10) << "COPIES"
         << "\n";
    cout << "============================================================\n";

    bool found = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;

        int id = sqlite3_column_int(stmt, 0);

        string name =
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 1)
            );

        string author =
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 2)
            );

        int copies = sqlite3_column_int(stmt, 3);

        cout << left
             << setw(8) << id
             << setw(25) << name
             << setw(25) << author
             << setw(10) << copies
             << "\n";
    }

    if (!found)
        cout << "No books available.\n";

    sqlite3_finalize(stmt);
}

void searchBook() {
    string keyword;

    cin.ignore();

    cout << "\nEnter book name or author: ";
    getline(cin, keyword);

    string sql = R"(
        SELECT id, name, author, copies
        FROM books
        WHERE name LIKE ?
        OR author LIKE ?
        ORDER BY name;
    )";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr
    );

    string pattern = "%" + keyword + "%";

    sqlite3_bind_text(
        stmt,
        1,
        pattern.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        stmt,
        2,
        pattern.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    bool found = false;

    cout << "\nSearch Results:\n";
    cout << "------------------------------------------------------------\n";

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;

        cout
            << "ID: "
            << sqlite3_column_int(stmt, 0)
            << " | Book: "
            << sqlite3_column_text(stmt, 1)
            << " | Author: "
            << sqlite3_column_text(stmt, 2)
            << " | Copies: "
            << sqlite3_column_int(stmt, 3)
            << "\n";
    }

    if (!found)
        cout << "No matching books found.\n";

    sqlite3_finalize(stmt);
}

void updateBook() {
    int id;

    cout << "\nEnter Book ID to update: ";
    cin >> id;

    string checkSQL =
        "SELECT id FROM books WHERE id = ?;";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(
        db,
        checkSQL.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        cout << "Book not found.\n";
        sqlite3_finalize(stmt);
        return;
    }

    sqlite3_finalize(stmt);

    string name, author;
    int copies;

    cin.ignore();

    cout << "Enter new Book Name: ";
    getline(cin, name);

    cout << "Enter new Author: ";
    getline(cin, author);

    cout << "Enter new Copies: ";
    cin >> copies;

    if (copies < 0) {
        cout << "Invalid number of copies.\n";
        return;
    }

    string sql = R"(
        UPDATE books
        SET name = ?,
            author = ?,
            copies = ?
        WHERE id = ?;
    )";

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_text(
        stmt,
        1,
        name.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        stmt,
        2,
        author.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(stmt, 3, copies);
    sqlite3_bind_int(stmt, 4, id);

    if (sqlite3_step(stmt) == SQLITE_DONE)
        cout << "Book updated successfully!\n";

    sqlite3_finalize(stmt);
}

void deleteBook() {
    int id;

    cout << "\nEnter Book ID to delete: ";
    cin >> id;

    string sql =
        "DELETE FROM books WHERE id = ?;";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        if (sqlite3_changes(db) > 0)
            cout << "Book deleted successfully!\n";
        else
            cout << "Book not found.\n";
    }

    sqlite3_finalize(stmt);
}

void addMember() {
    int id;
    string name;

    cout << "\nEnter Member ID: ";
    cin >> id;

    cin.ignore();

    cout << "Enter Member Name: ";
    getline(cin, name);

    string sql = R"(
        INSERT INTO members
        (id, name)
        VALUES (?, ?);
    )";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(stmt, 1, id);

    sqlite3_bind_text(
        stmt,
        2,
        name.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    if (sqlite3_step(stmt) == SQLITE_DONE)
        cout << "Member added successfully!\n";
    else
        cout << "Member ID already exists.\n";

    sqlite3_finalize(stmt);
}

void displayMembers() {
    string sql = R"(
        SELECT
            m.id,
            m.name,
            b.name
        FROM members m
        LEFT JOIN issued_books i
            ON m.id = i.member_id
            AND i.return_date IS NULL
        LEFT JOIN books b
            ON i.book_id = b.id
        ORDER BY m.id;
    )";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr
    );

    cout << "\n";
    cout << "========================================================\n";

    cout << left
         << setw(10) << "ID"
         << setw(25) << "MEMBER"
         << setw(25) << "ISSUED BOOK"
         << "\n";

    cout << "========================================================\n";

    bool found = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;

        int id =
            sqlite3_column_int(stmt, 0);

        string name =
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 1)
            );

        const unsigned char* book =
            sqlite3_column_text(stmt, 2);

        cout << left
             << setw(10) << id
             << setw(25) << name;

        if (book != nullptr)
            cout << setw(25) << book;
        else
            cout << setw(25) << "None";

        cout << "\n";
    }

    if (!found)
        cout << "No members available.\n";

    sqlite3_finalize(stmt);
}

void deleteMember() {
    int id;

    cout << "\nEnter Member ID to delete: ";
    cin >> id;

    string checkSQL = R"(
        SELECT id
        FROM issued_books
        WHERE member_id = ?
        AND return_date IS NULL;
    )";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(
        db,
        checkSQL.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        cout << "Cannot delete member while a book is issued.\n";
        sqlite3_finalize(stmt);
        return;
    }

    sqlite3_finalize(stmt);

    string sql =
        "DELETE FROM members WHERE id = ?;";

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(stmt, 1, id);

    sqlite3_step(stmt);

    if (sqlite3_changes(db) > 0)
        cout << "Member deleted successfully!\n";
    else
        cout << "Member not found.\n";

    sqlite3_finalize(stmt);
}

void issueBook() {
    int memberId;
    int bookId;

    cout << "\nEnter Member ID: ";
    cin >> memberId;

    cout << "Enter Book ID: ";
    cin >> bookId;

    executeSQL("BEGIN TRANSACTION;");

    string memberSQL =
        "SELECT id FROM members WHERE id = ?;";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(
        db,
        memberSQL.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(stmt, 1, memberId);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        cout << "Member not found.\n";
        sqlite3_finalize(stmt);
        executeSQL("ROLLBACK;");
        return;
    }

    sqlite3_finalize(stmt);

    string bookSQL =
        "SELECT copies FROM books WHERE id = ?;";

    sqlite3_prepare_v2(
        db,
        bookSQL.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(stmt, 1, bookId);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        cout << "Book not found.\n";
        sqlite3_finalize(stmt);
        executeSQL("ROLLBACK;");
        return;
    }

    int copies =
        sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    if (copies <= 0) {
        cout << "No copies available.\n";
        executeSQL("ROLLBACK;");
        return;
    }

    string activeSQL = R"(
        SELECT id
        FROM issued_books
        WHERE member_id = ?
        AND return_date IS NULL;
    )";

    sqlite3_prepare_v2(
        db,
        activeSQL.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(stmt, 1, memberId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        cout << "Member already has a book.\n";
        sqlite3_finalize(stmt);
        executeSQL("ROLLBACK;");
        return;
    }

    sqlite3_finalize(stmt);

    string updateSQL = R"(
        UPDATE books
        SET copies = copies - 1
        WHERE id = ?;
    )";

    sqlite3_prepare_v2(
        db,
        updateSQL.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(stmt, 1, bookId);

    sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    string insertSQL = R"(
        INSERT INTO issued_books
        (member_id, book_id)
        VALUES (?, ?);
    )";

    sqlite3_prepare_v2(
        db,
        insertSQL.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(stmt, 1, memberId);
    sqlite3_bind_int(stmt, 2, bookId);

    sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    executeSQL("COMMIT;");

    cout << "Book issued successfully!\n";
}

void returnBook() {
    int memberId;

    cout << "\nEnter Member ID: ";
    cin >> memberId;

    string sql = R"(
        SELECT
            id,
            book_id,
            issue_date
        FROM issued_books
        WHERE member_id = ?
        AND return_date IS NULL;
    )";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(
        stmt,
        1,
        memberId
    );

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        cout << "No active book found.\n";
        sqlite3_finalize(stmt);
        return;
    }

    int issueId =
        sqlite3_column_int(stmt, 0);

    int bookId =
        sqlite3_column_int(stmt, 1);

    string issueDate =
        reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 2)
        );

    sqlite3_finalize(stmt);

    cout << "\nIssue Date: "
         << issueDate
         << endl;

    int daysKept;

    cout << "Enter number of days kept: ";
    cin >> daysKept;

    int fine = 0;

    if (daysKept > 14)
        fine = (daysKept - 14) * 10;

    executeSQL("BEGIN TRANSACTION;");

    string returnSQL = R"(
        UPDATE issued_books
        SET return_date = CURRENT_TIMESTAMP
        WHERE id = ?;
    )";

    sqlite3_prepare_v2(
        db,
        returnSQL.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(
        stmt,
        1,
        issueId
    );

    sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    string bookUpdate =
        "UPDATE books "
        "SET copies = copies + 1 "
        "WHERE id = ?;";

    sqlite3_prepare_v2(
        db,
        bookUpdate.c_str(),
        -1,
        &stmt,
        nullptr
    );

    sqlite3_bind_int(
        stmt,
        1,
        bookId
    );

    sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    executeSQL("COMMIT;");

    cout << "\nBook returned successfully!\n";
    cout << "Fine: ₹" << fine << endl;
}

void borrowingHistory() {
    string sql = R"(
        SELECT
            m.name,
            b.name,
            i.issue_date,
            i.return_date
        FROM issued_books i
        INNER JOIN members m
            ON i.member_id = m.id
        INNER JOIN books b
            ON i.book_id = b.id
        ORDER BY i.issue_date DESC;
    )";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr
    );

    cout << "\n";
    cout << "==============================================================\n";

    cout << left
         << setw(20) << "MEMBER"
         << setw(25) << "BOOK"
         << setw(22) << "ISSUED"
         << setw(22) << "RETURNED"
         << "\n";

    cout << "==============================================================\n";

    bool found = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;

        string member =
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 0)
            );

        string book =
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 1)
            );

        string issued =
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 2)
            );

        const unsigned char* returned =
            sqlite3_column_text(stmt, 3);

        cout << left
             << setw(20) << member
             << setw(25) << book
             << setw(22) << issued;

        if (returned != nullptr)
            cout << setw(22) << returned;
        else
            cout << setw(22) << "Not Returned";

        cout << "\n";
    }

    if (!found)
        cout << "No borrowing history.\n";

    sqlite3_finalize(stmt);
}

void currentlyIssuedBooks() {
    string sql = R"(
        SELECT
            m.name,
            b.name,
            i.issue_date
        FROM issued_books i
        INNER JOIN members m
            ON i.member_id = m.id
        INNER JOIN books b
            ON i.book_id = b.id
        WHERE i.return_date IS NULL
        ORDER BY i.issue_date;
    )";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(
        db,
        sql.c_str(),
        -1,
        &stmt,
        nullptr
    );

    cout << "\n";
    cout << "====================================================\n";

    cout << left
         << setw(25) << "MEMBER"
         << setw(25) << "BOOK"
         << setw(25) << "ISSUE DATE"
         << "\n";

    cout << "====================================================\n";

    bool found = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;

        cout
            << left
            << setw(25)
            << sqlite3_column_text(stmt, 0)

            << setw(25)
            << sqlite3_column_text(stmt, 1)

            << setw(25)
            << sqlite3_column_text(stmt, 2)

            << "\n";
    }

    if (!found)
        cout << "No books currently issued.\n";

    sqlite3_finalize(stmt);
}

void showMenu() {
    cout << "\n\n";
    cout << "================================================\n";
    cout << "       LIBRARY MANAGEMENT SYSTEM\n";
    cout << "================================================\n";
    cout << "1.  Add Book\n";
    cout << "2.  Display Books\n";
    cout << "3.  Search Book\n";
    cout << "4.  Update Book\n";
    cout << "5.  Delete Book\n";
    cout << "6.  Add Member\n";
    cout << "7.  Display Members\n";
    cout << "8.  Delete Member\n";
    cout << "9.  Issue Book\n";
    cout << "10. Return Book\n";
    cout << "11. Currently Issued Books\n";
    cout << "12. Borrowing History\n";
    cout << "0.  Exit\n";
    cout << "================================================\n";
}

int main() {

    int result =
        sqlite3_open(
            "library.db",
            &db
        );

    if (result != SQLITE_OK) {
        cout << "Database connection failed!\n";
        return 1;
    }

    if (!initializeDatabase()) {
        sqlite3_close(db);
        return 1;
    }

    int choice;

    while (true) {

        showMenu();

        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {

            case 1:
                addBook();
                break;

            case 2:
                displayBooks();
                break;

            case 3:
                searchBook();
                break;

            case 4:
                updateBook();
                break;

            case 5:
                deleteBook();
                break;

            case 6:
                addMember();
                break;

            case 7:
                displayMembers();
                break;

            case 8:
                deleteMember();
                break;

            case 9:
                issueBook();
                break;

            case 10:
                returnBook();
                break;

            case 11:
                currentlyIssuedBooks();
                break;

            case 12:
                borrowingHistory();
                break;

            case 0:
                sqlite3_close(db);
                cout << "\nGoodbye!\n";
                return 0;

            default:
                cout << "Invalid choice. Try again.\n";
        }
    }
}
