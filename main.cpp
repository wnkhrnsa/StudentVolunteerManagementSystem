// main.cpp
// Student Volunteer Management System 
// DB: studentvolunteermanagementsystem

// main.cpp
#include "StudentModule.h"
#include "StaffModule.h"
#include "LogRegModule.h"


#include <stdlib.h>
#include <iostream>
#include <string>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/statement.h>


using namespace std;

// DB connection config 
const string server = "tcp://127.0.0.1:3306";
const string username = "root";
const string password = "";
const string schema = "studentvolunteermanagementsystem";

//print header
void printHeader(const string& title) {
    cout << "------------------------------------------------------------------------------------------------------------------------\n"; // 120 char
    cout << "                                           STUDENT VOLUNTEER SYSTEM\n";
    cout << "------------------------------------------------------------------------------------------------------------------------\n";
    if (!title.empty()) {
        cout << "                                               " << title << "\n";
        cout << "------------------------------------------------------------------------------------------------------------------------\n\n";
    }
}

int main() {
    sql::Driver* driver = nullptr;
    sql::Connection* con = nullptr;

    try {
        driver = get_driver_instance(); //connect to database
        con = driver->connect(server, username, password);
        con->setSchema(schema);
        cout << "Database connection successful!\n\n";
        updateEventStatus(con);
    }
    catch (sql::SQLException& e) {
        cout << "Could not connect to server. Error message: " << e.what() << endl;
        return 1;
    }
    while (true) {
        // Main Menu UI 
        printHeader("");
        cout << "1. REGISTER\n";
        cout << "2. LOGIN\n";
        cout << "3. EXIT\n\n";
        cout << "Enter choice: ";

        int choice = 0;
        if (!(cin >> choice)) {
            cin.clear();
            string tmp; getline(cin, tmp);
            continue;
        }
        string tmp; getline(cin, tmp);

        if (choice == 1) {
            handleRegistration(con);
        }
        else if (choice == 2) {
            handleLogin(con);
        }
        else if (choice == 3) {
            cout << "Exiting...\n";
            break;
        }
        else {
            cout << "Unknown option. Press Enter to continue...";
            string s; getline(cin, s);
        }
    }

    if (con) delete con;
    return 0;
}
