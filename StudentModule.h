#pragma once

#include <cppconn/connection.h>

// Function yang main.cpp akan panggil
void studentMenu(sql::Connection* con, int userId);
void updateStudentSummary(sql::Connection* con, const std::string& studentId);
