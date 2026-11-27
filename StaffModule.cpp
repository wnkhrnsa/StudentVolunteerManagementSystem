#include <iostream>
#include <string>
#include <regex>
#include <iomanip>

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include "StaffModule.h"
#include "LogRegModule.h"
#include "StudentModule.h"

using namespace std;

// printHeader ada dalam main.cpp
extern void printHeader(const std::string& title);

// ---------- Forward declarations (helper dalam file ni je) ----------
static void manageEventsMenu(sql::Connection* con, int userId);
static void viewRegisteredStudents(sql::Connection* con, int userId);
static void updateStaffProfile(sql::Connection* con, int userId);
static void manageAttendanceMenu(sql::Connection* con, int userId);
static void viewStaffProfile(sql::Connection* con, int userId); 

static void viewMyEvents(sql::Connection* con, int userId);
static void viewEventDetails(sql::Connection* con, int eventId);

static void createEvent(sql::Connection* con, int userId); //create
static void editEvent(sql::Connection* con, int userId); //update
static void cancelUpcomingEvent(sql::Connection* con, int userId); //delete
static void searchEvent(sql::Connection* con); //read 
static void viewFilteredEvents(sql::Connection* con);
static void viewUpcomingEvents(sql::Connection* con);
int listAndPickByIndex(
    sql::Connection* con,
    const std::string& table,
    const std::string& idCol,
    const std::string& nameCol,
    const std::string& prompt
);
void updateStudentSummary(sql::Connection* con, const std::string& studentId);
static void generateSingleEventReport(sql::Connection* con, int userId, int eventId);
static void staffEventReportMenu(sql::Connection* con, int userId);
// ================== STAFF MENU UTAMA ==================

void staffMenu(sql::Connection* con, int userId)
{
    updateEventStatus(con);
    while (true) {
        printHeader("STAFF MENU");

        cout << "1. Manage events\n";
        cout << "2. Manage attendance\n";
        cout << "3. View registered students\n";
        cout << "4. View Profile\n";
        cout << "5. Update Profile\n";
        cout << "6. Event reporting\n";
        cout << "7. Back to main menu\n";
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
            manageEventsMenu(con, userId);
        }
        else if (choice == 2) {
            manageAttendanceMenu(con, userId);
        }
        else if (choice == 3) {
            viewRegisteredStudents(con, userId);
        }
        else if (choice == 4) {
            viewStaffProfile(con, userId);
        }
        else if (choice == 5) {
            updateStaffProfile(con, userId);
        }
        else if (choice == 6) {
            staffEventReportMenu(con, userId);
        }
        else if (choice == 7) {
            break;
        }
        else {
            cout << "\nInvalid choice.\n";
            cout << "Press Enter to continue..."; getline(cin, junk);
        }
    }
}

//check leapyear
//Check format guna regex
//check Extract nombor
static bool isLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}
static bool validateDateTime(const std::string& dt) {
    std::regex re(R"(^(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})$)");
    std::smatch m;
    if (!std::regex_match(dt, m, re)) return false;
    int year = std::stoi(m[1].str());
    int month = std::stoi(m[2].str());
    int day = std::stoi(m[3].str());
    int hour = std::stoi(m[4].str());
    int minute = std::stoi(m[5].str());
    int second = std::stoi(m[6].str());
    if (month < 1 || month > 12) return false;
    if (hour < 0 || hour > 23) return false;
    if (minute < 0 || minute > 59) return false;
    if (second < 0 || second > 59) return false;
    int mdays[] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 }; //Check hari ikut bulan
    if (month == 2) mdays[2] = isLeapYear(year) ? 29 : 28;
    if (day < 1 || day > mdays[month]) return false;
    return true;
}

static void updateStudentSummary(sql::Connection* con, const std::string& studentId)
{
    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "UPDATE studentsummary ss "
            "SET "
            "  Total_hours = ( "
            "    SELECT IFNULL(SUM(TIMESTAMPDIFF(MINUTE, a.Check_In_Time, a.Check_Out_Time)/60.0), 0) "
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
// ================== MANAGE EVENTS ==================

static void manageEventsMenu(sql::Connection* con, int userId)
{
    updateEventStatus(con);

    while (true) {
        printHeader("MANAGE EVENTS (STAFF)");
        cout << "1. View my events\n";
        cout << "2. Create event\n";
        cout << "3. Edit event\n";
        cout << "4. Cancel upcoming event\n";
        cout << "5. Search event by title\n";
        cout << "6. Filter events (Category / Faculty / Status)\n";
        cout << "7. Back to Staff Menu\n\n";
        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            string junk; getline(cin, junk);
            continue;
        }
        string junk; getline(cin, junk);
        if (choice == 1) {
            viewMyEvents(con, userId);
        }
        else if (choice == 2) {
            createEvent(con, userId);
        }
        else if (choice == 3) {
            editEvent(con, userId);
        }
        else if (choice == 4) {
            cancelUpcomingEvent(con, userId);
        }
        else if (choice == 5) {
            searchEvent(con);
        }
        else if (choice == 6) {
            viewFilteredEvents(con);
        }
        else if (choice == 7) {
            break;
        }
        else {
            cout << "\nInvalid choice.\n";
        }

        cout << "\nPress Enter to return to Manage Events menu...";
        getline(cin, junk);
    }
}
// Helper: fetch table (id_col, name_col) and display numbered list; return real DB ID mapped from chosen index (1..N) or 0 for ignore.
// table: table name (e.g. "category"), idCol: "Category_ID", nameCol: "Name", prompt: displayed prompt.
//before use id, now by no bil
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

// ================= VIEW MY EVENTS =================
static void viewMyEvents(sql::Connection* con, int userId)
{
    printHeader("MY EVENTS");

    cout << "Filter by status:\n";
    cout << "1. All\n";
    cout << "2. Upcoming\n";
    cout << "3. Ended\n";
    cout << "4. Cancelled\n";
    cout << "Enter choice: ";

    int choice;
    if (!(cin >> choice)) {
        cin.clear();
        string junk; getline(cin, junk);
        cout << "Invalid input.\n";
        return;
    }
    string junk; getline(cin, junk);

    string statusFilter = "";

    if (choice == 2) statusFilter = "Upcoming";
    else if (choice == 3) statusFilter = "Ended";
    else if (choice == 4) statusFilter = "Cancelled";
    else if (choice != 1) {
        cout << "Invalid choice.\n";
        return;
    }

    try {
        string query =
            "SELECT e.Event_ID, e.Title, e.EventDate, e.Status, "
            "e.Capacity, COUNT(p.Participation_ID) AS Registered "
            "FROM event e "
            "LEFT JOIN participation p ON e.Event_ID = p.Event_ID "
            "WHERE e.Organizer_User_ID = ? ";

        if (!statusFilter.empty())
            query += "AND e.Status = ? ";

        query +=
            "GROUP BY e.Event_ID "
            "ORDER BY e.EventDate DESC";

        sql::PreparedStatement* ps = con->prepareStatement(query);
        ps->setInt(1, userId);
        if (!statusFilter.empty())
            ps->setString(2, statusFilter);

        sql::ResultSet* rs = ps->executeQuery();

        cout << "\nID | Title | Date | Status | Slots\n";
        cout << "---------------------------------------------\n";

        bool any = false;
        while (rs->next()) {
            any = true;
            cout << rs->getInt("Event_ID") << " | "
                << rs->getString("Title") << " | "
                << rs->getString("EventDate") << " | "
                << rs->getString("Status") << " | "
                << rs->getInt("Registered") << "/"
                << rs->getInt("Capacity") << "\n";
        }

        if (!any)
            cout << "No events found.\n";

        delete rs;
        delete ps;
        cout << "\nEnter Event_ID to view details (blank = back): ";
        string eventInput;
        getline(cin, eventInput);

        if (eventInput.empty())
            return;

        int eventId = stoi(eventInput);
        viewEventDetails(con, eventId);

    }
    catch (sql::SQLException& e) {
        cout << "Error: " << e.what() << endl;
    }
}

static void viewEventDetails(sql::Connection* con, int eventId)
{
    printHeader("EVENT DETAILS");

    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT e.Title, e.EventDate, e.DurationHours, e.Location, "
            "e.Capacity, e.Description, e.Status, "
            "COUNT(p.Participation_ID) AS Registered "
            "FROM event e "
            "LEFT JOIN participation p ON e.Event_ID = p.Event_ID "
            "WHERE e.Event_ID = ? "
            "GROUP BY e.Event_ID"
        );

        ps->setInt(1, eventId);
        sql::ResultSet* rs = ps->executeQuery();

        if (!rs->next()) {
            cout << "Event not found.\n";
        }
        else {
            cout << "Title      : " << rs->getString("Title") << "\n";
            cout << "Date       : " << rs->getString("EventDate") << "\n";
            cout << "Duration   : " << rs->getInt("DurationHours") << " hours\n";
            cout << "Location   : " << rs->getString("Location") << "\n";
            cout << "Status     : " << rs->getString("Status") << "\n";
            cout << "Slots      : "
                << rs->getInt("Registered") << "/"
                << rs->getInt("Capacity") << "\n";
            cout << "Description:\n"
                << rs->getString("Description") << "\n";
        }

        delete rs;
        delete ps;
    }
    catch (sql::SQLException& e) {
        cout << "Error: " << e.what() << endl;
    }

    cout << "\nPress Enter to return...";
    string junk;
    getline(cin, junk);
}

static void createEvent(sql::Connection* con, int userId)
{
    printHeader("CREATE EVENT");

    string title, dateTime, location, description, status, attendancePasscode;
    double durationHours = 0.0;
    int capacity = 0;
    int categoryId = 0;
    int facultyId = 0;
    int chosenRuleId = 0;
    int staffFacultyId = 0;

    // ================= GET STAFF FACULTY =================
    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT Faculty_ID FROM staff WHERE User_ID = ?"
        );
        ps->setInt(1, userId);
        sql::ResultSet* rs = ps->executeQuery();
        if (rs->next() && !rs->isNull("Faculty_ID")) {
            staffFacultyId = rs->getInt("Faculty_ID");
        }
        delete rs;
        delete ps;
    }
    catch (sql::SQLException& e) {
        cout << "Error retrieving staff faculty: " << e.what() << endl;
        return;
    }

    // ================= BASIC INPUT =================
    while (true) {
        cout << "Title : ";
        getline(cin, title);
        if (title == "cancel") return;
        if (title.empty()) { cout << "Title cannot be empty.\n"; continue; }
        break;
    }

    bool validDate = false;
    while (!validDate) {
        cout << "Event DateTime (YYYY-MM-DD HH:MM:SS): ";
        getline(cin, dateTime);
        if (dateTime == "cancel") return;

        try {
            sql::PreparedStatement* ps = con->prepareStatement(
                "SELECT (NOW() < ?) AS ok"
            );
            ps->setString(1, dateTime);
            sql::ResultSet* rs = ps->executeQuery();
            rs->next();
            validDate = rs->getInt("ok") == 1;
            delete rs;
            delete ps;

            if (!validDate)
                cout << "Date must be in the future.\n";
        }
        catch (...) {
            cout << "Invalid date format.\n";
        }
    }

    while (true) {
        cout << "Duration (hours): ";
        string tmp; getline(cin, tmp);
        if (tmp == "cancel") return;
        try {
            durationHours = stod(tmp);
            if (durationHours <= 0) throw 1;
            break;
        }
        catch (...) {
            cout << "Invalid duration.\n";
        }
    }

    cout << "Location : ";
    getline(cin, location);
    if (location == "cancel") return;

    cout << "Capacity : ";
    string cap; getline(cin, cap);
    capacity = cap.empty() ? 0 : stoi(cap);

    cout << "Description : ";
    getline(cin, description);

    cout << "Attendance Passcode (optional): ";
    getline(cin, attendancePasscode);

    // ================= EVENT TYPE =================
    int etChoice = 0;
    string eventTypeStr;

    // ================= EVENT TYPE (UX FIXED) =================
    if (staffFacultyId == 0) {
        // Staff tanpa fakulti → University sahaja
        cout << "\nEvent Type:\n";
        cout << "1. University\n";
        cout << "Select (1): ";

        string tmp;
        getline(cin, tmp);
        if (tmp == "cancel") return;

        etChoice = 2;                 // force University
        eventTypeStr = "University";
        facultyId = 0;                // NULL
    }
    else {
        // Staff ada fakulti → boleh pilih
        while (true) {
            cout << "\nEvent Type:\n";
            cout << "1. Faculty\n";
            cout << "2. University\n";
            cout << "Select (1/2): ";

            string tmp;
            getline(cin, tmp);
            if (tmp == "cancel") return;

            try {
                etChoice = stoi(tmp);
                if (etChoice == 1) {
                    eventTypeStr = "Faculty";
                    facultyId = staffFacultyId;   // AUTO ikut staff
                    break;
                }
                else if (etChoice == 2) {
                    eventTypeStr = "University";
                    facultyId = 0;                // NULL
                    break;
                }
            }
            catch (...) {}

            cout << "Invalid choice. Please select 1 or 2.\n";
        }
    }

    // ================= FACULTY LOGIC =================

    if (etChoice == 1) {
        if (staffFacultyId == 0) {
            cout << "You are not assigned to any faculty. Cannot create faculty event.\n";
            return;
        }
        eventTypeStr = "Faculty";
        facultyId = staffFacultyId;   // AUTO
    }
    else {
        eventTypeStr = "University";
        facultyId = 0;                // NULL
    }

    // ================= REWARD RULE =================
    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT Rule_ID FROM rewardrule WHERE Event_type = ? LIMIT 1"
        );
        ps->setString(1, eventTypeStr);
        sql::ResultSet* rs = ps->executeQuery();
        if (rs->next())
            chosenRuleId = rs->getInt("Rule_ID");
        delete rs;
        delete ps;
    }
    catch (...) {
        chosenRuleId = 0;
    }

    // ================= CATEGORY =================
    categoryId = listAndPickByIndex(con, "category", "Category_ID", "Name", "Category");

    // ================= INSERT =================
    status = "Upcoming";

    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "INSERT INTO event "
            "(Title, EventDate, DurationHours, Location, Capacity, Status, Description, "
            "Rule_ID, Category_ID, Organizer_User_ID, Faculty_ID, Attendance_Passcode) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        );

        ps->setString(1, title);
        ps->setString(2, dateTime);
        ps->setDouble(3, durationHours);
        ps->setString(4, location);
        ps->setInt(5, capacity);
        ps->setString(6, status);
        ps->setString(7, description);

        if (chosenRuleId > 0) ps->setInt(8, chosenRuleId);
        else ps->setNull(8, sql::DataType::INTEGER);

        if (categoryId > 0) ps->setInt(9, categoryId);
        else ps->setNull(9, sql::DataType::INTEGER);

        ps->setInt(10, userId);

        if (facultyId > 0) ps->setInt(11, facultyId);
        else ps->setNull(11, sql::DataType::INTEGER);

        if (attendancePasscode.empty())
            ps->setNull(12, sql::DataType::VARCHAR);
        else
            ps->setString(12, attendancePasscode);

        ps->executeUpdate();
        delete ps;

        cout << "\nEvent created successfully.\n";
    }
    catch (sql::SQLException& e) {
        cout << "Error creating event: " << e.what() << endl;
    }
}

// ------------- EDIT EVENT ------------------
static void editEvent(sql::Connection* con, int userId)
{
    updateEventStatus(con);
    printHeader("EDIT EVENT");

    // ================= LIST UPCOMING EVENTS (OWNED) =================
    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT e.Event_ID, e.Title, e.EventDate, e.Capacity, "
            "COUNT(p.Participation_ID) AS Registered "
            "FROM event e "
            "LEFT JOIN participation p ON e.Event_ID = p.Event_ID "
            "WHERE e.Status = 'Upcoming' AND e.Organizer_User_ID = ? "
            "GROUP BY e.Event_ID "
            "ORDER BY e.EventDate"
        );
        ps->setInt(1, userId);
        sql::ResultSet* rs = ps->executeQuery();

        cout << "\nID | Title | Date | Slots\n";
        bool any = false;
        while (rs->next()) {
            any = true;
            cout << rs->getInt("Event_ID") << " | "
                << rs->getString("Title") << " | "
                << rs->getString("EventDate") << " | "
                << rs->getInt("Registered") << "/"
                << rs->getInt("Capacity") << "\n";
        }

        delete rs; delete ps;
        if (!any) {
            cout << "No upcoming events.\n";
            return;
        }
    }
    catch (...) {
        cout << "Error loading events.\n";
        return;
    }

    // ================= PICK EVENT =================
    cout << "\nEnter Event_ID (blank = cancel): ";
    string input;
    getline(cin, input);
    if (input.empty()) return;

    int eventId = stoi(input);

    // ================= FETCH EVENT DETAIL =================
    string title, dateTime, location, description, categoryName, eventType, facultyCode;
    double duration;
    int capacity;

    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT e.Title, e.EventDate, e.DurationHours, e.Location, e.Capacity, "
            "e.Description, c.Name AS CategoryName, r.Event_type, f.FacultyCode "
            "FROM event e "
            "LEFT JOIN category c ON e.Category_ID = c.Category_ID "
            "LEFT JOIN rewardrule r ON e.Rule_ID = r.Rule_ID "
            "LEFT JOIN faculty f ON e.Faculty_ID = f.Faculty_ID "
            "WHERE e.Event_ID = ? AND e.Organizer_User_ID = ?"
        );
        ps->setInt(1, eventId);
        ps->setInt(2, userId);

        sql::ResultSet* rs = ps->executeQuery();
        if (!rs->next()) {
            cout << "Event not found or not yours.\n";
            delete rs; delete ps;
            return;
        }

        title = rs->getString("Title");
        dateTime = rs->getString("EventDate");
        duration = rs->getDouble("DurationHours");
        location = rs->getString("Location");
        capacity = rs->getInt("Capacity");
        description = rs->getString("Description");
        categoryName = rs->isNull("CategoryName") ? "N/A" : rs->getString("CategoryName");
        eventType = rs->getString("Event_type");
        facultyCode = rs->isNull("FacultyCode") ? "" : rs->getString("FacultyCode");

        delete rs; delete ps;
    }
    catch (...) {
        cout << "Error reading event.\n";
        return;
    }

    // ================= DISPLAY CURRENT =================
    cout << "\nCurrent Event Info\n";
    cout << "Title      : " << title << "\n";
    cout << "Date       : " << dateTime << "\n";
    cout << "Duration   : " << duration << " hours\n";
    cout << "Location   : " << location << "\n";
    cout << "Capacity   : " << capacity << "\n";
    cout << "Category   : " << categoryName << "\n";
    cout << "Event Type : " << eventType << "\n";
    if (eventType == "Faculty")
        cout << "Faculty    : " << facultyCode << "\n";
    cout << "Description: " << description << "\n\n";

    // ================= EDIT =================
    string newTitle, newDate, newLocation, newDesc, tmp;

    cout << "New title (blank = keep): ";
    getline(cin, newTitle);
    if (newTitle.empty()) newTitle = title;

    cout << "New datetime (YYYY-MM-DD HH:MM:SS, blank = keep): ";
    getline(cin, newDate);
    if (newDate.empty()) newDate = dateTime;

    cout << "New duration (blank = keep): ";
    getline(cin, tmp);
    if (!tmp.empty()) duration = stod(tmp);

    cout << "New location (blank = keep): ";
    getline(cin, newLocation);
    if (newLocation.empty()) newLocation = location;

    cout << "New capacity (blank = keep): ";
    getline(cin, tmp);
    if (!tmp.empty()) capacity = stoi(tmp);

    cout << "New description (blank = keep): ";
    getline(cin, newDesc);
    if (newDesc.empty()) newDesc = description;

    // ================= UPDATE =================
    try {
        sql::PreparedStatement* upd = con->prepareStatement(
            "UPDATE event SET Title=?, EventDate=?, DurationHours=?, "
            "Location=?, Capacity=?, Description=? "
            "WHERE Event_ID=? AND Organizer_User_ID=?"
        );
        upd->setString(1, newTitle);
        upd->setString(2, newDate);
        upd->setDouble(3, duration);
        upd->setString(4, newLocation);
        upd->setInt(5, capacity);
        upd->setString(6, newDesc);
        upd->setInt(7, eventId);
        upd->setInt(8, userId);

        upd->executeUpdate();
        delete upd;
        cout << "\nEvent updated successfully.\n";
    }
    catch (...) {
        cout << "Update failed.\n";
    }
}

static void viewUpcomingEvents(sql::Connection* con)
{
    updateEventStatus(con);
    try {
        sql::PreparedStatement* pstmt = con->prepareStatement(
            "SELECT e.Event_ID, e.Title, e.EventDate, e.Status, "
            "e.Capacity, COUNT(p.Participation_ID) AS Registered "
            "FROM event e "
            "LEFT JOIN participation p ON e.Event_ID = p.Event_ID "
            "WHERE LOWER(e.Status) = 'upcoming' "
            "GROUP BY e.Event_ID "
            "ORDER BY e.EventDate"
        );
        sql::ResultSet* res = pstmt->executeQuery();

        cout << "\n=== Upcoming Events ===\n";
        bool any = false;
        while (res->next()) {
            any = true;
            int id = res->getInt("Event_ID");
            string name = res->getString("Title");
            string dateStr = res->getString("EventDate");
            string status = res->getString("Status");
            int reg = res->getInt("Registered");
            int cap = res->getInt("Capacity");
            cout << "ID: " << id
                << " | " << name
                << " | " << dateStr
                << " | Status: " << status
                << " | Slot: " << reg << "/" << cap
                << "\n";

        }
        if (!any) cout << "No upcoming event.\n";

        delete res;
        delete pstmt;
    }
    catch (sql::SQLException& e) {
        cout << "Error reading events: " << e.what() << endl;
    }
}

// deleteEvent yang hanya membenarkan delete untuk upcoming events milik staff sendiri
static void cancelUpcomingEvent(sql::Connection* con, int userId)
{
    updateEventStatus(con);
    printHeader("CANCEL UPCOMING EVENT");

    // Papar upcoming events MILIK STAFF
    try {
        sql::PreparedStatement* psList = con->prepareStatement(
            "SELECT Event_ID, Title, EventDate "
            "FROM event "
            "WHERE Status = 'Upcoming' AND Organizer_User_ID = ? "
            "ORDER BY EventDate"
        );
        psList->setInt(1, userId);
        sql::ResultSet* rsList = psList->executeQuery();

        bool any = false;
        printHeader("MY UPCOMING EVENT");
        while (rsList->next()) {
            any = true;
            cout << "Event ID : " << rsList->getInt("Event_ID") << "\n";
            cout << "Title    : " << rsList->getString("Title") << "\n";
            cout << "Date     : " << rsList->getString("EventDate") << "\n";
            cout << "----------------------------------\n";
        }

        delete rsList;
        delete psList;

        if (!any) {
            cout << "You have no upcoming events.\n";
            cout << "Press Enter to continue...";
            string junk; getline(cin, junk);
            return;
        }
    }
    catch (sql::SQLException& e) {
        cout << "Error listing events: " << e.what() << endl;
        return;
    }

    cout << "\nEnter Event_ID to cancel (0 = back): ";
    int eventId;
    if (!(cin >> eventId)) {
        cin.clear();
        string junk; getline(cin, junk);
        cout << "Invalid input.\n";
        return;
    }
    string junk; getline(cin, junk);

    if (eventId == 0) return;

    try {
        // Check event ownership + status
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT Title, Status FROM event "
            "WHERE Event_ID = ? AND Organizer_User_ID = ?"
        );
        ps->setInt(1, eventId);
        ps->setInt(2, userId);

        sql::ResultSet* rs = ps->executeQuery();

        if (!rs->next()) {
            cout << "Event not found or you are not authorized.\n";
            delete rs; delete ps;
            return;
        }

        string title = rs->getString("Title");
        string status = rs->getString("Status");

        delete rs;
        delete ps;

        if (status != "Upcoming") {
            cout << "Only UPCOMING events can be cancelled.\n";
            return;
        }

        // Confirm
        char confirm;
        cout << "Cancel event \"" << title << "\" ? (y/n): ";
        cin >> confirm;
        getline(cin, junk);

        if (confirm != 'y' && confirm != 'Y') {
            cout << "Cancelled by user.\n";
            return;
        }

        // Cancel event
        ps = con->prepareStatement(
            "UPDATE event SET Status = 'Cancelled' WHERE Event_ID = ?"
        );
        ps->setInt(1, eventId);
        ps->executeUpdate();
        delete ps;

        cout << "Event cancelled successfully.\n";
    }
    catch (sql::SQLException& e) {
        cout << "Database error: " << e.what() << endl;
    }
}


// --- Search event by title ---
static void searchEvent(sql::Connection* con)
{
    updateEventStatus(con);
    printHeader("SEARCH EVENT");

    cout << "Enter keyword (title): ";
    string keyword;
    getline(cin, keyword);
    if (keyword.empty()) {
        cout << "Search cancelled.\n";
        return;
    }

    try {
        sql::PreparedStatement* pstmt = con->prepareStatement(
            "SELECT Event_ID, Title, EventDate, Location, Status "
            "FROM event WHERE Title LIKE ? ORDER BY EventDate"
        );
        string pattern = "%" + keyword + "%";
        pstmt->setString(1, pattern);

        sql::ResultSet* res = pstmt->executeQuery();
        bool any = false;

        while (res->next()) {
            any = true;
            int eventId = res->getInt("Event_ID");
            string title = res->getString("Title");
            string dateTime = res->getString("EventDate");
            string location = res->getString("Location");
            string status = res->getString("Status");

            cout << "Event ID : " << eventId << "\n";
            cout << "Title    : " << title << "\n";
            cout << "DateTime : " << dateTime << "\n";
            cout << "Location : " << location << "\n";
            cout << "Status   : " << status << "\n";
            cout << "------------------------------------------------------------\n";
        }

        if (!any) cout << "No events match your search.\n";

        delete res;
        delete pstmt;
    }
    catch (sql::SQLException& e) {
        cout << "Error searching events: " << e.what() << endl;
    }
}

// --- Filter by Category / Faculty (simple version) ---
// Gabungan filter: Category_ID, Faculty_ID, Status (optional kombinasi)
// Gabungan filter: gunakan listAndPickByIndex untuk category & faculty
static void viewFilteredEvents(sql::Connection* con)
{
    updateEventStatus(con);
    printHeader("FILTER EVENTS");

    // ===== CATEGORY (OPTIONAL) =====
    int categoryId = listAndPickByIndex(
        con, "category", "Category_ID", "Name", "Category"
    );

    // ===== STATUS (OPTIONAL) =====
    string status;
    cout << "Enter Status (Upcoming / Active / Ended / Cancelled, blank = skip): ";
    getline(cin, status);

    bool hasStatus = false;
    if (!status.empty()) {
        transform(status.begin(), status.end(), status.begin(), ::tolower);
        hasStatus = true;
    }

    // ===== EVENT TYPE (OPTIONAL) =====
    string eventType;
    cout << "Enter Event Type (University / Faculty, blank = skip): ";
    getline(cin, eventType);

    bool hasType = false;
    if (!eventType.empty()) {
        transform(eventType.begin(), eventType.end(), eventType.begin(), ::tolower);
        if (eventType == "university" || eventType == "faculty") {
            eventType[0] = toupper(eventType[0]);
            hasType = true;
        }
        else {
            cout << "Invalid event type, ignoring.\n";
        }
    }

    // ===== FACULTY (ONLY IF FACULTY EVENT) =====
    int facultyId = 0;
    if (hasType && eventType == "Faculty") {
        facultyId = listAndPickByIndex(
            con, "faculty", "Faculty_ID", "FacultyCode", "Faculty"
        );
    }

    // ===== BUILD QUERY =====
    string sql =
        "SELECT e.Event_ID, e.Title, e.EventDate, e.Location, e.Status, "
        "c.Name AS CategoryName, "
        "r.Event_type AS EventType, "
        "f.FacultyCode "
        "FROM event e "
        "LEFT JOIN category c ON e.Category_ID = c.Category_ID "
        "LEFT JOIN rewardrule r ON e.Rule_ID = r.Rule_ID "
        "LEFT JOIN faculty f ON e.Faculty_ID = f.Faculty_ID";

    bool first = true;

    if (categoryId != 0) {
        sql += first ? " WHERE " : " AND ";
        sql += "e.Category_ID = ?";
        first = false;
    }

    if (hasStatus) {
        sql += first ? " WHERE " : " AND ";
        sql += "LOWER(e.Status) = ?";
        first = false;
    }

    if (hasType) {
        sql += first ? " WHERE " : " AND ";
        sql += "r.Event_type = ?";
        first = false;
    }

    if (hasType && eventType == "Faculty" && facultyId != 0) {
        sql += first ? " WHERE " : " AND ";
        sql += "e.Faculty_ID = ?";
        first = false;
    }

    sql += " ORDER BY e.EventDate";

    // ===== EXECUTE =====
    try {
        sql::PreparedStatement* ps = con->prepareStatement(sql);
        int idx = 1;

        if (categoryId != 0) ps->setInt(idx++, categoryId);
        if (hasStatus) ps->setString(idx++, status);
        if (hasType) ps->setString(idx++, eventType);
        if (hasType && eventType == "Faculty" && facultyId != 0)
            ps->setInt(idx++, facultyId);

        sql::ResultSet* rs = ps->executeQuery();

        bool any = false;
        cout << "\n--- FILTERED EVENTS ---\n";
        while (rs->next()) {
            any = true;
            cout << "Event ID : " << rs->getInt("Event_ID") << "\n";
            cout << "Title    : " << rs->getString("Title") << "\n";
            cout << "Type     : " << rs->getString("EventType") << "\n";
            cout << "Date     : " << rs->getString("EventDate") << "\n";
            cout << "Location : " << rs->getString("Location") << "\n";
            cout << "Status   : " << rs->getString("Status") << "\n";
            cout << "Category : "
                << (rs->isNull("CategoryName") ? "N/A" : rs->getString("CategoryName")) << "\n";

            if (rs->getString("EventType") == "Faculty") {
                cout << "Faculty  : " << rs->getString("FacultyCode") << "\n";
            }
            cout << "-------------------------------------------\n";
        }

        if (!any)
            cout << "No events matched your filters.\n";

        delete rs;
        delete ps;
    }
    catch (sql::SQLException& e) {
        cout << "Error filtering events: " << e.what() << endl;
    }

    cout << "Press Enter to continue...";
    string junk;
    getline(cin, junk);
}


// ================== MANAGE ATTENDANCE ==================
static void manageAttendanceMenu(sql::Connection* con, int userId)
{
    updateEventStatus(con);
    while (true) {
        printHeader("MANAGE ATTENDANCE");

        cout << "1. Manage ACTIVE event attendance\n";
        cout << "2. Late attendance (ENDED event)\n";
        cout << "3. Back\n\n";

        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            string junk; getline(cin, junk);
            continue;
        }
        string junk; getline(cin, junk);


        if (choice == 3) return;
        if (choice != 1 && choice != 2) continue;


        // ================= SELECT EVENT OWNED BY STAFF =================

        if (choice == 1) {

            int eventId = 0;
            try {
                // ===== DISPLAY ACTIVE EVENTS =====
                sql::PreparedStatement* psList = con->prepareStatement(
                    "SELECT Event_ID, Title, Status "
                    "FROM event "
                    "WHERE Organizer_User_ID = ? AND Status = 'Active'"
                );
                psList->setInt(1, userId);
                sql::ResultSet* rsList = psList->executeQuery();
                printHeader("MY ACTIVE EVENTS");
                bool any = false;
                while (rsList->next()) {
                    any = true;
                    cout << "Event ID : " << rsList->getInt("Event_ID") << "\n";
                    cout << "Title    : " << rsList->getString("Title") << "\n";
                    cout << "Status   : " << rsList->getString("Status") << "\n";
                    cout << "--------------------------------------------\n";
                }

                delete rsList;
                delete psList;

                if (!any) {
                    cout << "No active events under your responsibility.\n";
                    continue;
                }

                cout << "Enter Event_ID (0 = cancel): ";
                cin >> eventId;
                getline(cin, junk);

                if (eventId == 0) return;

                // ===== VALIDATE EVENT =====
                bool validEvent = false;

                sql::PreparedStatement* psCheck = con->prepareStatement(
                    "SELECT COUNT(*) FROM event "
                    "WHERE Event_ID = ? AND Status = 'Active' AND Organizer_User_ID = ?"
                );
                psCheck->setInt(1, eventId);
                psCheck->setInt(2, userId);

                sql::ResultSet* rsCheck = psCheck->executeQuery();
                if (rsCheck->next() && rsCheck->getInt(1) > 0)
                    validEvent = true;

                delete rsCheck;
                delete psCheck;

                if (!validEvent) {
                    cout << "Invalid Event ID or event is not active.\n";
                    system("pause");
                    continue;
                }
            }
            catch (sql::SQLException& e) {
                cout << "Error loading events: " << e.what() << endl;
                continue;
            }
            // ================= EVENT SUBMENU =================
            while (true) {
                printHeader("EVENT ATTENDANCE");

                cout << "1. View registered students\n";
                cout << "2. Check-in student\n";
                cout << "3. Check-out student\n";
                cout << "4. Back\n\n";

                cout << "Enter choice: ";

                int sub;
                if (!(cin >> sub)) {
                    cin.clear();
                    getline(cin, junk);
                    continue;
                }
                getline(cin, junk);

                if (sub == 5) break;

                // ========== VIEW REGISTERED ==========
                if (sub == 1) {
                    printHeader("REGISTERED STUDENTS");

                    try {
                        sql::PreparedStatement* ps = con->prepareStatement(
                            "SELECT p.Participation_ID, s.Student_ID, s.Name, s.Matric_No, "
                            "a.Check_In_Time, a.Check_Out_Time, a.Status AS AttendanceStatus "
                            "FROM participation p "
                            "JOIN student s ON p.Student_ID = s.Student_ID "
                            "LEFT JOIN attendance a ON p.Participation_ID = a.Participation_ID "
                            "WHERE p.Event_ID = ? "
                            "ORDER BY s.Name"
                        );
                        ps->setInt(1, eventId);
                        sql::ResultSet* rs = ps->executeQuery();

                        bool any = false;
                        while (rs->next()) {
                            any = true;

                            string studentId = rs->getString("Student_ID");
                            string name = rs->getString("Name");
                            string matric = rs->getString("Matric_No");

                            bool hasCheckIn = !rs->isNull("Check_In_Time");
                            bool hasCheckOut = !rs->isNull("Check_Out_Time");
                            string attStat = rs->isNull("AttendanceStatus") ? "" : rs->getString("AttendanceStatus");

                            // ===== Determine Attendance Status =====
                            string status;
                            if (!hasCheckIn) {
                                status = "Not Checked-in";
                            }
                            else if (hasCheckIn && !hasCheckOut) {
                                status = "Checked-in (No Checkout)";
                            }
                            else {
                                if (attStat == "LATE")      status = "Completed (Late)";
                                else if (attStat == "COMPLETED") status = "Completed";
                                else status = "Completed";
                            }

                            // ===== Output =====
                            cout << "Student ID : " << studentId << "\n";
                            cout << "Name       : " << name << "\n";
                            cout << "Matric No  : " << matric << "\n";
                            cout << "Attendance : " << status << "\n";
                            cout << "--------------------------------------------\n";
                        }

                        if (!any)
                            cout << "No registered students.\n";

                        delete rs;
                        delete ps;

                        cout << "Press Enter to continue...";
                        cin.get();
                    }
                    catch (sql::SQLException& e) {
                        cout << "Error: " << e.what() << endl;
                    }
                }

                //CHECK IN
                else if (sub == 2) {
                    printHeader("CHECK-IN STUDENT");

                    // Display event info (from eventId context)
                    sql::PreparedStatement* psEvent = con->prepareStatement(
                        "SELECT Title, "
                        "DATE_FORMAT(EventDate, '%d %M %Y, %h:%i %p') AS EventDate, "
                        "DurationHours, Location "
                        "FROM event WHERE Event_ID = ?"
                    );
                    psEvent->setInt(1, eventId);
                    sql::ResultSet* rsEvent = psEvent->executeQuery();

                    if (rsEvent->next()) {
                        cout << rsEvent->getString("Title") << "\n";
                        cout << "Date     : " << rsEvent->getString("EventDate") << "\n";
                        cout << "Duration : " << rsEvent->getDouble("DurationHours") << " Hours\n";
                        cout << "Venue    : " << rsEvent->getString("Location") << "\n";
                        cout << "-----------------------------------------------------\n";
                    }
                    delete rsEvent;
                    delete psEvent;

                    cout << "Enter Matric No or Student ID (0 = cancel): ";
                    string input;
                    getline(cin, input);
                    if (input == "0") continue;

                    try {
                        // 1️⃣ Find participation for this event + student
                        sql::PreparedStatement* psFind = con->prepareStatement(
                            "SELECT p.Participation_ID "
                            "FROM participation p "
                            "JOIN student s ON p.Student_ID = s.Student_ID "
                            "WHERE p.Event_ID = ? AND (s.Matric_No = ? OR s.Student_ID = ?)"
                        );
                        psFind->setInt(1, eventId);
                        psFind->setString(2, input);
                        psFind->setString(3, input);

                        sql::ResultSet* rsFind = psFind->executeQuery();
                        if (!rsFind->next()) {
                            cout << "Student not registered for this event.\n";
                            delete rsFind; delete psFind;
                            cout << "Press Enter to continue...";
                            cin.get();
                            continue;
                        }

                        int pid = rsFind->getInt("Participation_ID");
                        delete rsFind; delete psFind;

                        // 2️⃣ prevent double check-in
                        sql::PreparedStatement* chk = con->prepareStatement(
                            "SELECT 1 FROM attendance "
                            "WHERE Participation_ID = ? AND Check_Out_Time IS NULL"
                        );
                        chk->setInt(1, pid);
                        sql::ResultSet* r = chk->executeQuery();

                        if (r->next()) {
                            cout << "Already checked-in.\n";
                            delete r; delete chk;
                            cout << "Press Enter to continue...";
                            cin.get();
                            continue;
                        }
                        delete r; delete chk;

                        // 3️⃣ staff does NOT need passcode → override logic
                        sql::PreparedStatement* ins = con->prepareStatement(
                            "INSERT INTO attendance (Participation_ID, Check_In_Time, Status) "
                            "VALUES (?, NOW(), 'IN')"
                        );
                        ins->setInt(1, pid);
                        ins->executeUpdate();
                        delete ins;

                        cout << "Check-in successful.\n";
                        cout << "Press Enter to continue...";
                        cin.get();
                    }
                    catch (sql::SQLException& e) {
                        cout << "Error: " << e.what() << endl;
                    }
                }
                else if (sub == 3) {
                    printHeader("CHECK-OUT STUDENT");

                    cout << "Enter Matric No (or Student ID) (0 = cancel): ";
                    string input;
                    getline(cin, input);
                    if (input == "0") continue;

                    try {
                        // 1️⃣ Find Participation_ID for student in current event
                        sql::PreparedStatement* psFind = con->prepareStatement(
                            "SELECT p.Participation_ID, s.Student_ID "
                            "FROM participation p "
                            "JOIN student s ON p.Student_ID = s.Student_ID "
                            "WHERE p.Event_ID = ? AND (s.Matric_No = ? OR s.Student_ID = ?)"
                        );
                        psFind->setInt(1, eventId);
                        psFind->setString(2, input);
                        psFind->setString(3, input);

                        sql::ResultSet* rsFind = psFind->executeQuery();
                        if (!rsFind->next()) {
                            cout << "Student not registered for this event.\n";
                            delete rsFind; delete psFind;
                            cout << "Press Enter to continue..."; cin.get();
                            continue;
                        }

                        int pid = rsFind->getInt("Participation_ID");
                        string realStudentId = rsFind->getString("Student_ID");
                        delete rsFind;
                        delete psFind;

                        // 2️⃣ ensure student is IN (not already checked-out)
                        sql::PreparedStatement* psIn = con->prepareStatement(
                            "SELECT Attendance_ID "
                            "FROM attendance "
                            "WHERE Participation_ID = ? AND Check_Out_Time IS NULL"
                        );
                        psIn->setInt(1, pid);
                        sql::ResultSet* rsIn = psIn->executeQuery();

                        if (!rsIn->next()) {
                            cout << "Student has not checked in or already checked out.\n";
                            delete rsIn; delete psIn;
                            cout << "Press Enter to continue..."; cin.get();
                            continue;
                        }

                        int aid = rsIn->getInt("Attendance_ID");
                        delete rsIn;
                        delete psIn;

                        // 3️⃣ get multiplier
                        double multiplier = 0.0;
                        sql::PreparedStatement* psRule = con->prepareStatement(
                            "SELECT rr.Multiplier "
                            "FROM participation p "
                            "JOIN event e ON p.Event_ID = e.Event_ID "
                            "JOIN rewardrule rr ON e.Rule_ID = rr.Rule_ID "
                            "WHERE p.Participation_ID = ?"
                        );
                        psRule->setInt(1, pid);
                        sql::ResultSet* rsRule = psRule->executeQuery();
                        rsRule->next();
                        multiplier = rsRule->getDouble("Multiplier");
                        delete rsRule;
                        delete psRule;

                        // 4️⃣ compute raw hours
                        double hours = 0.0;
                        sql::PreparedStatement* psHours = con->prepareStatement(
                            "SELECT TIMESTAMPDIFF(MINUTE, Check_In_Time, NOW()) / 60.0 AS Hours "
                            "FROM attendance WHERE Attendance_ID = ?"
                        );
                        psHours->setInt(1, aid);
                        sql::ResultSet* rsHours = psHours->executeQuery();
                        rsHours->next();
                        hours = rsHours->getDouble("Hours");
                        delete rsHours;
                        delete psHours;

                        if (hours < 0) hours = 0; // safety only

                        // 5️⃣ calculate points
                        double points = hours * multiplier;

                        // 6️⃣ round both to 2dp
                        hours = round(hours * 100.0) / 100.0;
                        points = round(points * 100.0) / 100.0;

                        // 7️⃣ update attendance as COMPLETED
                        sql::PreparedStatement* psOut = con->prepareStatement(
                            "UPDATE attendance "
                            "SET Check_Out_Time = NOW(), Status='COMPLETED', AttendanceHours=? "
                            "WHERE Attendance_ID = ?"
                        );
                        psOut->setDouble(1, hours);
                        psOut->setInt(2, aid);
                        psOut->executeUpdate();
                        delete psOut;

                        // 8️⃣ update participation points + marked
                        sql::PreparedStatement* psPart = con->prepareStatement(
                            "UPDATE participation "
                            "SET Points_earned=?, Attendance_marked=1 "
                            "WHERE Participation_ID=?"
                        );
                        psPart->setDouble(1, points);
                        psPart->setInt(2, pid);
                        psPart->executeUpdate();
                        delete psPart;

                        // 9️⃣ output clean
                        cout << fixed << setprecision(2);
                        cout << "Check-out successful.\n";
                        cout << "Hours   : " << hours << "\n";
                        cout << "Points  : " << points << "\n";

                        updateStudentSummary(con, realStudentId);
                        cout << "Press Enter to continue...";
                        cin.get();
                    }
                    catch (sql::SQLException& e) {
                        cout << "Error: " << e.what() << endl;
                    }
                }
                else if (sub == 4) {
                    break;
                }

            }
        }
        else if (choice == 2) {
            printHeader("LATE ATTENDANCE - ENDED EVENTS");

            int eventId = 0;

            try {
                // 1️⃣ List ended events
                sql::PreparedStatement* psList = con->prepareStatement(
                    "SELECT Event_ID, Title, "
                    "DATE_FORMAT(EventDate, '%d %M %Y, %h:%i %p') AS EventDate, "
                    "DurationHours, Location "
                    "FROM event "
                    "WHERE Organizer_User_ID = ? AND Status = 'Ended'"
                );
                psList->setInt(1, userId);

                sql::ResultSet* rsList = psList->executeQuery();
                int eidList[50];
                int ecount = 0;

                while (rsList->next()) {
                    ecount++;
                    eidList[ecount] = rsList->getInt("Event_ID");
                    cout << ecount << ") " << rsList->getString("Title") << "\n";
                    cout << "   Date     : " << rsList->getString("EventDate") << "\n";
                    cout << "   Duration : " << rsList->getDouble("DurationHours") << " Hours\n";
                    cout << "   Venue    : " << rsList->getString("Location") << "\n";
                    cout << "----------------------------------------------------\n";
                }

                delete rsList;
                delete psList;

                if (ecount == 0) {
                    cout << "No ended events available.\n";
                    cin.get();
                    continue;
                }

                cout << "Select event (1-" << ecount << ", 0 = cancel): ";
                int opt;
                cin >> opt;
                string junk; getline(cin, junk);
                if (opt == 0) continue;
                if (opt < 1 || opt > ecount) {
                    cout << "Invalid selection.\n";
                    cin.get();
                    continue;
                }

                eventId = eidList[opt];

                // 2️⃣ Load event info
                sql::PreparedStatement* psEvent = con->prepareStatement(
                    "SELECT EventDate, DurationHours, Rule_ID "
                    "FROM event WHERE Event_ID = ?"
                );
                psEvent->setInt(1, eventId);

                sql::ResultSet* rsEvent = psEvent->executeQuery();
                rsEvent->next();

                string eventStartStr = rsEvent->getString("EventDate");
                double duration = rsEvent->getDouble("DurationHours");
                int ruleId = rsEvent->getInt("Rule_ID");

                delete rsEvent;
                delete psEvent;

                // compute eventEnd = eventStart + durationHours
                sql::PreparedStatement* psEnd = con->prepareStatement(
                    "SELECT DATE_ADD(?, INTERVAL ? HOUR) AS EventEnd"
                );
                psEnd->setString(1, eventStartStr);
                psEnd->setDouble(2, duration);

                sql::ResultSet* rsEnd = psEnd->executeQuery();
                rsEnd->next();
                string eventEndStr = rsEnd->getString("EventEnd");

                delete rsEnd;
                delete psEnd;

                cout << "Event Selected:\n";
                cout << "Start : " << eventStartStr << "\n";
                cout << "End   : " << eventEndStr << "\n";
                cout << "Duration: " << duration << " Hours\n";
                cout << "----------------------------------------------------\n";

                // 3️⃣ Load student groups for late fixing

                // Group A: no attendance row
                sql::PreparedStatement* psA = con->prepareStatement(
                    "SELECT p.Participation_ID, s.Student_ID, s.Name "
                    "FROM participation p "
                    "JOIN student s ON p.Student_ID=s.Student_ID "
                    "LEFT JOIN attendance a ON p.Participation_ID=a.Participation_ID "
                    "WHERE p.Event_ID=? AND a.Attendance_ID IS NULL"
                );
                psA->setInt(1, eventId);
                sql::ResultSet* rsA = psA->executeQuery();

                // Group B: check-in only
                sql::PreparedStatement* psB = con->prepareStatement(
                    "SELECT p.Participation_ID, s.Student_ID, s.Name, "
                    "DATE_FORMAT(a.Check_In_Time, '%Y-%m-%d %H:%i') AS CheckIn "
                    "FROM participation p "
                    "JOIN student s ON p.Student_ID=s.Student_ID "
                    "JOIN attendance a ON p.Participation_ID=a.Participation_ID "
                    "WHERE p.Event_ID=? AND a.Check_Out_Time IS NULL"
                );
                psB->setInt(1, eventId);
                sql::ResultSet* rsB = psB->executeQuery();

                int pidList[100];
                string sidList[100];
                string checkinList[100];
                bool modeList[100]; // false = no attendance, true = check-in only
                int scount = 0;

                cout << "Eligible Students for Late Attendance:\n";
                cout << "----------------------------------------------------\n";

                while (rsA->next()) {
                    scount++;
                    pidList[scount] = rsA->getInt("Participation_ID");
                    sidList[scount] = rsA->getString("Student_ID");
                    checkinList[scount] = "";
                    modeList[scount] = false;
                    cout << scount << ") " << rsA->getString("Name") << " (NO ATTENDANCE)\n";
                }

                while (rsB->next()) {
                    scount++;
                    pidList[scount] = rsB->getInt("Participation_ID");
                    sidList[scount] = rsB->getString("Student_ID");
                    checkinList[scount] = rsB->getString("CheckIn");
                    modeList[scount] = true;
                    cout << scount << ") " << rsB->getString("Name") << " (CHECK-IN: "
                        << checkinList[scount] << ")\n";
                }

                delete rsA; delete psA;
                delete rsB; delete psB;

                if (scount == 0) {
                    cout << "No students eligible.\n";
                    cin.get();
                    continue;
                }

                cout << "Select student (1-" << scount << ", 0 = cancel): ";
                int pick;
                cin >> pick; getline(cin, junk);
                if (pick == 0) continue;
                if (pick < 1 || pick > scount) {
                    cout << "Invalid selection.\n";
                    cin.get();
                    continue;
                }

                int pid = pidList[pick];
                string realSID = sidList[pick];
                bool hasCheckIn = modeList[pick];

                string checkInManual = "";
                string checkOutManual = "";

                // 4️⃣ Input manual datetime
                if (!hasCheckIn) {
                retryCheckIn:
                    cout << "Enter Check-In (YYYY-MM-DD HH:MM, 0=cancel): ";
                    getline(cin, checkInManual);
                    if (checkInManual == "0") continue;
                }
                else {
                    checkInManual = checkinList[pick];
                    cout << "Existing Check-In: " << checkInManual << "\n";
                }

                retryCheckOut:
                cout << "Enter Check-Out (YYYY-MM-DD HH:MM, 0=cancel): ";
                getline(cin, checkOutManual);
                if (checkOutManual == "0") continue;

                // 5️⃣ Validation
                sql::PreparedStatement* psValid = con->prepareStatement(
                    "SELECT "
                    "TIMESTAMP(?) >= TIMESTAMP(?) AS v1, "
                    "TIMESTAMP(?) <= TIMESTAMP(?) AS v2, "
                    "TIMESTAMP(?) <= TIMESTAMP(?) AS v3"
                );
                psValid->setString(1, checkInManual);
                psValid->setString(2, eventStartStr);
                psValid->setString(3, checkOutManual);
                psValid->setString(4, eventEndStr);
                psValid->setString(5, checkInManual);
                psValid->setString(6, checkOutManual);

                sql::ResultSet* rsValid = psValid->executeQuery();
                rsValid->next();

                bool v1 = rsValid->getBoolean("v1");
                bool v2 = rsValid->getBoolean("v2");
                bool v3 = rsValid->getBoolean("v3");

                delete rsValid;
                delete psValid;

                if (!v1 || !v2 || !v3) {
                    cout << "Invalid time range. Must satisfy:\n";
                    cout << "- Check-In >= Event Start\n";
                    cout << "- Check-Out <= Event End\n";
                    cout << "- Check-Out >= Check-In\n";
                    goto retryCheckOut;
                }

                // 6️⃣ compute hours & points
                sql::PreparedStatement* psHours = con->prepareStatement(
                    "SELECT TIMESTAMPDIFF(MINUTE, ?, ?) / 60.0 AS Hours"
                );
                psHours->setString(1, checkInManual);
                psHours->setString(2, checkOutManual);

                sql::ResultSet* rsHours = psHours->executeQuery();
                rsHours->next();

                double hours = rsHours->getDouble("Hours");
                delete rsHours;
                delete psHours;

                if (hours < 0) hours = 0;

                sql::PreparedStatement* psRule = con->prepareStatement(
                    "SELECT Multiplier FROM rewardrule WHERE Rule_ID=?"
                );
                psRule->setInt(1, ruleId);
                sql::ResultSet* rsRule = psRule->executeQuery();
                rsRule->next();

                double multiplier = rsRule->getDouble("Multiplier");

                delete rsRule;
                delete psRule;

                double points = hours * multiplier;

                hours = round(hours * 100.0) / 100.0;
                points = round(points * 100.0) / 100.0;

                // 7️⃣ DB write
                if (!hasCheckIn) {
                    sql::PreparedStatement* psIns = con->prepareStatement(
                        "INSERT INTO attendance (Participation_ID, Check_In_Time, "
                        "Check_Out_Time, AttendanceHours, Status) "
                        "VALUES (?,?,?,?, 'LATE')"
                    );
                    psIns->setInt(1, pid);
                    psIns->setString(2, checkInManual);
                    psIns->setString(3, checkOutManual);
                    psIns->setDouble(4, hours);
                    psIns->executeUpdate();
                    delete psIns;
                }
                else {
                    sql::PreparedStatement* psUpd = con->prepareStatement(
                        "UPDATE attendance "
                        "SET Check_Out_Time=?, AttendanceHours=?, Status='LATE' "
                        "WHERE Participation_ID=? AND Check_Out_Time IS NULL"
                    );
                    psUpd->setString(1, checkOutManual);
                    psUpd->setDouble(2, hours);
                    psUpd->setInt(3, pid);
                    psUpd->executeUpdate();
                    delete psUpd;
                }

                sql::PreparedStatement* psPart = con->prepareStatement(
                    "UPDATE participation SET Points_earned=?, Attendance_marked=1 "
                    "WHERE Participation_ID=?"
                );
                psPart->setDouble(1, points);
                psPart->setInt(2, pid);
                psPart->executeUpdate();
                delete psPart;

                updateStudentSummary(con, realSID);

                cout << fixed << setprecision(2);
                cout << "Late attendance recorded.\n";
                cout << "Hours  : " << hours << "\n";
                cout << "Points : " << points << "\n";
                cout << "Press Enter to continue...";
                cin.get();
            }
            catch (sql::SQLException& e) {
                cout << "Error: " << e.what() << endl;
            }
        }
    }
}

// ================== VIEW REGISTERED STUDENTS (FULL FLOW) ==================
static void viewRegisteredStudents(sql::Connection* con, int userId)
{
    printHeader("VIEW REGISTERED STUDENTS");

    try {
        // ================== 1️⃣ PILIH STATUS ==================
        cout << "\nSelect event status:\n";
        cout << "1. Upcoming\n";
        cout << "2. Ended\n";
        cout << "3. All (except Active)\n";
        cout << "Enter choice (0 = back): ";

        int statusChoice;
        if (!(cin >> statusChoice)) {
            cin.clear();
            string junk; getline(cin, junk);
            cout << "Invalid input.\n";
            return;
        }
        string junk; getline(cin, junk);

        if (statusChoice == 0) return;

        string statusFilter;
        if (statusChoice == 1)
            statusFilter = "e.Status = 'Upcoming'";
        else if (statusChoice == 2)
            statusFilter = "e.Status = 'Ended'";
        else if (statusChoice == 3)
            statusFilter = "e.Status <> 'Active'";
        else {
            cout << "Invalid choice.\n";
            return;
        }

        // ================== 2️⃣ LIST EVENT STAFF ==================
        string query =
            "SELECT e.Event_ID, e.Title, e.EventDate, e.Status, "
            "COUNT(p.Participation_ID) AS TotalRegistered "
            "FROM event e "
            "LEFT JOIN participation p ON e.Event_ID = p.Event_ID "
            "WHERE e.Organizer_User_ID = ? AND " + statusFilter +
            " GROUP BY e.Event_ID "
            "ORDER BY e.EventDate";

        sql::PreparedStatement* ps = con->prepareStatement(query);
        ps->setInt(1, userId);
        sql::ResultSet* rs = ps->executeQuery();

        bool anyEvent = false;
        printHeader("REGISTERED STUDENTS");
        while (rs->next()) {
            anyEvent = true;
            cout << "Event ID    : " << rs->getInt("Event_ID") << "\n";
            cout << "Title       : " << rs->getString("Title") << "\n";
            cout << "Date        : " << rs->getString("EventDate") << "\n";
            cout << "Status      : " << rs->getString("Status") << "\n";
            cout << "Registered  : " << rs->getInt("TotalRegistered") << "\n";
            cout << "--------------------------------------------\n";
        }

        delete rs;
        delete ps;

        if (!anyEvent) {
            cout << "No events found.\n";
            cout << "Press Enter to continue...";
            getline(cin, junk);
            return;
        }

        // ================== 3️⃣ PILIH EVENT ==================
        cout << "\nEnter Event_ID to view registered students (0 = back): ";
        int eventId;
        if (!(cin >> eventId)) {
            cin.clear();
            getline(cin, junk);
            cout << "Invalid input.\n";
            return;
        }
        getline(cin, junk);

        if (eventId == 0) return;

        // ================== 4️⃣ CHECK STATUS EVENT ==================
        string eventStatus;
        ps = con->prepareStatement(
            "SELECT Status FROM event WHERE Event_ID = ? AND Organizer_User_ID = ?"
        );
        ps->setInt(1, eventId);
        ps->setInt(2, userId);
        rs = ps->executeQuery();

        if (!rs->next()) {
            cout << "Invalid Event ID or no access.\n";
            delete rs;
            delete ps;
            return;
        }

        eventStatus = rs->getString("Status");
        delete rs;
        delete ps;

        // ================== 5️⃣ DISPLAY STUDENT ==================
        if (eventStatus == "Upcoming") {

            ps = con->prepareStatement(
                "SELECT s.Student_ID, s.Name, s.Matric_No, p.RegisteredDate "
                "FROM participation p "
                "JOIN student s ON p.Student_ID = s.Student_ID "
                "JOIN event e ON p.Event_ID = e.Event_ID "
                "WHERE p.Event_ID = ? AND e.Organizer_User_ID = ? "
                "ORDER BY s.Name"
            );
            ps->setInt(1, eventId);
            ps->setInt(2, userId);

            rs = ps->executeQuery();

            printHeader("REGISTERED STUDENTS");
            bool any = false;
            while (rs->next()) {
                any = true;
                cout << "Student ID      : " << rs->getString("Student_ID") << "\n";
                cout << "Name            : " << rs->getString("Name") << "\n";
                cout << "Matric No       : " << rs->getString("Matric_No") << "\n";
                cout << "Registered Date : " << rs->getString("RegisteredDate") << "\n";
                cout << "--------------------------------------------\n";
            }

            if (!any)
                cout << "No students have registered yet.\n";

            delete rs;
            delete ps;
        }
        else if (eventStatus == "Ended") {

            ps = con->prepareStatement(
                "SELECT s.Student_ID, s.Name, s.Matric_No, p.RegisteredDate, "
                "a.Check_In_Time, a.Check_Out_Time, p.Points_earned "
                "FROM participation p "
                "JOIN student s ON p.Student_ID = s.Student_ID "
                "LEFT JOIN attendance a ON p.Participation_ID = a.Participation_ID "
                "JOIN event e ON p.Event_ID = e.Event_ID "
                "WHERE p.Event_ID = ? AND e.Organizer_User_ID = ? "
                "ORDER BY s.Name"
            );
            ps->setInt(1, eventId);
            ps->setInt(2, userId);

            rs = ps->executeQuery();

            cout << "\n--- Registered Students (Ended Event) ---\n";
            bool any = false;
            while (rs->next()) {
                any = true;
                cout << "Student ID      : " << rs->getString("Student_ID") << "\n";
                cout << "Name            : " << rs->getString("Name") << "\n";
                cout << "Matric No       : " << rs->getString("Matric_No") << "\n";
                cout << "Registered Date : " << rs->getString("RegisteredDate") << "\n";
                cout << "Check-in Time   : " << rs->getString("Check_In_Time") << "\n";
                cout << "Check-out Time  : " << rs->getString("Check_Out_Time") << "\n";
                cout << "Points Earned   : " << rs->getDouble("Points_earned") << "\n";
                cout << "--------------------------------------------\n";
            }

            if (!any)
                cout << "No participation records found.\n";

            delete rs;
            delete ps;
        }
        else {
            cout << "This event is currently active. Attendance is handled in Manage Attendance.\n";
        }
    }
    catch (sql::SQLException& e) {
        cout << "Error retrieving registrations: " << e.what() << endl;
    }

    cout << "Press Enter to continue...";
    string junk;
    getline(cin, junk);
}

void updateStaffProfile(sql::Connection* con, int userId)
{
    printHeader("UPDATE STAFF PROFILE");

    // ================= 1️⃣ FETCH CURRENT DATA =================
    string curName, curPhone, curEmail, curFacultyCode, staffId;
    int curFacultyId = -1;

    try {
        sql::PreparedStatement* q = con->prepareStatement(
            "SELECT st.Staff_ID, st.Name, st.Phone, u.Email, "
            "st.Faculty_ID, f.FacultyCode "
            "FROM staff st "
            "JOIN user u ON st.User_ID = u.User_ID "
            "LEFT JOIN faculty f ON st.Faculty_ID = f.Faculty_ID "
            "WHERE st.User_ID = ? LIMIT 1"
        );
        q->setInt(1, userId);

        sql::ResultSet* r = q->executeQuery();
        if (r->next()) {
            staffId = r->getString("Staff_ID");
            curName = r->getString("Name");
            curPhone = r->getString("Phone");
            curEmail = r->getString("Email");

            if (!r->isNull("Faculty_ID"))
                curFacultyId = r->getInt("Faculty_ID");
            if (!r->isNull("FacultyCode"))
                curFacultyCode = r->getString("FacultyCode");
        }
        delete r;
        delete q;
    }
    catch (sql::SQLException& e) {
        cout << "Error fetching staff profile: " << e.what() << endl;
        return;
    }

    // ================= 2️⃣ DISPLAY CURRENT PROFILE =================
    cout << "Staff ID  : " << staffId << "\n";
    cout << "Email     : " << curEmail << "\n";
    cout << "Name      : " << curName << "\n";
    cout << "Phone No  : " << curPhone << "\n";
    cout << "Faculty   : "
        << (curFacultyCode.empty() ? "(none)" : curFacultyCode) << "\n\n";

    // ================= 3️⃣ INPUT NEW VALUES =================
    cout << "New Name (press Enter to keep current): ";
    string name;
    getline(cin, name);
    if (name.empty()) name = curName;

    cout << "New Phone No (press Enter to keep current): ";
    string phone;
    getline(cin, phone);
    if (phone.empty()) phone = curPhone;

    cout << "New Faculty Code (e.g. FTMK) (press Enter to keep current): ";
    string facultyCode;
    getline(cin, facultyCode);

    int facultyId = curFacultyId;

    if (!facultyCode.empty()) {
        try {
            sql::PreparedStatement* stmt = con->prepareStatement(
                "SELECT Faculty_ID FROM faculty WHERE FacultyCode = ? LIMIT 1"
            );
            stmt->setString(1, facultyCode);

            sql::ResultSet* r = stmt->executeQuery();
            if (r->next()) {
                facultyId = r->getInt("Faculty_ID");
            }
            else {
                cout << "Invalid Faculty Code. Faculty not changed.\n";
                facultyId = curFacultyId;
            }
            delete r;
            delete stmt;
        }
        catch (sql::SQLException& e) {
            cout << "Faculty lookup error: " << e.what() << endl;
        }
    }

    // ================= 4️⃣ UPDATE DATABASE =================
    try {
        sql::PreparedStatement* up = con->prepareStatement(
            "UPDATE staff SET Name = ?, Phone = ?, Faculty_ID = ? WHERE User_ID = ?"
        );
        up->setString(1, name);
        up->setString(2, phone);

        if (facultyId > 0)
            up->setInt(3, facultyId);
        else
            up->setNull(3, sql::DataType::INTEGER);

        up->setInt(4, userId);

        int rows = up->executeUpdate();
        delete up;

        if (rows > 0)
            cout << "\nProfile updated successfully.\n";
        else
            cout << "\nNo changes made.\n";
    }
    catch (sql::SQLException& e) {
        cout << "Error updating staff profile: " << e.what() << endl;
    }

    cout << "Press Enter to continue...";
    string tmp;
    getline(cin, tmp);
}

void viewStaffProfile(sql::Connection* con, int userId)
{
    printHeader("VIEW STAFF PROFILE");

    string staffId, name, phone, email, facultyCode;

    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT st.Staff_ID, st.Name, st.Phone, u.Email, "
            "f.FacultyCode "
            "FROM staff st "
            "JOIN user u ON st.User_ID = u.User_ID "
            "LEFT JOIN faculty f ON st.Faculty_ID = f.Faculty_ID "
            "WHERE st.User_ID = ? LIMIT 1"
        );
        ps->setInt(1, userId);

        sql::ResultSet* rs = ps->executeQuery();

        if (rs->next()) {
            staffId = rs->getString("Staff_ID");
            name = rs->getString("Name");
            phone = rs->getString("Phone");
            email = rs->getString("Email");
            if (!rs->isNull("FacultyCode"))
                facultyCode = rs->getString("FacultyCode");
        }
        else {
            cout << "Profile not found.\n";
            delete rs;
            delete ps;
            return;
        }

        delete rs;
        delete ps;
    }
    catch (sql::SQLException& e) {
        cout << "Error retrieving staff profile: " << e.what() << endl;
        return;
    }

    // ===== DISPLAY PROFILE =====
    cout << "Staff ID  : " << staffId << "\n";
    cout << "Name      : " << name << "\n";
    cout << "Phone No  : " << phone << "\n";
    cout << "Email     : " << email << "\n";
    cout << "Faculty   : "
        << (facultyCode.empty() ? "(none)" : facultyCode) << "\n";

    cout << "\nPress Enter to continue...";
    string junk;
    getline(cin, junk);
}

static void staffEventReportMenu(sql::Connection* con, int userId)
{
    while (true) {
        printHeader("EVENT REPORTING");

        cout << "Listing ended events organized by you:\n\n";

        try {
            sql::PreparedStatement* ps = con->prepareStatement(
                "SELECT Event_ID, Title, "
                "DATE_FORMAT(EventDate, '%d %M %Y %h:%i %p') AS EventTime, "
                "DurationHours "
                "FROM event "
                "WHERE Organizer_User_ID = ? AND Status = 'Ended' "
                "ORDER BY EventDate DESC"
            );
            ps->setInt(1, userId);
            sql::ResultSet* rs = ps->executeQuery();

            bool any = false;
            while (rs->next()) {
                any = true;
                cout << "ID: " << rs->getInt("Event_ID")
                    << " | " << rs->getString("Title")
                    << " | " << rs->getString("EventTime")
                    << " | " << rs->getDouble("DurationHours") << " hrs\n";
            }

            delete rs; delete ps;

            if (!any) {
                cout << "\n(No ended events found)\n";
                cout << "Press Enter to continue...";
                cin.get();
                return;
            }

        }
        catch (sql::SQLException& e) {
            cout << "\nError loading events: " << e.what() << endl;
            cin.get();
            return;
        }

        cout << "\nEnter Event ID to generate report (0 = back): ";
        int eid;
        cin >> eid;
        string junk; getline(cin, junk);
        if (eid == 0) return;

        generateSingleEventReport(con, userId, eid);
    }
}
static void generateSingleEventReport(sql::Connection* con, int userId, int eventId)
{
    printHeader("EVENT REPORT");

    // ===== VALIDATE OWNERSHIP + GET EVENT INFO =====
    string title, desc, eventTime, location, eventType, facultyName = "-";
    int capacity = 0, regCount = 0;
    double duration = 0.0;

    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT e.Title, e.Description, "
            "DATE_FORMAT(e.EventDate, '%d %M %Y %h:%i %p') AS EventTime, "
            "e.DurationHours, e.Location, e.Capacity, "
            "CASE WHEN e.Faculty_ID IS NULL THEN 'University' "
            "     ELSE 'Faculty' END AS EventType, "
            "f.FacultyName "
            "FROM event e "
            "LEFT JOIN faculty f ON e.Faculty_ID=f.Faculty_ID "
            "WHERE e.Event_ID=? AND e.Organizer_User_ID=?"
        );
        ps->setInt(1, eventId);
        ps->setInt(2, userId);

        sql::ResultSet* rs = ps->executeQuery();

        if (!rs->next()) {
            cout << "\nInvalid event or you do not have access.\n";
            delete rs; delete ps;
            cin.get();
            return;
        }

        title = rs->getString("Title");
        desc = rs->getString("Description");
        eventTime = rs->getString("EventTime");
        duration = rs->getDouble("DurationHours");
        location = rs->getString("Location");
        capacity = rs->getInt("Capacity");
        eventType = rs->getString("EventType");
        if (!rs->isNull("FacultyName")) facultyName = rs->getString("FacultyName");

        delete rs; delete ps;

    }
    catch (sql::SQLException& e) {
        cout << "\nError loading event: " << e.what() << endl;
        return;
    }

    // ===== COUNT REGISTERED (PARTICIPANTS) =====
    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT COUNT(*) FROM participation WHERE Event_ID=?"
        );
        ps->setInt(1, eventId);
        sql::ResultSet* rs = ps->executeQuery();
        rs->next();
        regCount = rs->getInt(1);
        delete rs; delete ps;
    }
    catch (...) {}

    // ===== PRINT HEADER =====
    cout << "Event ID     : " << eventId << "\n";
    cout << "Title        : " << title << "\n";
    cout << "Description  : " << desc << "\n";
    cout << "Date & Time  : " << eventTime << "\n";
    cout << "Duration     : " << duration << " hours\n";
    cout << "Location     : " << location << "\n";
    cout << "Capacity     : " << capacity << "\n";
    cout << "Slots        : " << regCount << " / " << capacity << "\n";
    cout << "Event Type   : " << eventType << "\n";
    if (eventType == "Faculty")
        cout << "Faculty      : " << facultyName << "\n";

    cout << "\nRegistered Students (" << regCount << ")\n";
    cout << "--------------------------------------------------------\n";

    // ===== FETCH AND DISPLAY STUDENTS =====
    try {
        sql::PreparedStatement* ps = con->prepareStatement(
            "SELECT s.Matric_No, s.Name, "
            "a.Check_In_Time, a.Check_Out_Time "
            "FROM participation p "
            "JOIN student s ON p.Student_ID=s.Student_ID "
            "LEFT JOIN attendance a ON p.Participation_ID=a.Participation_ID "
            "WHERE p.Event_ID=? "
            "ORDER BY s.Name"
        );
        ps->setInt(1, eventId);
        sql::ResultSet* rs = ps->executeQuery();

        int idx = 1;
        while (rs->next()) {
            cout << idx++ << ") "
                << rs->getString("Matric_No") << " | "
                << rs->getString("Name");

            bool hasIn = !rs->isNull("Check_In_Time");
            bool hasOut = !rs->isNull("Check_Out_Time");

            if (!hasIn && !hasOut) {
                // Case 1
                cout << " | NOT ATTENDED\n";
            }
            else if (hasIn && !hasOut) {
                // Case 2
                cout << " | INCOMPLETE\n";
                cout << "     Check-in  : " << rs->getString("Check_In_Time") << "\n";
                cout << "     Check-out : NONE\n";
            }
            else if (hasIn && hasOut) {
                // Case 3
                cout << " | ATTENDED\n";
                cout << "     Check-in  : " << rs->getString("Check_In_Time") << "\n";
                cout << "     Check-out : " << rs->getString("Check_Out_Time") << "\n";
            }

            cout << "--------------------------------------------------------\n";
        }


        delete rs; delete ps;

    }
    catch (sql::SQLException& e) {
        cout << "\nError loading attendance: " << e.what() << endl;
    }

    cout << "Press Enter to continue...";
    cin.get();
}





