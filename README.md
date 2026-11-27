# Student Volunteer Management System

A C++ console application for managing university volunteer events and student participation.

## Features

- Student, staff, and administrator menus
- Event registration and event management
- Attendance check-in and check-out
- Volunteer-hour and reward-point calculations
- Participation reports and rankings

## Project files

The Visual Studio solution and project are at the repository root. C++ source is organized by user role. `database/schema.sql` contains the database schema without the original exported sample records.

## Requirements

- Visual Studio 2022 with the C++ desktop development workload (v143 toolset)
- MySQL Connector/C++ 8.0.27, or a compatible version
- MySQL or MariaDB server

The original Visual Studio project references the Connector/C++ installation under `C:\Program Files\MySQL\mysql-connector-c++-8.0.27-winx64`. Update the include and library paths in the project settings if your installation differs.

## Database setup

1. Create a local database named `studentvolunteermanagementsystem`.
2. Import `database/schema.sql` into that database.
3. Configure the local connection values in `main.cpp` for your MySQL setup.
4. Build and run the solution from Visual Studio.

The original database export included sample rows, including student contact and identity fields. Those rows were left out of this repository copy.

## Notes

This is a standalone, console-based academic project. It has not been verified on a fresh machine with a new MySQL Connector/C++ installation.
