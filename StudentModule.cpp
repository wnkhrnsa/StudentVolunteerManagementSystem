#include <iostream>
#include <string>
#include <iomanip>
#include <vector>

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include "StudentModule.h"
#include "LogRegModule.h"
#include "StaffModule.h"

using namespace std;

// printHeader ada dalam main.cpp
extern void printHeader(const std::string& title);

// ---------- Helper declarations (untuk file ni sahaja) ----------
static std::string getStudentIdByUserId(sql::Connection* con, int userId);
static void studentRegisterEvent(sql::Connection* con, const std::string& studentId);
static void viewStudentProfileSummary(sql::Connection* con, const std::string& studentId);
static void viewParticipationHistory(sql::Connection* con, const std::string& studentId);
static void updateStudentProfile(sql::Connection* con, const std::string& studentId);
static bool deleteStudentProfile(sql::Connection* con, const std::string& studentId, int userId);
static void studentAttendanceMenu(sql::Connection* con, const std::string& studentId);
static void studentWithdrawEvent(sql::Connection* con, const std::string& studentId);
bool canStudentJoinEvent(sql::Connection* con,
    const string& studentId,
    int eventId,
    string& errorMsg);

// ================== IMPLEMENTATION ==================

void studentMenu(sql::Connection* con, int userId) {
    std::string studentId = getStudentIdByUserId(con, userId);
    if (studentId.empty()) {
        cout << "\n[ERROR] Student profile not found for this account.\n";
        return;
    }

    while (true) {
        printHeader("STUDENT MENU");
        cout << "1. Register for event\n";
        cout << "2. Withdraw from event\n";
        cout << "3. Attendance (Check-in / Check-out)\n";
        cout << "4. View profile summary\n";
        cout << "5. View participation history\n";
        cout << "6. Update Profile\n";
        cout << "7. Deactivate Profile\n\n";
        cout << "8. Back to main menu\n\n";
        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            string junk; getline(cin, junk);
            continue;
        }
        string junk; getline(cin, junk); // clear newline

        if (choice == 1) {
            studentRegisterEvent(con, studentId);
        }
        else if (choice == 2) {
            studentWithdrawEvent(con, studentId);
        }
        else if (choice == 3) {
            studentAttendanceMenu(con, studentId);
        }
        else if (choice == 4) {
            viewStudentProfileSummary(con, studentId); //TODO
        }
        else if (choice == 5) {
            viewParticipationHistory(con, studentId);
        }
        else if (choice == 6) {
            updateStudentProfile(con, studentId);
        }
        else if (choice == 7) {
            bool deleted = deleteStudentProfile(con, studentId, userId);
            if (deleted) {
                break;
            }
        }
        else if (choice == 8) {
            break;
        }
        else {
            cout << "\nInvalid choice.\n";
        }

        cout << "\nPress Enter to return to Student Menu...";
        getline(cin, junk);
    }
}

static std::string getStudentIdByUserId(sql::Connection* con, int userId) {
    try {
        sql::PreparedStatement* pstmt = con->prepareStatement(
            "SELECT Student_ID FROM student WHERE User_ID = ? LIMIT 1"
        );
        pstmt->setInt(1, userId);

        sql::ResultSet* res = pstmt->executeQuery();

        std::string studentId;
        if (res->next()) {
            studentId = res->getString("Student_ID");   // S001 / S002 / S003
        }

        delete res;
        delete pstmt;
        return studentId;
    }
    catch (sql::SQLException& e) {
        cout << "Error getting Student_ID: " << e.what() << endl;
        return "";
    }
}

static bool canStudentJoinEvent(sql::Connection* con,
    const string& studentId,
    int eventId,
    string& errorMsg)
{
    sql::PreparedStatement* ps = con->prepareStatement(
        "SELECT s.Faculty_ID AS StudentFaculty, e.Faculty_ID AS EventFaculty "
        "FROM student s, event e "
        "WHERE s.Student_ID = ? AND e.Event_ID = ?"
    );
    ps->setString(1, studentId);
    ps->setInt(2, eventId);

    sql::ResultSet* rs = ps->executeQuery();

    if (!rs->next()) {
        errorMsg = "Invalid student or event.";
        delete rs; delete ps;
        return false;
    }

    int studentFaculty = rs->getInt("StudentFaculty");
    bool eventFacultyIsNull = rs->isNull("EventFaculty");
    int eventFaculty = eventFacultyIsNull ? 0 : rs->getInt("EventFaculty");

    delete rs;
    delete ps;

    // University event → boleh
    if (eventFacultyIsNull)
        return true;

    // Faculty event → mesti sama
    if (studentFaculty == eventFaculty)
        return true;

    errorMsg = "You can only register for events from your own faculty.";
    return false;
}

void studentRegisterEvent(sql::Connection* con, const std::string& studentId)
{
    printHeader("UPCOMING EVENTS");
    updateEventStatus(con);
    int studentFacultyId = 0;
    try {
        sql::PreparedStatement* psF = con->prepareStatement(
            "SELECT Faculty_ID FROM student WHERE Student_ID = ?"
        );
        psF->setString(1, studentId);
        sql::ResultSet* rsF = psF->executeQuery();

        if (rsF->next() && !rsF->isNull("Faculty_ID"))
            studentFacultyId = rsF->getInt("Faculty_ID");

        delete rsF;
        delete psF;
    }
    catch (sql::SQLException& e) {
        cout << "Error fetching student faculty: " << e.what() << endl;
    }
    while (true) {
        printHeader("REGISTER FOR EVENT");

        cout << "1. View upcoming events\n";
        cout << "2. Filter by category\n";
        cout << "3. Search by title\n";
        cout << "4. Back\n\n";
        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            string junk; getline(cin, junk);
            continue;
        }
        string junk; getline(cin, junk);

        if (choice == 1) {
            // VIEW UPCOMING
            try {
                sql::PreparedStatement* ps = con->prepareStatement(
                    "SELECT "
                    "e.Event_ID, "
                    "e.Title, "
                    "e.EventDate, "
                    "e.Location, "
                    "e.Capacity, "
                    "c.Name AS CategoryName, "
                    "f.FacultyCode, "
                    "COUNT(p.Participation_ID) AS Registered "
                    "FROM event e "
                    "LEFT JOIN participation p ON e.Event_ID = p.Event_ID "
                    "LEFT JOIN category c ON e.Category_ID = c.Category_ID "
                    "LEFT JOIN faculty f ON e.Faculty_ID = f.Faculty_ID "
                    "WHERE e.Status = 'Upcoming' "
                    "AND (e.Faculty_ID IS NULL OR e.Faculty_ID = ?) "
                    "GROUP BY e.Event_ID "
                    "ORDER BY e.EventDate;"
                );
                ps->setInt(1, studentFacultyId);
                sql::ResultSet* rs = ps->executeQuery();
                
                bool any = false;
                while (rs->next()) {
                    any = true;
                    cout << "Event ID : " << rs->getInt("Event_ID") << "\n";
                    cout << "Title    : " << rs->getString("Title") << "\n";
                    cout << "Date     : " << rs->getString("EventDate") << "\n";
                    cout << "Location : " << rs->getString("Location") << "\n";
                    int reg = rs->getInt("Registered");
                    int cap = rs->getInt("Capacity");
                    cout << "Category : " << rs->getString("CategoryName") << "\n";

                    if (rs->isNull("FacultyCode"))
                        cout << "Faculty  : University\n";
                    else
                        cout << "Faculty  : " << rs->getString("FacultyCode") << "\n";

                    cout << "Slot     : " << reg << " / " << cap;
                    if (cap > 0 && reg >= cap) cout << " (FULL)";
                    cout << "\n";

                    cout << "--------------------------------------------\n";
                }

                delete rs;
                delete ps;

                if (!any) {
                    cout << "No upcoming events.\n";
                    continue;
                }

                cout << "Enter Event_ID to register (0 = cancel): ";
                int eid;
                if (!(cin >> eid)) {
                    cin.clear();
                    getline(cin, junk);
                    continue;
                }
                getline(cin, junk);

                if (eid == 0) continue;
                sql::PreparedStatement* vchk = con->prepareStatement(
                    "SELECT 1 "
                    "FROM event "
                    "WHERE Event_ID = ? "
                    "AND Status = 'Upcoming' "
                    "AND (Faculty_ID IS NULL OR Faculty_ID = ?)"
                );
                vchk->setInt(1, eid);
                vchk->setInt(2, studentFacultyId);

                sql::ResultSet* vr = vchk->executeQuery();
                if (!vr->next()) {
                    cout << "Invalid Event ID. Please select from the listed events.\n";
                    delete vr;
                    delete vchk;
                    continue;
                }
                delete vr;
                delete vchk;

                // ===== CHECK DUPLICATE =====
                sql::PreparedStatement* chk = con->prepareStatement(
                    "SELECT 1 FROM participation WHERE Student_ID = ? AND Event_ID = ? LIMIT 1"
                );
                chk->setString(1, studentId);
                chk->setInt(2, eid);
                sql::ResultSet* r1 = chk->executeQuery();

                if (r1->next()) {
                    cout << "You have already registered for this event.\n";
                    delete r1; delete chk;
                    continue;
                }
                delete r1; delete chk;

                // ===== CHECK FULL =====
                sql::PreparedStatement* capchk = con->prepareStatement(
                    "SELECT e.Capacity, COUNT(p.Participation_ID) AS Registered "
                    "FROM event e LEFT JOIN participation p ON e.Event_ID = p.Event_ID "
                    "WHERE e.Event_ID = ? GROUP BY e.Event_ID"
                );
                capchk->setInt(1, eid);
                sql::ResultSet* r2 = capchk->executeQuery();

                if (r2->next()) {
                    int cap = r2->getInt("Capacity");
                    int reg = r2->getInt("Registered");

                    if (cap > 0 && reg >= cap) {
                        cout << "Event is FULL. Cannot register.\n";
                        delete r2; delete capchk;
                        continue;
                    }
                }
                delete r2; delete capchk;

                // ===== CHECK FACULTY RESTRICTION =====
                string err;
                if (!canStudentJoinEvent(con, studentId, eid, err)) {
                    cout << "[ERROR] " << err << "\n";
                    continue;
                }

                sql::PreparedStatement* ins = con->prepareStatement(
                    "INSERT INTO participation "
                    "(Student_ID, Event_ID, RegisteredDate, Attendance_marked, Points_earned) "
                    "VALUES (?, ?, NOW(), 0, 0)"
                );
                ins->setString(1, studentId);
                ins->setInt(2, eid);
                ins->executeUpdate();
                delete ins;

                cout << "\nSuccessfully registered.\n";
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }
        }

        else if (choice == 2) {
            // FILTER
            int categoryId = 0;
            int studentFacultyId = 0;
            {
                sql::PreparedStatement* ps = con->prepareStatement(
                    "SELECT Faculty_ID FROM student WHERE Student_ID = ?"
                );
                ps->setString(1, studentId);
                sql::ResultSet* rs = ps->executeQuery();
                if (rs->next() && !rs->isNull("Faculty_ID"))
                    studentFacultyId = rs->getInt("Faculty_ID");
                delete rs; delete ps;
            }
            if (studentFacultyId == 0) {
                cout << "[ERROR] Student faculty not found. Please contact admin.\n";
                continue;
            }
            try {
                cout << "\nSelect Category (0 = ignore):\n";

                // Ambik kategori & sort by name
                sql::Statement* st = con->createStatement();
                sql::ResultSet* rs = st->executeQuery(
                    "SELECT Category_ID, Name FROM category ORDER BY Name"
                );

                vector<int> categoryMap;
                int catIdx = 1;

                while (rs->next()) {
                    categoryMap.push_back(rs->getInt("Category_ID"));
                    cout << catIdx++ << " - " << rs->getString("Name") << "\n";
                }

                delete rs;
                delete st;

                cout << "Category: ";
                cin >> categoryId;
                getline(cin, junk);

                // Map user input back to real Category_ID
                if (categoryId > 0 && categoryId <= (int)categoryMap.size()) {
                    categoryId = categoryMap[categoryId - 1];
                }
                else {
                    categoryId = 0; // ignore
                }

                string sql =
                    "SELECT e.Event_ID, e.Title, e.EventDate, e.Location, "
                    "e.Capacity, "
                    "c.Name AS CategoryName, "
                    "f.FacultyCode, "
                    "COUNT(p.Participation_ID) AS Registered "
                    "FROM event e "
                    "LEFT JOIN participation p ON e.Event_ID = p.Event_ID "
                    "LEFT JOIN category c ON e.Category_ID = c.Category_ID "
                    "LEFT JOIN faculty f ON e.Faculty_ID = f.Faculty_ID "
                    "WHERE e.Status='Upcoming' "
                    "AND (e.Faculty_ID IS NULL OR e.Faculty_ID = ?)";

                if (categoryId != 0)
                    sql += " AND e.Category_ID = ?";

                sql += " GROUP BY e.Event_ID ORDER BY e.EventDate";

                sql::PreparedStatement* ps = con->prepareStatement(sql);
                int idx = 1;

                ps->setInt(idx++, studentFacultyId); // <<< WAJIB

                if (categoryId != 0)
                    ps->setInt(idx++, categoryId);

                rs = ps->executeQuery();
                bool any = false;
                while (rs->next()) {
                    any = true;
                    cout << "Event ID : " << rs->getInt("Event_ID") << "\n";
                    cout << "Title    : " << rs->getString("Title") << "\n";
                    cout << "Date     : " << rs->getString("EventDate") << "\n";
                    cout << "Location : " << rs->getString("Location") << "\n";
                    int reg = rs->getInt("Registered");
                    int cap = rs->getInt("Capacity");
                    cout << "Category : " << rs->getString("CategoryName") << "\n";

                    if (rs->isNull("FacultyCode"))
                        cout << "Faculty  : University\n";
                    else
                        cout << "Faculty  : " << rs->getString("FacultyCode") << "\n";


                    cout << "Slot     : " << reg << " / " << cap;
                    if (cap > 0 && reg >= cap) cout << " (FULL)";
                    cout << "\n";
                    cout << "--------------------------------------------\n";
                }

                delete rs;
                delete ps;

                if (!any) {
                    cout << "No matching events.\n";
                    continue;
                }

                cout << "Enter Event_ID to register (0 = cancel): ";
                int eid;
                cin >> eid;
                getline(cin, junk);
                if (eid == 0) continue;
                string vsql =
                    "SELECT 1 FROM event "
                    "WHERE Event_ID = ? "
                    "AND Status = 'Upcoming' "
                    "AND (Faculty_ID IS NULL OR Faculty_ID = ?)";

                if (categoryId != 0)
                    vsql += " AND Category_ID = ?";

                sql::PreparedStatement* vchk = con->prepareStatement(vsql);

                int vidx = 1;
                vchk->setInt(vidx++, eid);
                vchk->setInt(vidx++, studentFacultyId);

                if (categoryId != 0)
                    vchk->setInt(vidx++, categoryId);

                sql::ResultSet* vr = vchk->executeQuery();
                if (!vr->next()) {
                    cout << "Invalid Event ID. Please select from the listed events.\n";
                    delete vr;
                    delete vchk;
                    continue;
                }

                delete vr;
                delete vchk;

                // ===== CHECK DUPLICATE =====
                sql::PreparedStatement* chk = con->prepareStatement(
                    "SELECT 1 FROM participation WHERE Student_ID = ? AND Event_ID = ? LIMIT 1"
                );
                chk->setString(1, studentId);
                chk->setInt(2, eid);
                sql::ResultSet* r1 = chk->executeQuery();

                if (r1->next()) {
                    cout << "You have already registered for this event.\n";
                    delete r1; delete chk;
                    continue;
                }
                delete r1; delete chk;

                // ===== CHECK FULL =====
                sql::PreparedStatement* capchk = con->prepareStatement(
                    "SELECT e.Capacity, COUNT(p.Participation_ID) AS Registered "
                    "FROM event e LEFT JOIN participation p ON e.Event_ID = p.Event_ID "
                    "WHERE e.Event_ID = ? GROUP BY e.Event_ID"
                );
                capchk->setInt(1, eid);
                sql::ResultSet* r2 = capchk->executeQuery();

                if (r2->next()) {
                    int cap = r2->getInt("Capacity");
                    int reg = r2->getInt("Registered");

                    if (cap > 0 && reg >= cap) {
                        cout << "Event is FULL. Cannot register.\n";
                        delete r2; delete capchk;
                        continue;
                    }
                }
                delete r2; delete capchk;

                sql::PreparedStatement* ins = con->prepareStatement(
                    "INSERT INTO participation "
                    "(Student_ID, Event_ID, RegisteredDate, Attendance_marked, Points_earned) "
                    "VALUES (?, ?, NOW(), 0, 0)"
                );
                ins->setString(1, studentId);
                ins->setInt(2, eid);
                ins->executeUpdate();
                delete ins;

                cout << "\nSuccessfully registered.\n";
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }
        }

        else if (choice == 3) {
            // SEARCH
            int studentFacultyId = 0;
            {
                sql::PreparedStatement* ps = con->prepareStatement(
                    "SELECT Faculty_ID FROM student WHERE Student_ID = ?"
                );
                ps->setString(1, studentId);
                sql::ResultSet* rs = ps->executeQuery();
                if (rs->next() && !rs->isNull("Faculty_ID"))
                    studentFacultyId = rs->getInt("Faculty_ID");
                delete rs; delete ps;
            }
            cout << "Enter title keyword: ";
            string key;
            getline(cin, key);

            try {
                sql::PreparedStatement* ps = con->prepareStatement(
                    "SELECT e.Event_ID, e.Title, e.EventDate, e.Location, "
                    "e.Capacity, "
                    "c.Name AS CategoryName, "
                    "f.FacultyCode, "
                    "COUNT(p.Participation_ID) AS Registered "
                    "FROM event e "
                    "LEFT JOIN participation p ON e.Event_ID = p.Event_ID "
                    "LEFT JOIN category c ON e.Category_ID = c.Category_ID "
                    "LEFT JOIN faculty f ON e.Faculty_ID = f.Faculty_ID "
                    "WHERE e.Status = 'Upcoming' "
                    "AND e.Title LIKE ? "
                    "AND (e.Faculty_ID IS NULL OR e.Faculty_ID = ?) "
                    "GROUP BY e.Event_ID"
                );
                ps->setString(1, "%" + key + "%");
                ps->setInt(2, studentFacultyId);

                sql::ResultSet* rs = ps->executeQuery();
                bool any = false;

                while (rs->next()) {
                    any = true;
                    cout << "Event ID : " << rs->getInt("Event_ID") << "\n";
                    cout << "Title    : " << rs->getString("Title") << "\n";
                    cout << "Date     : " << rs->getString("EventDate") << "\n";
                    cout << "Location : " << rs->getString("Location") << "\n";
                    int reg = rs->getInt("Registered");
                    int cap = rs->getInt("Capacity");
                    cout << "Category : " << rs->getString("CategoryName") << "\n";

                    if (rs->isNull("FacultyCode"))
                        cout << "Faculty  : University\n";
                    else
                        cout << "Faculty  : " << rs->getString("FacultyCode") << "\n";


                    cout << "Slot     : " << reg << " / " << cap;
                    if (cap > 0 && reg >= cap) cout << " (FULL)";
                    cout << "\n";

                    cout << "--------------------------------------------\n";
                }

                delete rs;
                delete ps;

                if (!any) {
                    cout << "No matching events.\n";
                    continue;
                }

                cout << "Enter Event_ID to register (0 = cancel): ";
                int eid;
                cin >> eid;
                getline(cin, junk);
                if (eid == 0) continue;
                sql::PreparedStatement* vchk = con->prepareStatement(
                    "SELECT 1 FROM event "
                    "WHERE Event_ID = ? "
                    "AND Status = 'Upcoming' "
                    "AND Title LIKE ? "
                    "AND (Faculty_ID IS NULL OR Faculty_ID = ?)"
                );
                vchk->setInt(1, eid);
                vchk->setString(2, "%" + key + "%");
                vchk->setInt(3, studentFacultyId);

                sql::ResultSet* vr = vchk->executeQuery();
                if (!vr->next()) {
                    cout << "Invalid Event ID. Please select from the listed events.\n";
                    delete vr;
                    delete vchk;
                    continue;
                }
                delete vr;
                delete vchk;

                // ===== CHECK DUPLICATE =====
                sql::PreparedStatement* chk = con->prepareStatement(
                    "SELECT 1 FROM participation WHERE Student_ID = ? AND Event_ID = ? LIMIT 1"
                );
                chk->setString(1, studentId);
                chk->setInt(2, eid);
                sql::ResultSet* r1 = chk->executeQuery();

                if (r1->next()) {
                    cout << "You have already registered for this event.\n";
                    delete r1; delete chk;
                    continue;
                }
                delete r1; delete chk;

                // ===== CHECK FULL =====
                sql::PreparedStatement* capchk = con->prepareStatement(
                    "SELECT e.Capacity, COUNT(p.Participation_ID) AS Registered "
                    "FROM event e LEFT JOIN participation p ON e.Event_ID = p.Event_ID "
                    "WHERE e.Event_ID = ? GROUP BY e.Event_ID"
                );
                capchk->setInt(1, eid);
                sql::ResultSet* r2 = capchk->executeQuery();

                if (r2->next()) {
                    int cap = r2->getInt("Capacity");
                    int reg = r2->getInt("Registered");

                    if (cap > 0 && reg >= cap) {
                        cout << "Event is FULL. Cannot register.\n";
                        delete r2; delete capchk;
                        continue;
                    }
                }
                delete r2; delete capchk;

                sql::PreparedStatement* ins = con->prepareStatement(
                    "INSERT INTO participation "
                    "(Student_ID, Event_ID, RegisteredDate, Attendance_marked, Points_earned) "
                    "VALUES (?, ?, NOW(), 0, 0)"
                );
                ins->setString(1, studentId);
                ins->setInt(2, eid);
                ins->executeUpdate();
                delete ins;

                cout << "\nSuccessfully registered.\n";
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }
        }
        else if (choice == 4) {
            return;
        }
    }
}

static void studentWithdrawEvent(sql::Connection* con, const std::string& studentId)
{
    printHeader("WITHDRAW FROM EVENT");
    updateEventStatus(con);

    try {
        // 1️⃣ Papar HANYA upcoming events yang student register
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT e.Event_ID, e.Title, e.EventDate, e.Location "
            "FROM event e "
            "JOIN participation p ON e.Event_ID = p.Event_ID "
            "WHERE p.Student_ID = ? AND e.Status = 'Upcoming'"
        );
        ps->setString(1, studentId);
        sql::ResultSet* rs = ps->executeQuery();

        bool any = false;
        while (rs->next()) {
            any = true;
            cout << "Event ID : " << rs->getInt("Event_ID") << "\n";
            cout << "Title    : " << rs->getString("Title") << "\n";
            cout << "Date     : " << rs->getString("EventDate") << "\n";
            cout << "Location : " << rs->getString("Location") << "\n";
            cout << "--------------------------------------------\n";
        }

        delete rs;
        delete ps;

        if (!any) {
            cout << "You have no upcoming events to withdraw from.\n";
            return;
        }

        cout << "Enter Event_ID to withdraw (0 = cancel): ";
        int eid;
        cin >> eid;
        string junk; getline(cin, junk);
        if (eid == 0) return;

        // 2️⃣ Confirm
        cout << "Confirm withdraw? (Y/N): ";
        char c;
        cin >> c;
        getline(cin, junk);
        if (c != 'Y' && c != 'y') return;

        // 3️⃣ Delete participation
        ps = con->prepareStatement(
            "DELETE FROM participation WHERE Student_ID = ? AND Event_ID = ?"
        );
        ps->setString(1, studentId);
        ps->setInt(2, eid);
        int rows = ps->executeUpdate();
        delete ps;

        if (rows > 0)
            cout << "Successfully withdrawn from the event.\n";
        else
            cout << "Withdrawal failed.\n";
    }
    catch (sql::SQLException& e) {
        cout << "Error: " << e.what() << endl;
    }
}

static void studentAttendanceMenu(sql::Connection* con, const std::string& studentId)
{
    updateEventStatus(con);
    while (true) {
        printHeader("ATTENDANCE");

        cout << "1. Check-in\n";
        cout << "2. Check-out\n";
        cout << "3. Back\n\n";
        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            string junk; getline(cin, junk);
            continue;
        }
        string junk; getline(cin, junk);

        //CHECK IN
        if (choice == 1) {
            try {
                printHeader("CHECK-IN");

                sql::PreparedStatement* ps = con->prepareStatement(
                    "SELECT p.Participation_ID, e.Title, "
                    "DATE_FORMAT(e.EventDate,'%d %M %Y, %h:%i %p') AS EventDate, "
                    "e.DurationHours, e.Location, e.Attendance_Passcode "
                    "FROM participation p "
                    "JOIN event e ON p.Event_ID = e.Event_ID "
                    "WHERE p.Student_ID = ? AND e.Status = 'Active'"
                );
                ps->setString(1, studentId);

                sql::ResultSet* rs = ps->executeQuery();

                int pidList[50];
                string passList[50];
                int count = 0;

                cout << "Available Events:\n";
                cout << "------------------------------------------------------\n";

                while (rs->next()) {
                    count++;
                    pidList[count] = rs->getInt("Participation_ID");
                    passList[count] = rs->getString("Attendance_Passcode");

                    cout << count << ") " << rs->getString("Title") << "\n";
                    cout << "   Date     : " << rs->getString("EventDate") << "\n";
                    cout << "   Duration : " << rs->getDouble("DurationHours") << " Hours\n";
                    cout << "   Venue    : " << rs->getString("Location") << "\n";
                    cout << "------------------------------------------------------\n";
                }

                delete rs;
                delete ps;

                if (count == 0) {
                    cout << "No active events available for check-in.\n";
                    cout << "Press Enter to return...";
                    cin.get();
                    return;
                }

                cout << "Select event (1-" << count << ", 0 = Back): ";
                int opt;
                cin >> opt;
                string junk; getline(cin, junk);

                if (opt == 0) return;
                if (opt < 1 || opt > count) {
                    cout << "Invalid selection.\n";
                    cout << "Press Enter to return...";
                    cin.get();
                    return;
                }

                int pid = pidList[opt];
                string eventPass = passList[opt];

                // Prevent double check-in
                ps = con->prepareStatement(
                    "SELECT 1 FROM attendance "
                    "WHERE Participation_ID = ? AND Check_Out_Time IS NULL"
                );
                ps->setInt(1, pid);
                rs = ps->executeQuery();

                if (rs->next()) {
                    cout << "Already checked-in. Please check-out first.\n";
                    delete rs; delete ps;
                    cout << "Press Enter to return...";
                    cin.get();
                    return;
                }

                delete rs;
                delete ps;

                // Attendance passcode (optional)
                if (!eventPass.empty()) {
                    cout << "Enter attendance passcode: ";
                    string inputPass;
                    getline(cin, inputPass);

                    if (inputPass != eventPass) {
                        cout << "Invalid attendance passcode.\n";
                        cout << "Press Enter to return...";
                        cin.get();
                        return;
                    }
                }

                // Insert attendance
                ps = con->prepareStatement(
                    "INSERT INTO attendance (Participation_ID, Check_In_Time, Status) "
                    "VALUES (?, NOW(), 'IN')"
                );
                ps->setInt(1, pid);
                ps->executeUpdate();
                delete ps;

                cout << "\nCheck-in successful!\n";
                cout << "Press Enter to return to Student Menu...";
                cin.get();
                return;
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
                cout << "Press Enter to continue...";
                cin.get();
            }
        }


        else if (choice == 2) {
            try {
                printHeader("CHECK-OUT");

                // === STEP 1: List attendance still IN ===
                sql::PreparedStatement* ps = con->prepareStatement(
                    "SELECT a.Attendance_ID, a.Participation_ID, "
                    "DATE_FORMAT(a.Check_In_Time, '%d %M %Y %h:%i %p') AS CheckInTime, "
                    "e.Title, "
                    "f.FacultyCode, "
                    "rr.Event_type "
                    "FROM attendance a "
                    "JOIN participation p ON a.Participation_ID = p.Participation_ID "
                    "JOIN event e ON p.Event_ID = e.Event_ID "
                    "JOIN rewardrule rr ON e.Rule_ID = rr.Rule_ID "
                    "LEFT JOIN faculty f ON e.Faculty_ID = f.Faculty_ID "
                    "WHERE p.Student_ID = ? "
                    "AND a.Check_Out_Time IS NULL "
                    "AND e.Status != 'Ended'"
                );


                ps->setString(1, studentId);

                sql::ResultSet* rs = ps->executeQuery();

                int aidList[50];
                int count = 0;

                cout << "Checked-In Events:\n";
                cout << "-----------------------------------------------------\n";
                while (rs->next()) {
                    count++;
                    aidList[count] = rs->getInt("Attendance_ID");

                    cout << count << ") " << rs->getString("Title") << "\n";

                    string fac = rs->isNull("FacultyCode") ? "UNIVERSITY" : rs->getString("FacultyCode");
                    cout << "   Type       : " << fac << "\n";
                    cout << "   Checked-in : " << rs->getString("CheckInTime") << "\n";

                    cout << "-----------------------------------------------------\n";
                }

                delete rs;
                delete ps;

                if (count == 0) {
                    cout << "No active check-in found.\n";
                    return;
                }

                cout << "Select (1-" << count << ", 0=Back): ";
                int opt;
                cin >> opt;
                getline(cin, junk);
                if (opt == 0) return;
                if (opt < 1 || opt > count) {
                    cout << "Invalid selection.\n";
                    return;
                }

                int aid = aidList[opt];

                // === STEP 2: calc hours & multiplier ===
                double hours, multiplier;
                int pid;

                sql::PreparedStatement* ps2 = con->prepareStatement(
                    "SELECT a.Participation_ID, "
                    "TIMESTAMPDIFF(MINUTE, a.Check_In_Time, NOW()) / 60.0 AS Hours, "
                    "rr.Multiplier, "
                    "e.Status AS EventStatus "
                    "FROM attendance a "
                    "JOIN participation p ON a.Participation_ID = p.Participation_ID "
                    "JOIN event e ON p.Event_ID = e.Event_ID "
                    "JOIN rewardrule rr ON e.Rule_ID = rr.Rule_ID "
                    "WHERE a.Attendance_ID = ?"
                );

                ps2->setInt(1, aid);

                sql::ResultSet* rs2 = ps2->executeQuery();
                if (!rs2->next()) {
                    cout << "Invalid selection.\n";
                    delete rs2;
                    delete ps2;
                    return;
                }

                pid = rs2->getInt("Participation_ID");
                hours = rs2->getDouble("Hours");
                multiplier = rs2->getDouble("Multiplier");
                string evStatus = rs2->getString("EventStatus");
                if (evStatus == "Ended") {
                    cout << "This event has already ended. Check-out is not allowed.\n";
                    delete rs2;
                    delete ps2;
                    return;
                }

                delete rs2;
                delete ps2;

                if (hours < 0) hours = 0;  // just safety

                // === STEP 3: calculate points ===
                double points = hours * multiplier;

                // === STEP 4: ROUND hours + points (2 decimals) ===
                hours = round(hours * 100.0) / 100.0;
                points = round(points * 100.0) / 100.0;

                // === STEP 5: Update attendance ===
                ps2 = con->prepareStatement(
                    "UPDATE attendance "
                    "SET Check_Out_Time = NOW(), Status='COMPLETED', AttendanceHours=? "
                    "WHERE Attendance_ID = ?"
                );
                ps2->setDouble(1, hours);
                ps2->setInt(2, aid);
                ps2->executeUpdate();
                delete ps2;

                // === STEP 6: Update participation ===
                ps2 = con->prepareStatement(
                    "UPDATE participation SET Points_earned=?, Attendance_marked=1 "
                    "WHERE Participation_ID=?"
                );
                ps2->setDouble(1, points);
                ps2->setInt(2, pid);
                ps2->executeUpdate();
                delete ps2;

                // === STEP 7: Display nicely ===
                cout << fixed << setprecision(2);
                cout << "Check-out successful!\n";
                cout << "Hours   : " << hours << "\n";
                cout << "Points  : " << points << "\n";

                updateStudentSummary(con, studentId);
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }
        }

        else if (choice == 3) {
            return;
        }
    }
}

static void viewStudentProfileSummary(sql::Connection* con, const std::string& studentId)
{
    updateStudentSummary(con, studentId);
    printHeader("PROFILE SUMMARY");

    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT s.Name, s.Matric_No, s.NRIC, s.Phone, s.Status, "
            "       ss.Total_hours, ss.Total_points "
            "FROM student s "
            "JOIN studentsummary ss ON s.Student_ID = ss.Student_ID "
            "WHERE s.Student_ID = ? LIMIT 1"
        );
        ps->setString(1, studentId);
        sql::ResultSet* rs = ps->executeQuery();

        if (!rs->next()) {
            cout << "Student profile not found.\n";
            delete rs; delete ps;
            return;
        }

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

        // BREAKDOWN
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


static void viewParticipationHistory(sql::Connection* con, const std::string& studentId)
{
    printHeader("PARTICIPATION HISTORY");

    cout << "Filter:\n";
    cout << "1. All\n";
    cout << "2. Registered (Upcoming)\n";
    cout << "3. Ended\n";
    cout << "Choose: ";

    int filter;
    cin >> filter;
    string junk; getline(cin, junk);

    try {
        // ================= BUILD QUERY =================
        string query =
            "SELECT "
            " e.Title, "
            " DATE_FORMAT(e.EventDate, '%d-%m-%Y %H:%i') AS EventDateFormatted, "
            " e.Status, "
            " c.name AS CategoryName, "
            " rr.Event_type AS EventType, "
            " e.DurationHours, "
            " p.RegisteredDate, "
            " IFNULL(p.Points_earned, 0) AS PointsEarned, "
            " a.Check_In_Time, "
            " a.Check_Out_Time, "
            " a.Status AS AttendanceStatus, "
            " IFNULL(a.AttendanceHours, 0) AS AttendanceHours "
            "FROM participation p "
            "JOIN event e ON p.Event_ID = e.Event_ID "
            "JOIN category c ON e.Category_ID = c.Category_ID "
            "JOIN rewardrule rr ON e.Rule_ID = rr.Rule_ID "
            "LEFT JOIN attendance a ON p.Participation_ID = a.Participation_ID "
            "WHERE p.Student_ID = ? ";

        if (filter == 2) {
            query += "AND e.Status = 'Upcoming' ";
        }
        else if (filter == 3) {
            query += "AND e.Status = 'Ended' ";
        }

        query += "ORDER BY p.RegisteredDate DESC";

        // ================= EXECUTE =================
        printHeader("PARTICIPATION EVENTS HISTORY");
        sql::PreparedStatement* pstmt = con->prepareStatement(query);
        pstmt->setString(1, studentId);
        sql::ResultSet* res = pstmt->executeQuery();

        bool any = false;

        while (res->next()) {
            any = true;

            cout << "Event      : " << res->getString("Title") << "\n";
            cout << "Date       : " << res->getString("EventDateFormatted") << "\n";
            cout << "Status     : " << res->getString("Status") << "\n";
            cout << "Category   : " << res->getString("CategoryName") << "\n";
            cout << "Duration   : " << fixed << setprecision(1)
                << res->getDouble("DurationHours") << " hours\n";

            // ================= ATTENDANCE =================
            bool hasCheckIn = !res->isNull("Check_In_Time");
            bool hasCheckOut = !res->isNull("Check_Out_Time");
            string attStatus = res->isNull("AttendanceStatus") ? "" : res->getString("AttendanceStatus");

            if (!hasCheckIn) {
                cout << "Attendance : Not Attended\n";
            }
            else if (hasCheckIn && !hasCheckOut) {
                cout << "Attendance : Incomplete (No Checkout)\n";
                cout << "Check-in   : " << res->getString("Check_In_Time") << "\n";
            }
            else {
                cout << "Attendance : Completed";
                if (attStatus == "LATE") cout << " (Late)";
                cout << "\n";

                cout << "Check-in   : " << res->getString("Check_In_Time") << "\n";
                cout << "Check-out  : " << res->getString("Check_Out_Time") << "\n";
                cout << "Hours      : " << fixed << setprecision(2)
                    << res->getDouble("AttendanceHours") << "\n";
            }

            cout << "-----------------------------------------\n";
        }

        if (!any)
            cout << "No participation records found.\n";

        delete res;
        delete pstmt;
    }
    catch (sql::SQLException& e) {
        cout << "Error retrieving participation history: " << e.what() << endl;
    }
}


void updateStudentProfile(sql::Connection* con, const std::string& studentId) {
    // 1) Fetch current data
    string name, ic, matric, phone, status, facultyCode, email;
    int facultyId = -1;

    try {
        sql::PreparedStatement* stmt = con->prepareStatement(
            "SELECT s.Name, s.Matric_No, s.NRIC, s.Phone, s.Status, "
            "       s.Faculty_ID, f.FacultyCode, u.Email "
            "FROM student s "
            "JOIN user u ON s.User_ID = u.User_ID "
            "LEFT JOIN faculty f ON s.Faculty_ID = f.Faculty_ID "
            "WHERE s.Student_ID = ? LIMIT 1"
        );
        stmt->setString(1, studentId);
        sql::ResultSet* r = stmt->executeQuery();

        if (!r->next()) {
            cout << "\n[ERROR] Student record not found.\n";
            delete r;
            delete stmt;
            cout << "Press Enter to continue...";
            string tmp; getline(cin, tmp);
            return;
        }

        name = r->getString("Name");
        matric = r->getString("Matric_No");
        ic = r->getString("NRIC");
        phone = r->getString("Phone");
        email = r->getString("Email");
        status = r->getString("Status");
        if (!r->isNull("Faculty_ID")) {
            facultyId = r->getInt("Faculty_ID");
        }
        if (!r->isNull("FacultyCode")) {
            facultyCode = r->getString("FacultyCode");
        }

        delete r;
        delete stmt;
    }
    catch (sql::SQLException& e) {
        cout << "Error fetching current profile: " << e.what() << endl;
        cout << "Press Enter to continue...";
        string tmp; getline(cin, tmp);
        return;
    }

    // 2) Edit-loop – pilih apa nak tukar
    while (true) {
        printHeader("UPDATE STUDENT PROFILE");

        cout << "Current profile:\n";
        cout << "  Name        : " << name << "\n";
        cout << "  Matric No   : " << matric << "\n";
        cout << "  IC          : " << ic << "\n";
        cout << "  Phone       : " << phone << "\n";
        cout << "  Email       : " << email << " \n";
        cout << "  Status      : " << status << "\n";
        cout << "  FacultyCode : " << (facultyCode.empty() ? "(none)" : facultyCode) << "\n\n";

        cout << "What do you want to update?\n";
        cout << "1. Name\n";
        cout << "2. IC\n";
        cout << "3. Matric No\n";
        cout << "4. Phone\n";
        cout << "5. Faculty\n";
        cout << "6. Save & return\n";
        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            string junk; getline(cin, junk);
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }
        string junk; getline(cin, junk); // clear newline

        if (choice == 1) {
            cout << "New Name: ";
            getline(cin, name);
        }
        else if (choice == 2) {
            cout << "New IC: ";
            getline(cin, ic);
        }
        else if (choice == 3) {
            cout << "New Matric No: ";
            getline(cin, matric);
        }
        else if (choice == 4) {
            cout << "New Phone No: ";
            getline(cin, phone);
        }
        else if (choice == 5) {
            cout << "New Faculty Code (e.g. FTMK): ";
            string newCode;
            getline(cin, newCode);

            if (newCode.empty()) {
                cout << "Faculty code empty. Faculty not changed.\n";
            }
            else {
                try {
                    sql::PreparedStatement* stmt = con->prepareStatement(
                        "SELECT Faculty_ID FROM faculty WHERE FacultyCode = ? LIMIT 1"
                    );
                    stmt->setString(1, newCode);
                    sql::ResultSet* r = stmt->executeQuery();

                    if (r->next()) {
                        facultyId = r->getInt("Faculty_ID");
                        facultyCode = newCode;
                        cout << "Faculty updated in buffer.\n";
                    }
                    else {
                        cout << "Invalid Faculty Code. Faculty not changed.\n";
                    }

                    delete r;
                    delete stmt;
                }
                catch (sql::SQLException& e) {
                    cout << "Faculty lookup error: " << e.what() << endl;
                }
            }
        }
        else if (choice == 6) {
            // SAVE TO DB
            try {
                sql::PreparedStatement* stmt = con->prepareStatement(
                    "UPDATE student "
                    "SET Name = ?, NRIC = ?, Matric_No = ?, Phone = ?, Faculty_ID = ? "
                    "WHERE Student_ID = ?"
                );
                stmt->setString(1, name);
                stmt->setString(2, ic);
                stmt->setString(3, matric);
                stmt->setString(4, phone);

                if (facultyId > 0) stmt->setInt(5, facultyId);
                else stmt->setNull(5, sql::DataType::INTEGER);

                stmt->setString(6, studentId);

                int rows = stmt->executeUpdate();
                delete stmt;

                if (rows > 0)
                    cout << "\nProfile updated successfully!\n";
                else
                    cout << "\nNo changes were made.\n";
            }
            catch (sql::SQLException& e) {
                cout << "Error updating student profile: " << e.what() << endl;
            }
            break;
        }
        else {
            cout << "Please choose between 1-6.\n";
        }
    }
}

static bool deleteStudentProfile(sql::Connection* con, const std::string& studentId, int userId) {
    printHeader("DELETE STUDENT PROFILE");

    cout << "Are you sure you want to DEACTIVATE this student profile and account?\n";
    cout << "This action CANNOT be undone.\n";
    cout << "Type Y to confirm, anything else to cancel: ";

    string ans;
    getline(cin, ans);
    if (ans != "Y" && ans != "y") {
        cout << "Deletion cancelled.\n";
        return false;
    }

    try {
        // 1) set student status to Inactive (kalau column Status ada)
        sql::PreparedStatement* pstmt = con->prepareStatement(
            "UPDATE student SET Status = 'Inactive' WHERE Student_ID = ?"
        );
        pstmt->setString(1, studentId);
        pstmt->executeUpdate();
        delete pstmt;

        // 2) set user status to Inactive (block login)
        pstmt = con->prepareStatement(
            "UPDATE `user` SET Status = 'Inactive' WHERE User_ID = ?"
        );
        pstmt->setInt(1, userId);
        pstmt->executeUpdate();
        delete pstmt;

        cout << "\nStudent profile deactivated successfully.\n";
    }
    catch (sql::SQLException& e) {
        cout << "Error deactivating student profile: " << e.what() << endl;
    }

    cout << "Press Enter to continue...";
    string tmp; getline(cin, tmp);

    return true;
}

