#pragma once

#include <cppconn/connection.h>

// Fungsi untuk handle REGISTER dan LOGIN dari main menu
void handleRegistration(sql::Connection* con);
void handleLogin(sql::Connection* con);
void updateEventStatus(sql::Connection* con);
