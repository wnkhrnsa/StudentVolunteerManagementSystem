#pragma once

#include <cppconn/connection.h>

// Staff main menu – dipanggil dari loginUser
void staffMenu(sql::Connection* con, int userId);
