#include <iostream>
#include <string>
#include <iomanip>
#include <vector>
#include <map>

#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>

#include "AdminModule.h"
#include "StudentModule.h"

using namespace std;

// printHeader ada dalam main.cpp
extern void printHeader(const std::string& title);

// ------ forward declaration submenu (dalam file ni je) ------
static void manageStaffMenu(sql::Connection* con, int userId);
static void manageFacultyMenu(sql::Connection* con);
static void manageEventViewMenu(sql::Connection* con);
static void manageStudentMenu(sql::Connection* con);
static void generateReportsPlaceholder(sql::Connection* con);
static void viewStudentProfileSummary(sql::Connection* con, const std::string& studentId);
static void generateYearlyReport(sql::Connection* con);
static void generateFacultyStudentRanking(sql::Connection* con);
// ================== ADMIN MENU UTAMA ==================

void adminMenu(sql::Connection* con, int userId)
{
    while (true) {
        printHeader("ADMIN MENU");
        cout << "1. Manage staff\n";
        cout << "2. View events\n";
        cout << "3. View students\n";
        cout << "4. Generate reports\n";
        cout << "5. Back to main menu\n";

        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            string junk; getline(cin, junk);
            continue;
        }
        string junk; getline(cin, junk); // clear newline

        if (choice == 1) {
            manageStaffMenu(con, userId);
        }
        else if (choice == 2) {
            manageEventViewMenu(con);
        }
        else if (choice == 3) {
            manageStudentMenu(con);
        }
        else if (choice == 4) {
            generateReportsPlaceholder(con);   
        }
        else if (choice == 5) {
            break; // balik ke main menu
        }
        else {
            cout << "\nInvalid choice.\n";
            cout << "Press Enter to continue..."; getline(cin, junk);
        }
    }
}

static void manageStaffMenu(sql::Connection* con, int userId)
{
    while (true) {
        printHeader("MANAGE STAFF");

        cout << "1. View all staff\n";
        cout << "2. Filter staff by faculty\n";
        cout << "3. Search staff by name or staff ID\n";
        cout << "4. Edit staff info\n";
        cout << "5. Deactivate staff\n";
        cout << "6. Activate staff\n";
        cout << "7. Back\n\n";

        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            string junk; getline(cin, junk);
            continue;
        }
        string junk; getline(cin, junk);

        if (choice == 1) {
            try {
                sql::Statement* stmt = con->createStatement();
                sql::ResultSet* rs = stmt->executeQuery(
                    "SELECT s.Staff_ID, u.Email, s.Name, "
                    "f.FacultyCode AS Faculty, s.IsAdmin, u.Status "
                    "FROM staff s "
                    "JOIN user u ON s.User_ID = u.User_ID "
                    "LEFT JOIN faculty f ON s.Faculty_ID = f.Faculty_ID "
                    "ORDER BY s.Staff_ID"
                );

                printHeader("ALL STAFF");

                while (rs->next()) {
                    cout << "ID      : " << rs->getString("Staff_ID") << "\n";
                    cout << "Email   : " << rs->getString("Email") << "\n";
                    cout << "Name    : " << rs->getString("Name") << "\n";

                    cout << "Faculty : ";
                    if (rs->isNull("Faculty"))
                        cout << "University\n";
                    else
                        cout << rs->getString("Faculty") << "\n";

                    cout << "Admin   : "
                        << (rs->getInt("IsAdmin") ? "YES" : "NO") << "\n";
                    cout << "Status  : " << rs->getString("Status") << "\n";
                    cout << "--------------------------------------------\n";
                }

                delete rs;
                delete stmt;
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }

        else if (choice == 2) {
            // FILTER STAFF BY FACULTY (USER FRIENDLY & SAFE)

            try {
                printHeader("SELECT FACULTY");

                // 1️⃣ Grab faculty list into vector
                vector<pair<int, string>> facultyList;
                sql::Statement* st = con->createStatement();
                sql::ResultSet* rs = st->executeQuery(
                    "SELECT Faculty_ID, FacultyCode FROM faculty ORDER BY Faculty_ID"
                );

                while (rs->next()) {
                    facultyList.push_back({
                        rs->getInt("Faculty_ID"),
                        rs->getString("FacultyCode")
                        });
                }

                delete rs;
                delete st;

                // 2️⃣ Display numbered list
                if (facultyList.empty()) {
                    cout << "No faculty found.\n";
                    cout << "Press Enter to continue...";
                    getline(cin, junk);
                    continue;
                }

                for (int i = 0; i < facultyList.size(); i++) {
                    cout << (i + 1) << ". " << facultyList[i].second << "\n";
                }

                // 3️⃣ Add "University / No Faculty" as last index
                cout << (facultyList.size() + 1) << ". UNIVERSITY (NO FACULTY)\n";

                cout << "\nEnter choice (0 = cancel): ";
                int pick;
                cin >> pick;
                getline(cin, junk);

                if (pick == 0) continue;

                int nullIndex = facultyList.size() + 1;

                sql::PreparedStatement* ps;

                // 4️⃣ Filter logic
                if (pick == nullIndex) {
                    // NO FACULTY
                    ps = con->prepareStatement(
                        "SELECT s.Staff_ID, u.Email, s.Name, s.IsAdmin, u.Status "
                        "FROM staff s "
                        "JOIN user u ON s.User_ID = u.User_ID "
                        "WHERE s.Faculty_ID IS NULL "
                        "ORDER BY s.Staff_ID"
                    );
                }
                else if (pick > 0 && pick <= facultyList.size()) {
                    // Normal faculty via map
                    int facultyId = facultyList[pick - 1].first;

                    ps = con->prepareStatement(
                        "SELECT s.Staff_ID, u.Email, s.Name, s.IsAdmin, u.Status "
                        "FROM staff s "
                        "JOIN user u ON s.User_ID = u.User_ID "
                        "WHERE s.Faculty_ID = ? "
                        "ORDER BY s.Staff_ID"
                    );
                    ps->setInt(1, facultyId);
                }
                else {
                    cout << "Invalid choice.\n";
                    cout << "Press Enter to continue...";
                    getline(cin, junk);
                    continue;
                }

                // 5️⃣ Display result
                rs = ps->executeQuery();
                printHeader("STAFF BY FACULTY");

                bool any = false;
                while (rs->next()) {
                    any = true;
                    cout << "ID     : " << rs->getString("Staff_ID") << "\n";
                    cout << "Email  : " << rs->getString("Email") << "\n";
                    cout << "Name   : " << rs->getString("Name") << "\n";
                    cout << "Admin  : " << (rs->getInt("IsAdmin") ? "YES" : "NO") << "\n";
                    cout << "Status : " << rs->getString("Status") << "\n";
                    cout << "-----------------------------\n";
                }

                if (!any) {
                    cout << "No staff found.\n";
                }

                delete rs;
                delete ps;
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }
        
        else if (choice == 3) { // SEARCH STAFF INFO
            printHeader("SEARCH STAFF INFO");

            cout << "Enter name or staff ID (0 = cancel): ";
            string q;
            getline(cin, q);

            if (q == "0") continue;

            sql::PreparedStatement* ps;

            // Detect Staff ID (e.g. ST003)
            bool isStaffId = false;
            if (q.size() >= 3) {
                string prefix = q.substr(0, 2);
                for (auto& c : prefix) c = toupper(c);
                if (prefix == "ST") isStaffId = true;
            }

            if (isStaffId) {
                ps = con->prepareStatement(
                    "SELECT s.Staff_ID, u.User_ID, u.Email, s.Name, s.IsAdmin, u.Status, f.FacultyCode "
                    "FROM staff s "
                    "JOIN user u ON s.User_ID = u.User_ID "
                    "LEFT JOIN faculty f ON f.Faculty_ID = s.Faculty_ID "
                    "WHERE s.Staff_ID = ?"
                );
                ps->setString(1, q);
            }
            else {
                ps = con->prepareStatement(
                    "SELECT s.Staff_ID, u.User_ID, u.Email, s.Name, s.IsAdmin, u.Status, f.FacultyCode "
                    "FROM staff s "
                    "JOIN user u ON s.User_ID = u.User_ID "
                    "LEFT JOIN faculty f ON f.Faculty_ID = s.Faculty_ID "
                    "WHERE s.Name LIKE CONCAT('%', ?, '%')"
                );
                ps->setString(1, q);
            }

            sql::ResultSet* rs = ps->executeQuery();
            printHeader("SEARCH RESULT");

            bool any = false;
            while (rs->next()) {
                any = true;

                string staffId = rs->getString("Staff_ID").c_str();
                int userId = rs->getInt("User_ID");

                cout << "ID     : " << staffId << "\n";
                cout << "Email  : " << rs->getString("Email") << "\n";
                cout << "Name   : " << rs->getString("Name") << "\n";

                string fac = "";
                if (!rs->isNull("FacultyCode"))
                    fac = rs->getString("FacultyCode").c_str();
                if (fac.empty()) fac = "University";

                cout << "Faculty: " << fac << "\n";
                cout << "Admin  : " << (rs->getInt("IsAdmin") ? "YES" : "NO") << "\n";
                cout << "Status : " << rs->getString("Status") << "\n";
                cout << "-----------------------------\n";

                // ===================== DISPLAY EVENTS ORGANISED ======================
                sql::PreparedStatement* ep = con->prepareStatement(
                    "SELECT e.Event_ID, e.Title, e.Status, e.EventDate, "
                    "e.DurationHours, e.Capacity, e.Attendance_Passcode, "
                    "e.Faculty_ID, c.Name, f.FacultyCode "
                    "FROM event e "
                    "LEFT JOIN category c ON e.Category_ID = c.Category_ID "
                    "LEFT JOIN faculty f ON e.Faculty_ID = f.Faculty_ID "
                    "WHERE e.Organizer_User_ID = ? "
                    "ORDER BY e.Event_ID"
                );
                ep->setInt(1, userId);
                sql::ResultSet* ers = ep->executeQuery();

                cout << "\nEvents organized:\n";
                bool hasEvent = false;

                while (ers->next()) {
                    hasEvent = true;

                    cout << "Event_ID   : " << ers->getInt("Event_ID") << "\n";
                    cout << "Title      : " << ers->getString("Title") << "\n";
                    cout << "Status     : " << ers->getString("Status") << "\n";
                    cout << "Category   : " << ers->getString("Name") << "\n";

                    if (ers->isNull("Faculty_ID"))
                        cout << "Type       : University\n";
                    else
                        cout << "Type       : Faculty (" << ers->getString("FacultyCode") << ")\n";

                    cout << "Event Date : " << ers->getString("EventDate") << "\n";
                    cout << "Duration   : " << ers->getDouble("DurationHours") << " hours\n";
                    cout << "Capacity   : " << ers->getInt("Capacity") << "\n";
                    cout << "Passcode   : " << ers->getString("Attendance_Passcode") << "\n";
                    cout << "-----------------------------\n";
                }

                if (!hasEvent)
                    cout << "<No event organized>\n";

                delete ers;
                delete ep;
            }

            if (!any)
                cout << "No staff found.\n";

            delete rs;
            delete ps;

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }


        else if (choice == 4) {
            printHeader("EDIT STAFF INFO");

            // ===== SHOW STAFF LIST =====
            try {
                sql::Statement* st = con->createStatement();
                sql::ResultSet* rs = st->executeQuery(
                    "SELECT s.Staff_ID, s.Name, f.FacultyCode, s.IsAdmin "
                    "FROM staff s "
                    "LEFT JOIN faculty f ON s.Faculty_ID = f.Faculty_ID "
                    "ORDER BY s.Staff_ID"
                );

                cout << "Available Staff:\n";
                cout << left
                    << setw(8) << "ID" << " | "
                    << setw(22) << "Name" << " | "
                    << setw(12) << "Faculty" << " | "
                    << "Admin" << "\n";

                cout << string(65, '-') << "\n";

                bool any = false;

                while (rs->next()) {
                    any = true;

                    string fac;
                    if (rs->isNull("FacultyCode"))
                        fac = "University";
                    else
                        fac = string(rs->getString("FacultyCode").c_str());

                    cout << left
                        << setw(8) << string(rs->getString("Staff_ID").c_str()) << " | "
                        << setw(22) << string(rs->getString("Name").c_str()) << " | "
                        << setw(12) << fac << " | "
                        << (rs->getInt("IsAdmin") ? "YES" : "NO") << "\n";
                }

                if (!any) cout << "<NO STAFF>\n";

                cout << string(65, '-') << "\n";

                delete rs;
                delete st;
            }
            catch (sql::SQLException& e) {
                cout << "Error loading staff list: " << e.what() << endl;
                cout << "Press Enter to continue...";
                getline(cin, junk);
                continue;
            }

            // ===== INPUT STAFF ID =====
            string staffId;
            cout << "Enter Staff_ID to edit (0 = cancel): ";
            getline(cin, staffId);

            if (staffId == "0") continue;

            try {
                // ===== FETCH CURRENT INFO =====
                sql::PreparedStatement* ps = con->prepareStatement(
                    "SELECT Name, Faculty_ID, IsAdmin FROM staff WHERE Staff_ID = ?"
                );
                ps->setString(1, staffId);
                sql::ResultSet* rs = ps->executeQuery();

                if (!rs->next()) {
                    cout << "Invalid Staff_ID.\n";
                    delete rs; delete ps;
                    cout << "Press Enter to continue...";
                    getline(cin, junk);
                    continue;
                }

                string currName = string(rs->getString("Name").c_str());
                int currFaculty = rs->getInt("Faculty_ID");
                int currAdmin = rs->getInt("IsAdmin");

                delete rs;
                delete ps;

                // ===== SHOW CURRENT =====
                cout << "\nCurrent Name      : " << currName << "\n";
                string currFacStr;
                sql::PreparedStatement* facPs = con->prepareStatement(
                    "SELECT FacultyCode FROM faculty WHERE Faculty_ID = ?"
                );
                facPs->setInt(1, currFaculty);
                sql::ResultSet* facRs = facPs->executeQuery();
                if (facRs->next())
                    currFacStr = facRs->getString("FacultyCode").c_str();
                else
                    currFacStr = "University";

                delete facRs;
                delete facPs;

                cout << "Current Faculty : " << currFacStr << "\n";
                cout << "Current Admin     : " << (currAdmin ? "YES" : "NO") << "\n\n";

                // ===== INPUT NEW =====
                string newName;
                int newFaculty = currFaculty;
                int newAdmin;

                cout << "Enter new name (leave empty to keep same): ";
                getline(cin, newName);
                if (newName.empty()) newName = currName;

                // ===== FACULTY SELECTION =====
                sql::Statement* fst = con->createStatement();
                sql::ResultSet* frs = fst->executeQuery(
                    "SELECT Faculty_ID, FacultyCode, FacultyName FROM faculty ORDER BY Faculty_ID"
                );

                vector<int> facultyMap;

                cout << "\nSelect new Faculty (Enter = keep current):\n\n";

                int counter = 1;
                while (frs->next()) {
                    cout << counter << ". "
                        << frs->getString("FacultyCode").c_str()
                        << " - "
                        << frs->getString("FacultyName").c_str()
                        << "\n";

                    facultyMap.push_back(frs->getInt("Faculty_ID"));
                    counter++;
                }

                // Add university
                cout << counter << ". University (No Faculty)\n";

                delete frs;
                delete fst;

                string fInput;
                cout << "\nEnter choice (Enter = keep, 0 = cancel): ";
                getline(cin, fInput);

                if (!fInput.empty()) {
                    int pick = stoi(fInput);

                    if (pick == 0) {
                        continue;
                    }
                    else if (pick == counter) {
                        newFaculty = -1; // NULL
                    }
                    else if (pick > 0 && pick < counter) {
                        newFaculty = facultyMap[pick - 1];
                    }
                    else {
                        cout << "Invalid choice.\n";
                        cout << "Press Enter to continue...";
                        getline(cin, junk);
                        continue;
                    }
                }

                // ===== ADMIN =====
                string adminInput;
                cout << "Grant admin power? (1 = Yes, 0 = No, Enter = keep): ";
                getline(cin, adminInput);

                if (adminInput.empty())
                    newAdmin = currAdmin;
                else if (adminInput == "0" || adminInput == "1")
                    newAdmin = stoi(adminInput);
                else {
                    cout << "Invalid admin value.\n";
                    cout << "Press Enter to continue...";
                    getline(cin, junk);
                    continue;
                }

                // ===== UPDATE STAFF =====
                if (newFaculty == -1) {
                    ps = con->prepareStatement(
                        "UPDATE staff SET Name=?, Faculty_ID=NULL, IsAdmin=? WHERE Staff_ID=?"
                    );
                    ps->setString(1, newName);
                    ps->setInt(2, newAdmin);
                    ps->setString(3, staffId);
                }
                else {
                    ps = con->prepareStatement(
                        "UPDATE staff SET Name=?, Faculty_ID=?, IsAdmin=? WHERE Staff_ID=?"
                    );
                    ps->setString(1, newName);
                    ps->setInt(2, newFaculty);
                    ps->setInt(3, newAdmin);
                    ps->setString(4, staffId);
                }
                ps->executeUpdate();
                delete ps;

                // ===== UPDATE USER TABLE =====
                ps = con->prepareStatement(
                    "UPDATE user u "
                    "JOIN staff s ON u.User_ID = s.User_ID "
                    "SET u.IsAdmin = ? "
                    "WHERE s.Staff_ID = ?"
                );
                ps->setInt(1, newAdmin);
                ps->setString(2, staffId);
                ps->executeUpdate();
                delete ps;

                cout << "Staff info updated successfully.\n";
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }


        else if (choice == 5) {
            printHeader("DEACTIVATE STAFF");

            string staffId;
            cout << "Enter Staff_ID to deactivate (0 = cancel): ";
            cin >> staffId;
            getline(cin, junk);

            if (staffId == "0") continue;

            char confirm;
            cout << "Confirm deactivate this staff? (y/n): ";
            cin >> confirm;
            getline(cin, junk);

            if (confirm != 'y' && confirm != 'Y') continue;

            try {
                // 1️⃣ deactivate via user table
                sql::PreparedStatement* ps = con->prepareStatement(
                    "UPDATE user u "
                    "JOIN staff s ON u.User_ID = s.User_ID "
                    "SET u.Status = 'Inactive' "
                    "WHERE s.Staff_ID = ? AND u.Status = 'Active'"
                );
                ps->setString(1, staffId);

                int affected = ps->executeUpdate();
                delete ps;

                if (affected > 0)
                    cout << "Staff account deactivated successfully.\n";
                else
                    cout << "Staff account is already inactive or Staff_ID invalid.\n";
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }

        else if (choice == 6) {
            printHeader("ACTIVATE STAFF");

            string staffId;
            cout << "Enter Staff_ID to activate (0 = cancel): ";
            cin >> staffId;
            getline(cin, junk);

            if (staffId == "0") continue;

            try {
                sql::PreparedStatement* ps = con->prepareStatement(
                    "UPDATE user u "
                    "JOIN staff s ON u.User_ID = s.User_ID "
                    "SET u.Status = 'Active' "
                    "WHERE s.Staff_ID = ? AND u.Status = 'Inactive'"
                );
                ps->setString(1, staffId);

                int affected = ps->executeUpdate();
                delete ps;

                if (affected > 0)
                    cout << "Staff account activated successfully.\n";
                else
                    cout << "Staff account is already active or Staff_ID invalid.\n";
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }
        else if (choice == 7) {
            return;  
        }

    }
}

static void manageFacultyMenu(sql::Connection* con)
{
    while (true) {
        printHeader("MANAGE FACULTY");

        cout << "1. View all faculty\n";
        cout << "2. Add faculty\n";
        cout << "3. Delete faculty\n";
        cout << "4. Back\n\n";
        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            string junk; getline(cin, junk);
            continue;
        }
        string junk; getline(cin, junk);

        // ================= VIEW ALL =================
        if (choice == 1) {
            try {
                sql::Statement* stmt = con->createStatement();
                sql::ResultSet* rs = stmt->executeQuery(
                    "SELECT Faculty_ID, FacultyCode, FacultyName FROM faculty ORDER BY Faculty_ID"
                );

                printHeader("FACULTY LIST");
                while (rs->next()) {
                    cout << "ID   : " << rs->getInt("Faculty_ID") << "\n";
                    cout << "Code : " << rs->getString("FacultyCode") << "\n";
                    cout << "Name : " << rs->getString("FacultyName") << "\n";
                    cout << "-----------------------------\n";
                }

                delete rs;
                delete stmt;
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }

        // ================= ADD =================
        else if (choice == 2) {
            printHeader("ADD FACULTY");

            string code, name;
            cout << "Enter Faculty Code: ";
            getline(cin, code);

            cout << "Enter Faculty Name: ";
            getline(cin, name);

            if (code.empty() || name.empty()) {
                cout << "Faculty code and name cannot be empty.\n";
                cout << "Press Enter to continue...";
                getline(cin, junk);
                continue;
            }

            try {
                sql::PreparedStatement* ps = con->prepareStatement(
                    "INSERT INTO faculty (FacultyCode, FacultyName) VALUES (?, ?)"
                );
                ps->setString(1, code);
                ps->setString(2, name);
                ps->executeUpdate();
                delete ps;

                cout << "Faculty added successfully.\n";
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }

        // ================= DELETE =================
        else if (choice == 3) {
            printHeader("DELETE FACULTY");

            int facultyId;
            cout << "Enter Faculty_ID to delete (0 = cancel): ";
            cin >> facultyId;
            getline(cin, junk);

            if (facultyId == 0) continue;

            char confirm;
            cout << "Confirm delete this faculty? (y/n): ";
            cin >> confirm;
            getline(cin, junk);

            if (confirm != 'y' && confirm != 'Y') continue;

            try {
                sql::PreparedStatement* ps = con->prepareStatement(
                    "DELETE FROM faculty WHERE Faculty_ID = ?"
                );
                ps->setInt(1, facultyId);
                int affected = ps->executeUpdate();
                delete ps;

                if (affected > 0)
                    cout << "Faculty deleted successfully.\n";
                else
                    cout << "Invalid Faculty_ID.\n";
            }
            catch (sql::SQLException& e) {
                cout << "Cannot delete faculty.\n";
                cout << "There are students or staff under this faculty.\n";
            }
            cout << "Press Enter to continue...";
            getline(cin, junk);
        }

        else if (choice == 4) {
            break;
        }
    }
}

// ================== VIEW / SEARCH EVENTS ==================

static void manageEventViewMenu(sql::Connection* con)
{
    while (true) {
        printHeader("ADMIN EVENT MENU");

        cout << "1. Search event by title\n";
        cout << "2. Filter events\n";
        cout << "3. Back\n\n";
        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            string junk; getline(cin, junk);
            continue;
        }
        string junk; getline(cin, junk);

        // ================= SEARCH BY TITLE =================
        if (choice == 1) {
            printHeader("SEARCH EVENT BY TITLE");

            string keyword;
            cout << "Enter keyword (empty = cancel): ";
            getline(cin, keyword);

            if (keyword.empty()) continue;

            try {
                sql::PreparedStatement* ps = con->prepareStatement(
                    "SELECT e.Event_ID, e.Title, e.EventDate, e.DurationHours, "
                    "e.Location, e.Capacity, e.Status, "
                    "c.Name AS CategoryName, "
                    "s.Staff_ID, s.Name AS OrganizerName, "
                    "f.FacultyCode "
                    "FROM event e "
                    "LEFT JOIN category c ON e.Category_ID = c.Category_ID "
                    "LEFT JOIN staff s ON e.Organizer_User_ID = s.User_ID "
                    "LEFT JOIN faculty f ON e.Faculty_ID = f.Faculty_ID "
                    "WHERE e.Title LIKE ?"
                );

                ps->setString(1, "%" + keyword + "%");
                sql::ResultSet* rs = ps->executeQuery();

                bool any = false;
                while (rs->next()) {
                    any = true;

                    cout << "ID        : " << rs->getInt("Event_ID") << "\n";
                    cout << "Title     : " << rs->getString("Title") << "\n";

                    cout << "Category  : "
                        << (rs->isNull("CategoryName") ? "N/A" : rs->getString("CategoryName")) << "\n";

                    cout << "Organizer : ";
                    if (rs->isNull("Staff_ID"))
                        cout << "N/A\n";
                    else
                        cout << rs->getString("Staff_ID") << " - "
                        << rs->getString("OrganizerName") << "\n";

                    // EVENT TYPE
                    if (rs->isNull("FacultyCode")) {
                        cout << "Event Type: University\n";
                    }
                    else {
                        cout << "Event Type: Faculty\n";
                        cout << "Faculty   : " << rs->getString("FacultyCode") << "\n";
                    }
                    cout << "Date      : " << rs->getString("EventDate") << "\n";
                    cout << "Duration  : " << rs->getInt("DurationHours") << " hours\n";
                    cout << "Location  : " << rs->getString("Location") << "\n";
                    cout << "Capacity  : " << rs->getInt("Capacity") << "\n";
                    cout << "Status    : " << rs->getString("Status") << "\n";
                    cout << "---------------------------------------------\n";
                }

                if (!any)
                    cout << "No matching events found.\n";

                delete rs;
                delete ps;
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }

        // ================= FILTER EVENTS =================
        else if (choice == 2) {
            printHeader("FILTER EVENTS");

            int categoryId = 0;
            int facultyId = 0;
            string status, type;

            try {
                // ===== SHOW CATEGORY LIST =====
                cout << "Available Categories:\n";
                sql::Statement* st = con->createStatement();
                sql::ResultSet* rs = st->executeQuery(
                    "SELECT Category_ID, Name FROM category ORDER BY Category_ID"
                );

                while (rs->next()) {
                    cout << rs->getInt("Category_ID")
                        << " - " << rs->getString("Name") << "\n";
                }
                cout << "0 - Skip\n\n";

                delete rs;
                delete st;

                cout << "Enter Category ID : ";
                cin >> categoryId;
                getline(cin, junk);

                cout << "Enter Status (Active / Ended / Cancelled, empty = skip): ";
                getline(cin, status);

                cout << "Enter Event Type (University / Faculty, empty = skip): ";
                getline(cin, type);

                // ===== SHOW FACULTY LIST IF NEEDED =====
                if (type == "Faculty") {
                    cout << "\nAvailable Faculties:\n";
                    st = con->createStatement();
                    rs = st->executeQuery(
                        "SELECT Faculty_ID, FacultyCode FROM faculty ORDER BY Faculty_ID"
                    );

                    while (rs->next()) {
                        cout << rs->getInt("Faculty_ID")
                            << " - " << rs->getString("FacultyCode") << "\n";
                    }
                    cout << "0 - Skip\n\n";

                    delete rs;
                    delete st;

                    cout << "Enter Faculty ID : ";
                    cin >> facultyId;
                    getline(cin, junk);
                }

                // ===== BUILD QUERY =====
                string query =
                    "SELECT e.Event_ID, e.Title, e.Status, "
                    "c.Name AS CategoryName, "
                    "f.FacultyCode "
                    "FROM event e "
                    "LEFT JOIN category c ON e.Category_ID = c.Category_ID "
                    "LEFT JOIN faculty f ON e.Faculty_ID = f.Faculty_ID";

                bool first = true;

                if (categoryId != 0) {
                    query += first ? " WHERE " : " AND ";
                    query += "e.Category_ID = ?";
                    first = false;
                }

                if (!status.empty()) {
                    query += first ? " WHERE " : " AND ";
                    query += "e.Status = ?";
                    first = false;
                }

                if (type == "University") {
                    query += first ? " WHERE " : " AND ";
                    query += "e.Faculty_ID IS NULL";
                    first = false;
                }
                else if (type == "Faculty") {
                    query += first ? " WHERE " : " AND ";
                    query += "e.Faculty_ID IS NOT NULL";
                    first = false;
                }

                if (type == "Faculty" && facultyId != 0) {
                    query += first ? " WHERE " : " AND ";
                    query += "e.Faculty_ID = ?";
                    first = false;
                }

                sql::PreparedStatement* ps = con->prepareStatement(query);
                int idx = 1;

                if (categoryId != 0)
                    ps->setInt(idx++, categoryId);
                if (!status.empty())
                    ps->setString(idx++, status);
                if (type == "Faculty" && facultyId != 0)
                    ps->setInt(idx++, facultyId);

                rs = ps->executeQuery();

                // ===== DISPLAY RESULT =====
                printHeader("FILTERED EVENTS");

                bool any = false;
                while (rs->next()) {
                    any = true;

                    cout << "ID        : " << rs->getInt("Event_ID") << "\n";
                    cout << "Title     : " << rs->getString("Title") << "\n";
                    cout << "Category  : "
                        << (rs->isNull("CategoryName") ? "N/A" : rs->getString("CategoryName")) << "\n";

                    if (rs->isNull("FacultyCode")) {
                        cout << "Event Type: University\n";
                    }
                    else {
                        cout << "Event Type: Faculty\n";
                        cout << "Faculty   : " << rs->getString("FacultyCode") << "\n";
                    }

                    cout << "Status    : " << rs->getString("Status") << "\n";
                    cout << "-----------------------------\n";
                }

                if (!any)
                    cout << "No events matched your filter.\n";

                delete rs;
                delete ps;
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }
        else if (choice == 3) {
            break;
        }
    }
}

static void manageStudentMenu(sql::Connection* con)
{
    while (true) {
        printHeader("MANAGE STUDENTS");

        cout << "1. Search student by name / matric / student ID\n";
        cout << "2. Filter students by faculty\n";
        cout << "3. Activate student account\n";
        cout << "4. Deactivate student account\n";
        cout << "5. Edit student info\n";
        cout << "6. View student report\n";
        cout << "7. Back\n\n";
        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            string junk; getline(cin, junk);
            continue;
        }
        string junk; getline(cin, junk);

        // ================= SEARCH STUDENT =================
        if (choice == 1) {
            printHeader("SEARCH STUDENT");

            string keyword;
            cout << "Enter name or matric or student ID (empty = cancel): ";
            getline(cin, keyword);

            if (keyword.empty()) continue;

            try {
                sql::PreparedStatement* ps = con->prepareStatement(
                    "SELECT s.Student_ID, s.Name, s.Matric_No, s.NRIC, s.Phone, "
                    "u.Email, u.Status AS AccountStatus, "
                    "f.FacultyCode "
                    "FROM student s "
                    "JOIN user u ON s.User_ID = u.User_ID "
                    "LEFT JOIN faculty f ON s.Faculty_ID = f.Faculty_ID "
                    "WHERE s.Name LIKE ? "
                    "   OR s.Matric_No LIKE ? "
                    "   OR s.Student_ID LIKE ? "
                    "ORDER BY s.Student_ID"
                );

                ps->setString(1, "%" + keyword + "%");
                ps->setString(2, "%" + keyword + "%");
                ps->setString(3, "%" + keyword + "%");

                sql::ResultSet* rs = ps->executeQuery();

                bool any = false;
                while (rs->next()) {
                    any = true;

                    cout << "Student ID : " << rs->getString("Student_ID") << "\n";
                    cout << "Name       : " << rs->getString("Name") << "\n";
                    cout << "Matric No  : " << rs->getString("Matric_No") << "\n";
                    cout << "NRIC       : " << rs->getString("NRIC") << "\n";
                    cout << "Phone      : " << rs->getString("Phone") << "\n";
                    cout << "Email      : " << rs->getString("Email") << "\n";

                    cout << "Faculty    : "
                        << (rs->isNull("FacultyCode") ? "N/A" : rs->getString("FacultyCode")) << "\n";

                    cout << "Status     : " << rs->getString("AccountStatus") << "\n";
                    cout << "---------------------------------------------\n";
                }

                if (!any)
                    cout << "No student found.\n";

                delete rs;
                delete ps;
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }

        // ================= FILTER STUDENTS BY FACULTY =================
        else if (choice == 2) {
            printHeader("FILTER STUDENTS BY FACULTY");

            int facultyId = 0;

            try {
                // ===== SHOW FACULTY LIST =====
                cout << "Available Faculties:\n";
                sql::Statement* st = con->createStatement();
                sql::ResultSet* rs = st->executeQuery(
                    "SELECT Faculty_ID, FacultyCode FROM faculty ORDER BY Faculty_ID"
                );

                while (rs->next()) {
                    cout << rs->getInt("Faculty_ID")
                        << " - " << rs->getString("FacultyCode") << "\n";
                }
                cout << "0 - Cancel\n\n";

                delete rs;
                delete st;

                cout << "Enter Faculty_ID (0 = cancel): ";
                cin >> facultyId;
                getline(cin, junk);

                if (facultyId == 0) continue;

                // ===== FILTER STUDENTS =====
                sql::PreparedStatement* ps = con->prepareStatement(
                    "SELECT s.Student_ID, s.Name, s.Matric_No, "
                    "u.Status AS AccountStatus, "
                    "f.FacultyCode "
                    "FROM student s "
                    "JOIN user u ON s.User_ID = u.User_ID "
                    "LEFT JOIN faculty f ON s.Faculty_ID = f.Faculty_ID "
                    "WHERE s.Faculty_ID = ? "
                    "ORDER BY s.Student_ID"
                );
                ps->setInt(1, facultyId);

                rs = ps->executeQuery();

                printHeader("STUDENTS BY FACULTY");

                bool any = false;
                while (rs->next()) {
                    any = true;

                    cout << "Student ID : " << rs->getString("Student_ID") << "\n";
                    cout << "Name       : " << rs->getString("Name") << "\n";
                    cout << "Matric No  : " << rs->getString("Matric_No") << "\n";
                    cout << "Faculty    : "
                        << (rs->isNull("FacultyCode") ? "N/A" : rs->getString("FacultyCode")) << "\n";
                    cout << "Status     : " << rs->getString("AccountStatus") << "\n";
                    cout << "---------------------------------------------\n";
                }

                if (!any)
                    cout << "No students found for this faculty.\n";

                delete rs;
                delete ps;
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }

        // ================= ACTIVATE STUDENT =================
        else if (choice == 3) {
            printHeader("ACTIVATE STUDENT");

            string studentId;
            cout << "Enter Student_ID to activate (0 = cancel): ";
            getline(cin, studentId);

            if (studentId == "0") continue;

            try {
                sql::PreparedStatement* ps = con->prepareStatement(
                    "UPDATE user u "
                    "JOIN student s ON u.User_ID = s.User_ID "
                    "SET u.Status = 'Active' "
                    "WHERE s.Student_ID = ?"
                );
                ps->setString(1, studentId);

                int affected = ps->executeUpdate();
                delete ps;

                if (affected > 0)
                    cout << "Student account activated successfully.\n";
                else
                    cout << "Invalid Student_ID.\n";
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }


        // ================= DEACTIVATE STUDENT =================
        else if (choice == 4) {
            printHeader("DEACTIVATE STUDENT");

            string studentId;
            cout << "Enter Student_ID to deactivate (0 = cancel): ";
            getline(cin, studentId);

            if (studentId == "0") continue;

            char confirm;
            cout << "Confirm deactivate this student? (y/n): ";
            cin >> confirm;
            getline(cin, junk);

            if (confirm != 'y' && confirm != 'Y') continue;

            try {
                sql::PreparedStatement* ps = con->prepareStatement(
                    "UPDATE user u "
                    "JOIN student s ON u.User_ID = s.User_ID "
                    "SET u.Status = 'Inactive' "
                    "WHERE s.Student_ID = ?"
                );
                ps->setString(1, studentId);

                int affected = ps->executeUpdate();
                delete ps;

                if (affected > 0)
                    cout << "Student account deactivated successfully.\n";
                else
                    cout << "Invalid Student_ID.\n";
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }

        // ================= EDIT STUDENT =================
        else if (choice == 5) {
            printHeader("EDIT STUDENT INFO");

            string studentId;
            cout << "Enter Student_ID to edit (0 = cancel): ";
            getline(cin, studentId);

            if (studentId == "0") continue;

            try {
                // ===== FETCH CURRENT DATA =====
                sql::PreparedStatement* ps = con->prepareStatement(
                    "SELECT s.Name, s.Matric_No, s.Faculty_ID, f.FacultyCode "
                    "FROM student s "
                    "LEFT JOIN faculty f ON s.Faculty_ID = f.Faculty_ID "
                    "WHERE s.Student_ID = ?"
                );
                ps->setString(1, studentId);
                sql::ResultSet* rs = ps->executeQuery();

                if (!rs->next()) {
                    cout << "Invalid Student_ID.\n";
                    delete rs; delete ps;
                    cout << "Press Enter to continue...";
                    getline(cin, junk);
                    continue;
                }

                string currName = rs->getString("Name");
                string currMatric = rs->getString("Matric_No");
                int currFacultyId = rs->getInt("Faculty_ID");
                string currFacultyCode = rs->isNull("FacultyCode")
                    ? "N/A" : rs->getString("FacultyCode");

                delete rs;
                delete ps;

                // ===== SHOW CURRENT INFO =====
                cout << "Current Name    : " << currName << "\n";
                cout << "Current Matric  : " << currMatric << "\n";
                cout << "Current Faculty : " << currFacultyCode << "\n\n";

                // ===== INPUT NEW VALUES =====
                string newName;
                cout << "Enter new name (empty = keep): ";
                getline(cin, newName);
                if (newName.empty()) newName = currName;

                // ===== SHOW FACULTY LIST =====
                cout << "\nAvailable Faculties:\n";
                sql::Statement* st = con->createStatement();
                rs = st->executeQuery(
                    "SELECT Faculty_ID, FacultyCode FROM faculty ORDER BY Faculty_ID"
                );
                while (rs->next()) {
                    cout << rs->getInt("Faculty_ID")
                        << " - " << rs->getString("FacultyCode") << "\n";
                }
                cout << "0 - Keep current\n";

                delete rs;
                delete st;

                int newFacultyId;
                cout << "Enter new Faculty_ID (0 = keep): ";
                cin >> newFacultyId;
                getline(cin, junk);

                if (newFacultyId == 0)
                    newFacultyId = currFacultyId;

                // ===== UPDATE =====
                ps = con->prepareStatement(
                    "UPDATE student SET Name = ?, Faculty_ID = ? WHERE Student_ID = ?"
                );
                ps->setString(1, newName);
                ps->setInt(2, newFacultyId);
                ps->setString(3, studentId);

                ps->executeUpdate();
                delete ps;

                cout << "Student info updated successfully.\n";
                cout << "Use Search Student to verify the changes.\n";
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }

            cout << "Press Enter to continue...";
            getline(cin, junk);
        }
        else if (choice == 6) {
            string sid;
            cout << "Enter Student ID / Matric (0 = cancel): ";
            getline(cin, sid);
            if (sid == "0") continue;

            // Convert matric to Student_ID if needed
            sql::PreparedStatement* pst = con->prepareStatement(
                "SELECT Student_ID FROM student WHERE Student_ID = ? OR Matric_No = ?"
            );
            pst->setString(1, sid);
            pst->setString(2, sid);

            sql::ResultSet* rr = pst->executeQuery();
            if (!rr->next()) {
                cout << "Student not found.\n";
                cin.get();
            }
            else {
                string realId = rr->getString("Student_ID");
                viewStudentProfileSummary(con, realId);
            }

            delete rr;
            delete pst;
        }
        else if (choice == 7) {
            break;
        }
    }
}

static void updateStudentSummary(sql::Connection* con, const std::string& studentId)
{
    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "UPDATE studentsummary ss "
            "SET "
            "  Total_hours = ( "
            "    SELECT IFNULL(SUM(a.AttendanceHours), 0) "
            "    FROM attendance a "
            "    JOIN participation p ON a.Participation_ID = p.Participation_ID "
            "    WHERE p.Student_ID = ss.Student_ID "
            "      AND a.Check_Out_Time IS NOT NULL "
            "  ), "
            "  Total_points = ( "
            "    SELECT IFNULL(SUM(p.Points_earned), 0) "
            "    FROM participation p "
            "    WHERE p.Student_ID = ss.Student_ID "
            "      AND p.Attendance_marked = 1 "
            "  ) "
            "WHERE ss.Student_ID = ?"
        );

        ps->setString(1, studentId);
        ps->executeUpdate();
        delete ps;
    }
    catch (sql::SQLException& e) {
        cout << "Error updating student summary: " << e.what() << endl;
    }
}

static void viewStudentProfileSummary(sql::Connection* con, const std::string& studentId)
{
    updateStudentSummary(con, studentId);
    printHeader("PROFILE SUMMARY");

    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT s.Student_ID, s.Name, s.Matric_No, s.NRIC, s.Phone, s.Status, "
            "       f.FacultyName, "
            "       ss.Total_hours, ss.Total_points "
            "FROM student s "
            "JOIN studentsummary ss ON s.Student_ID = ss.Student_ID "
            "JOIN faculty f ON s.Faculty_ID = f.Faculty_ID "
            "WHERE s.Student_ID = ? LIMIT 1"
        );
        ps->setString(1, studentId);
        sql::ResultSet* rs = ps->executeQuery();

        if (!rs->next()) {
            cout << "Student profile not found.\n";
            delete rs; delete ps;
            return;
        }

        cout << "Student ID   : " << rs->getString("Student_ID") << "\n";
        cout << "Faculty      : " << rs->getString("FacultyName") << "\n";
        cout << "Name         : " << rs->getString("Name") << "\n";
        cout << "Matric No    : " << rs->getString("Matric_No") << "\n";
        cout << "IC No.       : " << rs->getString("NRIC") << "\n";
        cout << "Phone        : " << rs->getString("Phone") << "\n";
        cout << "Status       : " << rs->getString("Status") << "\n";

        cout << fixed << setprecision(2);
        cout << "Total Hours  : " << rs->getDouble("Total_hours") << "\n";
        cout << "Total Points : " << rs->getDouble("Total_points") << "\n\n";

        delete rs;
        delete ps;

        cout << "Event Breakdown:\n";
        cout << "------------------------------------------------------------\n";

        sql::PreparedStatement* psB = con->prepareStatement(
            "SELECT e.Title, rr.Event_type, "
            "       DATE_FORMAT(a.Check_In_Time, '%d %M %Y %h:%i %p') AS CheckIn, "
            "       DATE_FORMAT(a.Check_Out_Time, '%d %M %Y %h:%i %p') AS CheckOut, "
            "       a.AttendanceHours, p.Points_earned "
            "FROM attendance a "
            "JOIN participation p ON a.Participation_ID = p.Participation_ID "
            "JOIN event e ON p.Event_ID = e.Event_ID "
            "JOIN rewardrule rr ON e.Rule_ID = rr.Rule_ID "
            "WHERE p.Student_ID = ? AND a.Check_Out_Time IS NOT NULL "
            "ORDER BY e.EventDate DESC"
        );
        psB->setString(1, studentId);

        sql::ResultSet* rsB2 = psB->executeQuery();
        bool found = false;

        while (rsB2->next()) {
            found = true;
            cout << rsB2->getString("Title") << "\n";
            cout << "  Event Type: " << rsB2->getString("Event_type") << "\n";
            cout << "  Check-in  : " << rsB2->getString("CheckIn") << "\n";
            cout << "  Check-out : " << rsB2->getString("CheckOut") << "\n";
            cout << "  Hours     : " << rsB2->getDouble("AttendanceHours") << "\n";
            cout << "  Points    : " << rsB2->getDouble("Points_earned") << "\n";
            cout << "------------------------------------------------------------\n";
        }

        if (!found) {
            cout << "No attendance records found.\n";
        }

        delete rsB2;
        delete psB;
    }
    catch (sql::SQLException& e) {
        cout << "Error retrieving summary: " << e.what() << endl;
    }
}



// ================== GENERATE REPORTS (PLACEHOLDER) ==================

static void generateReportsPlaceholder(sql::Connection* con)
{
    while (true) {
        printHeader("GENERATE REPORT");
        cout << "1. Yearly event summary report\n";
        cout << "2. Yearly Student Ranking Report by Faculty\n";
        cout << "3. Back\n";
        cout << "Enter choice: ";

        int choice;
        cin >> choice;

        if (choice == 3) return;

        if (choice == 1) {
            generateYearlyReport(con);
        }
        else if (choice == 2) {
            generateFacultyStudentRanking(con);
        }
    }
}

string fitText(const string& text, size_t width)
{
    if (text.length() <= width)
        return text;
    return text.substr(0, width - 3) + "...";
}

static void generateYearlyReport(sql::Connection* con)
{
    printHeader("YEARLY EVENT SUMMARY REPORT");

    cout << "Enter Year: ";
    int year;
    cin >> year;
    string junk; getline(cin, junk);

    // ===== FACULTY LIST (using code) =====
    vector<string> groups;
    groups.push_back("UNIVERSITY"); // for university-level events

    try {
        sql::PreparedStatement* psF = con->prepareStatement(
            "SELECT FacultyCode FROM faculty ORDER BY FacultyCode"
        );
        sql::ResultSet* rsF = psF->executeQuery();
        while (rsF->next()) {
            groups.push_back(rsF->getString("FacultyCode"));
        }
        delete rsF; delete psF;
    }
    catch (...) {}

    // ===== DATA STRUCT =====
    struct Row {
        int events;
        int attendance;
        double hours;
        Row() : events(0), attendance(0), hours(0.0) {}
    };

    const string monthName[13] = {
        "", "January","February","March","April","May","June",
        "July","August","September","October","November","December"
    };

    int totalYearEvents = 0;
    int totalYearAttendance = 0;
    double totalYearHours = 0.0;

    cout << "\nGenerating report for " << year << "...\n\n";

    for (int m = 1; m <= 12; m++)
    {
        map<string, Row> data;
        for (auto& g : groups) data[g] = Row();

        // ===== EVENTS =====
        try {
            sql::PreparedStatement* ps = con->prepareStatement(
                "SELECT "
                "CASE WHEN e.Faculty_ID IS NULL THEN 'UNIVERSITY' "
                "     ELSE f.FacultyCode END AS GroupCode, "
                "COUNT(*) AS TotalEvents "
                "FROM event e "
                "LEFT JOIN faculty f ON e.Faculty_ID = f.Faculty_ID "
                "WHERE YEAR(e.EventDate)=? AND MONTH(e.EventDate)=? "
                "AND e.Status='ENDED' "
                "GROUP BY GroupCode"
            );

            ps->setInt(1, year);
            ps->setInt(2, m);
            sql::ResultSet* rs = ps->executeQuery();
            while (rs->next()) {
                data[rs->getString("GroupCode")].events =
                    rs->getInt("TotalEvents");
            }
            delete rs; delete ps;
        }
        catch (...) {}

        // ===== ATTENDANCE =====
        try {
            sql::PreparedStatement* ps = con->prepareStatement(
                "SELECT "
                "CASE WHEN e.Faculty_ID IS NULL THEN 'UNIVERSITY' "
                "     ELSE f.FacultyCode END AS GroupCode, "
                "COUNT(a.Attendance_ID) AS TotalAttendance "
                "FROM attendance a "
                "JOIN participation p ON a.Participation_ID = p.Participation_ID "
                "JOIN event e ON p.Event_ID = e.Event_ID "
                "LEFT JOIN faculty f ON e.Faculty_ID = f.Faculty_ID "
                "WHERE YEAR(a.Check_In_Time)=? AND MONTH(a.Check_In_Time)=? "
                "AND a.Check_Out_Time IS NOT NULL "
                "GROUP BY GroupCode"
            );
            ps->setInt(1, year);
            ps->setInt(2, m);
            sql::ResultSet* rs = ps->executeQuery();
            while (rs->next()) {
                data[rs->getString("GroupCode")].attendance =
                    rs->getInt("TotalAttendance");
            }
            delete rs; delete ps;
        }
        catch (...) {}

        // ===== HOURS =====
        try {
            sql::PreparedStatement* ps = con->prepareStatement(
                "SELECT "
                "CASE WHEN e.Faculty_ID IS NULL THEN 'UNIVERSITY' "
                "     ELSE f.FacultyCode END AS GroupCode, "
                "SUM(e.DurationHours) AS TotalHours "
                "FROM event e "
                "LEFT JOIN faculty f ON e.Faculty_ID = f.Faculty_ID "
                "WHERE YEAR(e.EventDate)=? AND MONTH(e.EventDate)=? "
                "AND e.Status='ENDED' "
                "GROUP BY GroupCode"
            );

            ps->setInt(1, year);
            ps->setInt(2, m);
            sql::ResultSet* rs = ps->executeQuery();
            while (rs->next()) {
                data[rs->getString("GroupCode")].hours =
                    rs->getDouble("TotalHours");
            }
            delete rs; delete ps;
        }
        catch (...) {}

        cout << monthName[m] << " " << year << "\n";
        cout << "-------------------------------------------------\n";

        cout << left << setw(12) << "Group"
            << right << setw(10) << "Events"
            << setw(12) << "Hours"
            << setw(15) << "Attendance"
            << "\n";

        cout << "-------------------------------------------------\n";

        int monthlyEvents = 0;
        int monthlyAttendance = 0;
        double monthlyHours = 0.0;

        for (auto& g : groups)
        {
            auto& r = data[g];

            cout << left << setw(12) << g
                << right << setw(10) << r.events
                << setw(12) << fixed << setprecision(2) << r.hours
                << setw(15) << r.attendance
                << "\n";

            // === monthly totals ===
            monthlyEvents += r.events;
            monthlyHours += r.hours;
            monthlyAttendance += r.attendance;

            // === yearly totals ===
            totalYearEvents += r.events;
            totalYearHours += r.hours;
            totalYearAttendance += r.attendance;
        }

        cout << "-------------------------------------------------\n";
        cout << left << setw(12) << "TOTAL"
            << right << setw(10) << monthlyEvents
            << setw(12) << fixed << setprecision(2) << monthlyHours
            << setw(15) << monthlyAttendance
            << "\n";

        cout << "-------------------------------------------------\n\n";
    }

    // ===== YEAR SUMMARY =====
    cout << "\nYEAR " << year << " SUMMARY\n";
    cout << "------------------------------------------------\n";
    cout << "Total Events       : " << totalYearEvents << "\n";
    cout << "Total Attendance   : " << totalYearAttendance << "\n";
    cout << "Total Hours        : " << fixed << setprecision(2) << totalYearHours << "\n";
    cout << "------------------------------------------------\n\n";

    cout << "Press Enter to continue...";
    cin.get();
}


static void generateFacultyStudentRanking(sql::Connection* con)
{
    printHeader("YEARLY STUDENT RANKING REPORT BY FACULTY");

    // ==== Step 1: List Faculties ====
    cout << "Select Faculty:\n";

    vector<pair<int, string>> faculties;

    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT Faculty_ID, FacultyName FROM faculty ORDER BY FacultyName"
        );
        sql::ResultSet* rs = ps->executeQuery();

        int idx = 1;
        while (rs->next()) {
            faculties.push_back({ rs->getInt("Faculty_ID"), rs->getString("FacultyName") });
            cout << idx++ << ". " << rs->getString("FacultyName") << "\n";
        }
        delete rs; delete ps;
    }
    catch (...) {
        cout << "Error loading faculty list.\n";
        return;
    }

    if (faculties.empty()) {
        cout << "No faculty found.\n";
        return;
    }

    cout << "Enter choice (0 = cancel): ";
    int choice;
    cin >> choice;
    string junk; getline(cin, junk);

    if (choice <= 0 || choice > (int)faculties.size()) return;

    int facId = faculties[choice - 1].first;
    string facName = faculties[choice - 1].second;

    // ==== Step 1.5: Ask Year ====
    cout << "Enter Year (e.g., 2026): ";
    int year;
    cin >> year;
    getline(cin, junk); // flush

    cout << "\nGenerating ranking for faculty: " << facName
        << " (Year " << year << ")\n\n";

    // ==== Step 2: Load Students in faculty ====
    struct Row {
        string matric;
        string name;
        string studentId;
        int events = 0;
        double hours = 0;
        double points = 0;
    };

    vector<Row> rows;

    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT Student_ID, Name, Matric_No "
            "FROM student WHERE Faculty_ID = ? ORDER BY Name"
        );
        ps->setInt(1, facId);

        sql::ResultSet* rs = ps->executeQuery();
        while (rs->next()) {
            Row r;
            r.studentId = rs->getString("Student_ID");
            r.name = rs->getString("Name");
            r.matric = rs->getString("Matric_No");
            rows.push_back(r);
        }
        delete rs; delete ps;
    }
    catch (...) {
        cout << "Error loading student list.\n";
        return;
    }

    if (rows.empty()) {
        cout << "No students found under this faculty.\n";
        return;
    }

    // ==== Step 3: Load attendance stats for selected year ====
    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT p.Student_ID, "
            "COUNT(a.Attendance_ID) AS TotalEvents, "
            "SUM(a.AttendanceHours) AS TotalHours, "
            "SUM(p.Points_earned) AS TotalPoints "
            "FROM attendance a "
            "JOIN participation p ON a.Participation_ID = p.Participation_ID "
            "WHERE a.Check_Out_Time IS NOT NULL "
            "AND YEAR(a.Check_Out_Time) = ? "
            "GROUP BY p.Student_ID"
        );

        ps->setInt(1, year);

        sql::ResultSet* rs = ps->executeQuery();
        map<string, Row> stats;

        while (rs->next()) {
            Row r;
            r.studentId = rs->getString("Student_ID");
            r.events = rs->getInt("TotalEvents");
            r.hours = rs->getDouble("TotalHours");
            r.points = rs->getDouble("TotalPoints");
            stats[r.studentId] = r;
        }

        delete rs; delete ps;

        // attach stats
        for (auto& r : rows) {
            if (stats.count(r.studentId)) {
                r.events = stats[r.studentId].events;
                r.hours = stats[r.studentId].hours;
                r.points = stats[r.studentId].points;
            }
        }
    }
    catch (...) {
        cout << "Error computing stats.\n";
    }

    // ==== Step 4: Sorting ====
    sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        if (a.points != b.points) return a.points > b.points;
        if (a.hours != b.hours) return a.hours > b.hours;
        return a.events > b.events;
        });

    // ==== Step 5: Output ====
    cout << left
        << setw(6) << "Rank"
        << setw(15) << "Matric"
        << setw(35) << "Name"
        << setw(12) << "Events"
        << setw(12) << "Hours"
        << setw(12) << "Points" << "\n";

    cout << string(92, '-') << "\n";

    int rank = 1;
    for (auto& r : rows) {
        cout << left
            << setw(6) << rank++
            << setw(15) << r.matric
            << setw(35) << r.name
            << setw(12) << r.events
            << setw(12) << fixed << setprecision(2) << r.hours
            << setw(12) << fixed << setprecision(2) << r.points
            << "\n";
    }

    cout << string(92, '-') << "\n";
    cout << "Press Enter to continue...";
    cin.get();
}








