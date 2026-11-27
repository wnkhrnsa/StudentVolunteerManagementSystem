#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <regex>

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include "LogRegModule.h"
#include "StudentModule.h"
#include "StaffModule.h"
#include "AdminModule.h"

using namespace std;

int listAndPickByIndex(
    sql::Connection* con,
    const std::string& table,
    const std::string& idCol,
    const std::string& nameCol,
    const std::string& prompt
);

// printHeader ada dalam main.cpp
extern void printHeader(const std::string& title);

//hashing password
static const unsigned int SHA256_K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static inline unsigned int rotr(unsigned int x, unsigned int n) {
    return (x >> n) | (x << (32 - n));
}

static std::string sha256(const std::string& data) { 
    unsigned int h0 = 0x6a09e667;
    unsigned int h1 = 0xbb67ae85;
    unsigned int h2 = 0x3c6ef372;
    unsigned int h3 = 0xa54ff53a;
    unsigned int h4 = 0x510e527f;
    unsigned int h5 = 0x9b05688c;
    unsigned int h6 = 0x1f83d9ab;
    unsigned int h7 = 0x5be0cd19;

    std::string msg = data;
    unsigned long long bit_len = msg.size() * 8ULL;

    msg += (char)0x80;
    while ((msg.size() % 64) != 56) msg += (char)0x00;

    for (int i = 7; i >= 0; --i)
        msg += (char)((bit_len >> (i * 8)) & 0xff);

    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        unsigned int w[64] = { 0 };

        for (int i = 0; i < 16; ++i) {
            w[i] =
                (static_cast<unsigned char>(msg[chunk + i * 4 + 0]) << 24) |
                (static_cast<unsigned char>(msg[chunk + i * 4 + 1]) << 16) |
                (static_cast<unsigned char>(msg[chunk + i * 4 + 2]) << 8) |
                (static_cast<unsigned char>(msg[chunk + i * 4 + 3]) << 0);
        }

        for (int i = 16; i < 64; ++i) {
            unsigned int s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            unsigned int s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        unsigned int a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;

        for (int i = 0; i < 64; i++) {
            unsigned int S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            unsigned int ch = (e & f) ^ (~e & g);
            unsigned int temp1 = h + S1 + ch + SHA256_K[i] + w[i];
            unsigned int S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            unsigned int maj = (a & b) ^ (a & c) ^ (b & c);
            unsigned int temp2 = S0 + maj;

            h = g; g = f; f = e; e = d + temp1; d = c; c = b; b = a; a = temp1 + temp2;
        }

        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e; h5 += f; h6 += g; h7 += h;
    }

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << h0 << std::setw(8) << h1 << std::setw(8) << h2 << std::setw(8) << h3
        << std::setw(8) << h4 << std::setw(8) << h5 << std::setw(8) << h6 << std::setw(8) << h7;

    return ss.str();
}

static bool isValidEmail(const string& email) {
    size_t atPos = email.find('@');
    if (atPos == string::npos || atPos == 0) return false; // tak ada @ / @ kat depan

    size_t dotPos = email.find('.', atPos + 1);
    if (dotPos == string::npos) return false;              // tak ada . lepas @

    if (dotPos == email.length() - 1) return false;        // . kat last char

    return true;
}

static bool isAllowedDomain(const string& email) {
    size_t atPos = email.find('@');
    if (atPos == string::npos) return false;

    string domain = email.substr(atPos + 1);
    return (
        domain == "gmail.com" ||
        domain == "yahoo.com" ||
        domain == "outlook.com" ||
        domain == "student.utem.edu.my"
        );
}

// create user row dalam table `user`
static int createUser(sql::Connection* con, const string& email, const string& pw, const string& role) {
    int userId = -1;
    try {
        sql::PreparedStatement* pstmt = con->prepareStatement(
            "INSERT INTO `user` (Email, Password, Role) VALUES (?, ?, ?)"
        );
        pstmt->setString(1, email);
        pstmt->setString(2, pw);
        pstmt->setString(3, role);
        pstmt->executeUpdate();
        delete pstmt;

        sql::Statement* stmt = con->createStatement();
        sql::ResultSet* res = stmt->executeQuery("SELECT LAST_INSERT_ID() AS id");
        if (res->next()) {
            userId = res->getInt("id");
        }
        delete res;
        delete stmt;
    }
    catch (sql::SQLException& e) {
        if (e.getErrorCode() == 1062) {
            cout << "\n[ERROR] This email is already registered. Please use a different email.\n";
        }
        else {
            cout << "\n[ERROR] Failed to create user account. Please try again.\n";
            cout << "[DEBUG] createUser SQL error: " << e.what() << endl;
        }
        userId = -1;
    }
    return userId;
}

static void createStudentProfile(sql::Connection* con,
    int userId,
    const string& name,
    const string& ic,
    const string& matric,
    const string& phone,
    int facultyId)
{
    if (facultyId <= 0) {
        cout << "Error: Student must be assigned to a faculty.\n";
        return;
    }

    try {
        // --- Generate Student_ID (S001, S002...) ---
        string newId = "S001";
        sql::Statement* stmt = con->createStatement();
        sql::ResultSet* res = stmt->executeQuery(
            "SELECT Student_ID FROM student ORDER BY Student_ID DESC LIMIT 1"
        );
        if (res->next()) {
            string lastId = res->getString("Student_ID");
            int num = stoi(lastId.substr(1));
            num++;

            ostringstream oss;
            oss << 'S' << setw(3) << setfill('0') << num;
            newId = oss.str();
        }
        delete res;
        delete stmt;

        sql::PreparedStatement* pstmt = con->prepareStatement(
            "INSERT INTO student "
            "(Student_ID, User_ID, Name, Matric_No, NRIC, Status, Phone, Faculty_ID) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
        );

        pstmt->setString(1, newId);
        pstmt->setInt(2, userId);
        pstmt->setString(3, name);
        pstmt->setString(4, matric);
        pstmt->setString(5, ic);
        pstmt->setString(6, "Active");
        pstmt->setString(7, phone);
        pstmt->setInt(8, facultyId);

        pstmt->executeUpdate();
        delete pstmt;

        sql::PreparedStatement* ps2 = con->prepareStatement(
            "INSERT INTO studentsummary (Student_ID, Total_hours, Total_points) "
            "VALUES (?, 0, 0)"
        );
        ps2->setString(1, newId);
        ps2->executeUpdate();
        delete ps2;

    }
    catch (sql::SQLException& e) {
        cout << "Error createStudentProfile: " << e.what() << endl;
    }
}


//buat staff profile
static void createStaffProfile(sql::Connection* con,
    int userId,
    const string& name,
    const string& phone,
    int facultyId)
{
    try {
        // --- Generate Staff_ID (ST001, ST002...) ---
        string newId = "ST001";
        sql::Statement* stmt = con->createStatement();
        sql::ResultSet* res = stmt->executeQuery(
            "SELECT Staff_ID FROM staff ORDER BY Staff_ID DESC LIMIT 1"
        );
        if (res->next()) {
            string lastId = res->getString("Staff_ID"); // e.g. ST009
            int num = 0;
            try {
                num = stoi(lastId.substr(2));
            }
            catch (...) {
                num = 0;
            }
            num++;

            std::ostringstream oss;
            oss << "ST" << std::setw(3) << std::setfill('0') << num;
            newId = oss.str();

        }
        delete res;
        delete stmt;

        sql::PreparedStatement* pstmt = con->prepareStatement(
            "INSERT INTO staff (Staff_ID, User_ID, Name, Phone, Faculty_ID, IsAdmin) "
            "VALUES (?, ?, ?, ?, ?, ?)"
        );
        pstmt->setString(1, newId);
        pstmt->setInt(2, userId);
        pstmt->setString(3, name);
        pstmt->setString(4, phone);

        if (facultyId > 0) pstmt->setInt(5, facultyId);
        else pstmt->setNull(5, sql::DataType::INTEGER);

        pstmt->setInt(6, 0);
        pstmt->executeUpdate();
        delete pstmt;
    }
    catch (sql::SQLException& e) {
        cout << "Error createStaffProfile: " << e.what() << endl;
    }
}


//login
void handleLogin(sql::Connection* con) { //READ
    printHeader("LOGIN");

    // Input
    cout << "Enter email: ";
    string email; getline(cin, email);
    cout << "Enter password: ";
    string pw; getline(cin, pw);

    try {
        sql::PreparedStatement* pstmt = con->prepareStatement(
            "SELECT User_ID, Password, Role, IsAdmin, Status FROM `user` WHERE Email = ? LIMIT 1"
        );
        pstmt->setString(1, email);
        sql::ResultSet* res = pstmt->executeQuery();

        if (!res->next()) {
            cout << "\nLogin failed: email not found.\n";
            delete res; delete pstmt;
            cout << "\nPress Enter to return to main menu...";
            string s; getline(cin, s);
            return;
        }

        int userId = res->getInt("User_ID");
        string dbPass = res->getString("Password");
        string role = res->getString("Role");
        string accStatus = "Active";
        try { accStatus = res->getString("Status"); }
        catch (...) { accStatus = "Active"; }

        int isAdmin = 0;
        try { isAdmin = res->getInt("IsAdmin"); }
        catch (...) { isAdmin = 0; }

        delete res;
        delete pstmt;

        // compute hash once
        std::string inputHash = sha256(pw);

        bool loggedIn = false;

        // Case 1: DB already stores hashed password ? compare hash with hash
        if (dbPass == inputHash) {
            loggedIn = true;
        }
        // Case 2: DB still has old plaintext password ? compare plaintext, then migrate
        else if (dbPass == pw) {
            loggedIn = true;

            // Update DB: replace plaintext with hash (best-effort)
            try {
                sql::PreparedStatement* updatePw = con->prepareStatement(
                    "UPDATE `user` SET Password = ? WHERE User_ID = ?"
                );
                updatePw->setString(1, inputHash);
                updatePw->setInt(2, userId);
                updatePw->executeUpdate();
                delete updatePw;
            }
            catch (sql::SQLException& e) {
                cout << "Warning: failed to update hashed password: " << e.what() << endl;
            }
        }
        // lepas confirm password betu
        // Case 3: wrong password
        else {
            cout << "\nLogin failed: incorrect password.\n";
            cout << "\nPress Enter to return to main menu...";
            string s; getline(cin, s);
            return;
        }
        if (accStatus != "Active") {
            cout << "\nYour account is deactivated. Please contact admin.\n";
            cout << "\nPress Enter to return to main menu...";
            string s; getline(cin, s);
            return;
        }
        // If we reach here, login OK
        cout << "\nLogin successful.\n";

        // normalize role string to lowercase safely
        string role_lc = role;
        for (char& c : role_lc) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (role_lc == "student") {
            studentMenu(con, userId);
        }
        else if (role_lc == "staff") {
            if (isAdmin == 1) {
                int m = 0;
                while (true) {
                    cout << "\nYou have admin privileges. Choose session mode:\n";
                    cout << "1) Staff mode\n";
                    cout << "2) Admin mode\n";
                    cout << "Choose (1 or 2): ";

                    if (!(cin >> m)) {
                        cin.clear();
                        string tmp; getline(cin, tmp);
                        cout << "Invalid input. Please enter 1 or 2.\n\n";
                        continue;
                    }
                    string junk; getline(cin, junk);

                    if (m != 1 && m != 2) {
                        cout << "Please enter 1 or 2 only.\n\n";
                        continue;
                    }
                    break;
                }

                if (m == 2) {
                    adminMenu(con, userId);
                }
                else {
                    staffMenu(con, userId);
                }
            }
            else {
                staffMenu(con, userId);
            }
        }
    }
    catch (sql::SQLException& e) {
        cout << "Login error: " << e.what() << endl;
    }

    cout << "\nPress Enter to return to main menu...";
    string s; getline(cin, s);
}



// ============= FUNGSI PUBLIC DIPANGGIL DARI main.cpp =============

void handleRegistration(sql::Connection* con) {
    printHeader("REGISTER");
    cout << "Pick a role (type 'student' or 'staff'):\n";
    cout << "Role: ";
    string role;
    getline(cin, role);
    for (auto& c : role) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    while (role != "student" && role != "staff") {
        cout << "Invalid. Enter 'student' or 'staff': ";
        getline(cin, role);
        for (auto& c : role) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }

    string email;
    while (true) {
        cout << "\nEnter email: ";
        getline(cin, email);

        if (!isValidEmail(email)) {
            cout << "[ERROR] Invalid email format. Try again.\n";
            continue;
        }

        if (!isAllowedDomain(email)) {
            cout << "[ERROR] Please use an appropriate domain.\n";
            continue;
        }

        sql::PreparedStatement* check = con->prepareStatement(
            "SELECT 1 FROM `user` WHERE Email = ? LIMIT 1"
        );
        check->setString(1, email);
        sql::ResultSet* res = check->executeQuery();

        if (res->next()) {
            cout << "[ERROR] Email already exists. Please use a different one.\n";
            delete res; delete check;
            continue;
        }

        delete res; delete check;
        break;
    }

    cout << "Create password: ";
    string pw; getline(cin, pw);
    string hashedPw = sha256(pw);

    string name, ic, matric, phone;
    int facultyId = -1; // <<< declare OUTSIDE so available later

    if (role == "student") {
        cout << "\nRole : Student\n\n";

        cout << "Enter Name : ";
        getline(cin, name);

        cout << "IC No. : ";
        getline(cin, ic);

        // ===== Matric validation =====
        bool matricOk = false;
        while (!matricOk) {
            cout << "Matric No. : ";
            getline(cin, matric);

            if (matric.empty()) {
                cout << "[ERROR] Matric number cannot be empty.\n";
                continue;
            }

            sql::PreparedStatement* ps = con->prepareStatement(
                "SELECT 1 FROM student WHERE Matric_No = ? LIMIT 1"
            );
            ps->setString(1, matric);
            sql::ResultSet* rs = ps->executeQuery();

            if (rs->next()) {
                cout << "[ERROR] Matric number already exists. Please enter a different matric number.\n";
            }
            else {
                matricOk = true;
            }

            delete rs;
            delete ps;
        }

        cout << "Phone No. : ";
        getline(cin, phone);

        // ===== Faculty wajib =====
        facultyId = listAndPickByIndex(
            con,
            "faculty",
            "Faculty_ID",
            "FacultyCode",
            "Faculty"
        );

        while (facultyId == 0) {
            cout << "[ERROR] Student must be assigned to a faculty.\n";
            facultyId = listAndPickByIndex(
                con,
                "faculty",
                "Faculty_ID",
                "FacultyCode",
                "Faculty"
            );
        }
    }
    else { // staff
        cout << "\nRole : Staff\n\n";
        cout << "Enter Name : ";   getline(cin, name);
        cout << "Phone No. : ";    getline(cin, phone);

        cout << "Faculty Code (e.g. FTMK or empty) : ";
        string facultyCode; getline(cin, facultyCode);

        try {
            sql::PreparedStatement* p = con->prepareStatement(
                "SELECT Faculty_ID FROM faculty WHERE FacultyCode = ? LIMIT 1"
            );
            p->setString(1, facultyCode);
            sql::ResultSet* r = p->executeQuery();
            if (r->next()) facultyId = r->getInt("Faculty_ID");
            else cout << "No faculty provided. Staff will be assigned under UNIVERSITY.\n";
            delete r; delete p;
        }
        catch (sql::SQLException& e) {
            cout << "Faculty lookup error: " << e.what() << endl;
            facultyId = -1;
        }
    }

    int userId = createUser(con, email, hashedPw, role);

    if (userId == -1) {
        cout << "\n[ERROR] Failed to create user account.\n";
    }
    else {
        if (role == "student") {
            createStudentProfile(con, userId, name, ic, matric, phone, facultyId);
        }
        else {
            createStaffProfile(con, userId, name, phone, facultyId);
        }
        cout << "\n------------------------------------------------------------\n";
        cout << "REGISTRATION SUCCESSFUL\n";
    }

    cout << "Press Enter to return to main menu...";
    string tmp; getline(cin, tmp);
}

void updateEventStatus(sql::Connection* con)
{
    sql::Statement* stmt = con->createStatement();

    // ===== UPDATE EVENT STATUS =====
    stmt->execute(
        "UPDATE event SET Status = 'Active' "
        "WHERE Status <> 'Cancelled' "
        "AND NOW() >= EventDate "
        "AND NOW() < DATE_ADD(EventDate, INTERVAL DurationHours HOUR)"
    );

    stmt->execute(
        "UPDATE event SET Status = 'Ended' "
        "WHERE Status <> 'Cancelled' "
        "AND NOW() >= DATE_ADD(EventDate, INTERVAL DurationHours HOUR)"
    );

    stmt->execute(
        "UPDATE event SET Status = 'Upcoming' "
        "WHERE Status <> 'Cancelled' "
        "AND NOW() < EventDate"
    );
}

static int listAndPickByIndex(sql::Connection* con, const std::string& table,
    const std::string& idCol, const std::string& nameCol,
    const std::string& prompt)
{
    std::vector<int> realIds;
    try {
        std::string q = "SELECT " + idCol + ", " + nameCol +
            " FROM " + table + " ORDER BY " + idCol;
        sql::Statement* st = con->createStatement();
        sql::ResultSet* rs = st->executeQuery(q);

        cout << "\nAvailable " << prompt << ":\n";
        int idx = 1;
        while (rs->next()) {
            int rid = rs->getInt(idCol);
            std::string rname = rs->getString(nameCol);
            cout << idx << ". " << rname << " \n";
            realIds.push_back(rid);
            idx++;
        }
        if (realIds.empty()) cout << "(No " << prompt << " found)\n";

        delete rs;
        delete st;
    }
    catch (sql::SQLException& e) {
        cout << "Warning: cannot fetch " << prompt << ": " << e.what() << "\n";
    }

    // ask user pick
    cout << "\nChoose " << prompt << " number (blank = skip): ";
    string in; getline(cin, in);
    if (in.empty()) return 0;
    try {
        int pick = stoi(in);
        if (pick == 0) return 0;
        if (pick > 0 && pick <= (int)realIds.size()) return realIds[pick - 1];
        cout << "Invalid selection — ignoring.\n";
        return 0;
    }
    catch (...) {
        cout << "Invalid input — ignoring.\n";
        return 0;
    }
}
