-- phpMyAdmin SQL Dump
-- version 5.2.1
-- https://www.phpmyadmin.net/
--
-- Host: 127.0.0.1
-- Generation Time: Jan 13, 2026 at 08:04 AM
-- Server version: 10.4.32-MariaDB
-- PHP Version: 8.2.12

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `studentvolunteermanagementsystem`
--

-- --------------------------------------------------------

--
-- Table structure for table `attendance`
--

CREATE TABLE `attendance` (
  `Attendance_ID` int(10) UNSIGNED NOT NULL,
  `Participation_ID` bigint(20) UNSIGNED NOT NULL,
  `Check_In_Time` datetime DEFAULT NULL,
  `Check_Out_Time` datetime DEFAULT NULL,
  `Status` enum('IN','COMPLETED','LATE') NOT NULL,
  `AttendanceHours` double NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `attendance`
--


-- --------------------------------------------------------

--
-- Table structure for table `category`
--

CREATE TABLE `category` (
  `category_id` int(10) UNSIGNED NOT NULL,
  `name` varchar(120) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `category`
--


-- --------------------------------------------------------

--
-- Table structure for table `event`
--

CREATE TABLE `event` (
  `Event_ID` int(10) UNSIGNED NOT NULL,
  `Title` varchar(300) NOT NULL,
  `Description` text DEFAULT NULL,
  `EventDate` datetime DEFAULT NULL,
  `DurationHours` double DEFAULT 0,
  `Location` varchar(255) DEFAULT NULL,
  `Capacity` int(10) UNSIGNED DEFAULT 0,
  `Status` enum('Upcoming','Active','Ended','Cancelled') NOT NULL DEFAULT 'Upcoming',
  `Rule_ID` int(10) UNSIGNED DEFAULT NULL,
  `Category_ID` int(10) UNSIGNED DEFAULT NULL,
  `Organizer_User_ID` int(10) UNSIGNED DEFAULT NULL,
  `Faculty_ID` int(10) UNSIGNED DEFAULT NULL,
  `Attendance_Passcode` varchar(20) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `event`
--


-- --------------------------------------------------------

--
-- Table structure for table `faculty`
--

CREATE TABLE `faculty` (
  `Faculty_ID` int(10) UNSIGNED NOT NULL,
  `FacultyCode` varchar(20) NOT NULL,
  `FacultyName` varchar(255) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `faculty`
--


-- --------------------------------------------------------

--
-- Table structure for table `participation`
--

CREATE TABLE `participation` (
  `Participation_ID` bigint(20) UNSIGNED NOT NULL,
  `Student_ID` varchar(10) NOT NULL,
  `Event_ID` int(10) UNSIGNED NOT NULL,
  `RegisteredDate` datetime DEFAULT current_timestamp(),
  `Attendance_marked` tinyint(1) DEFAULT 0,
  `Points_earned` double DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `participation`
--


-- --------------------------------------------------------

--
-- Table structure for table `rewardrule`
--

CREATE TABLE `rewardrule` (
  `Rule_ID` int(10) UNSIGNED NOT NULL,
  `Name` varchar(150) NOT NULL,
  `Multiplier` double NOT NULL,
  `Event_type` varchar(50) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `rewardrule`
--


-- --------------------------------------------------------

--
-- Table structure for table `staff`
--

CREATE TABLE `staff` (
  `Staff_ID` varchar(10) NOT NULL,
  `User_ID` int(10) UNSIGNED NOT NULL,
  `Name` varchar(150) NOT NULL,
  `Phone` varchar(20) DEFAULT NULL,
  `Faculty_ID` int(10) UNSIGNED DEFAULT NULL,
  `IsAdmin` tinyint(1) NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `staff`
--


-- --------------------------------------------------------

--
-- Table structure for table `student`
--

CREATE TABLE `student` (
  `User_ID` int(10) UNSIGNED NOT NULL,
  `Name` varchar(255) NOT NULL,
  `Matric_No` varchar(80) NOT NULL,
  `NRIC` varchar(30) DEFAULT NULL,
  `Phone` varchar(40) DEFAULT NULL,
  `Status` varchar(30) DEFAULT 'Active',
  `Faculty_ID` int(10) UNSIGNED NOT NULL,
  `Student_ID` varchar(10) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `student`
--


-- --------------------------------------------------------

--
-- Table structure for table `studentsummary`
--

CREATE TABLE `studentsummary` (
  `Student_ID` varchar(10) NOT NULL,
  `Total_hours` double DEFAULT 0,
  `Total_points` double DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `studentsummary`
--


-- --------------------------------------------------------

--
-- Table structure for table `user`
--

CREATE TABLE `user` (
  `User_ID` int(10) UNSIGNED NOT NULL,
  `Email` varchar(200) NOT NULL,
  `Password` varchar(255) NOT NULL,
  `Role` varchar(50) NOT NULL,
  `IsAdmin` tinyint(1) DEFAULT 0,
  `Status` enum('Active','Inactive') NOT NULL DEFAULT 'Active'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `user`
--


--
-- Indexes for dumped tables
--

--
-- Indexes for table `attendance`
--
ALTER TABLE `attendance`
  ADD PRIMARY KEY (`Attendance_ID`),
  ADD KEY `Participation_ID` (`Participation_ID`);

--
-- Indexes for table `category`
--
ALTER TABLE `category`
  ADD PRIMARY KEY (`category_id`),
  ADD UNIQUE KEY `name` (`name`);

--
-- Indexes for table `event`
--
ALTER TABLE `event`
  ADD PRIMARY KEY (`Event_ID`),
  ADD KEY `Rule_ID` (`Rule_ID`),
  ADD KEY `Category_ID` (`Category_ID`),
  ADD KEY `Organizer_User_ID` (`Organizer_User_ID`),
  ADD KEY `Faculty_ID` (`Faculty_ID`);

--
-- Indexes for table `faculty`
--
ALTER TABLE `faculty`
  ADD PRIMARY KEY (`Faculty_ID`),
  ADD UNIQUE KEY `ux_faculty_code` (`FacultyCode`);

--
-- Indexes for table `participation`
--
ALTER TABLE `participation`
  ADD PRIMARY KEY (`Participation_ID`),
  ADD KEY `Event_ID` (`Event_ID`),
  ADD KEY `participation_ibfk_1` (`Student_ID`);

--
-- Indexes for table `rewardrule`
--
ALTER TABLE `rewardrule`
  ADD PRIMARY KEY (`Rule_ID`);

--
-- Indexes for table `staff`
--
ALTER TABLE `staff`
  ADD PRIMARY KEY (`Staff_ID`),
  ADD UNIQUE KEY `uq_staff_user` (`User_ID`),
  ADD KEY `fk_staff_faculty` (`Faculty_ID`);

--
-- Indexes for table `student`
--
ALTER TABLE `student`
  ADD PRIMARY KEY (`Student_ID`),
  ADD UNIQUE KEY `uniq_student_matric` (`Matric_No`),
  ADD UNIQUE KEY `User_ID` (`User_ID`) USING BTREE,
  ADD KEY `fk_student_faculty_ref` (`Faculty_ID`);

--
-- Indexes for table `studentsummary`
--
ALTER TABLE `studentsummary`
  ADD PRIMARY KEY (`Student_ID`);

--
-- Indexes for table `user`
--
ALTER TABLE `user`
  ADD PRIMARY KEY (`User_ID`),
  ADD UNIQUE KEY `Email` (`Email`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `attendance`
--
ALTER TABLE `attendance`
  MODIFY `Attendance_ID` int(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=93;

--
-- AUTO_INCREMENT for table `category`
--
ALTER TABLE `category`
  MODIFY `category_id` int(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=10;

--
-- AUTO_INCREMENT for table `event`
--
ALTER TABLE `event`
  MODIFY `Event_ID` int(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=59;

--
-- AUTO_INCREMENT for table `faculty`
--
ALTER TABLE `faculty`
  MODIFY `Faculty_ID` int(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=11;

--
-- AUTO_INCREMENT for table `participation`
--
ALTER TABLE `participation`
  MODIFY `Participation_ID` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=185;

--
-- AUTO_INCREMENT for table `rewardrule`
--
ALTER TABLE `rewardrule`
  MODIFY `Rule_ID` int(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=3;

--
-- AUTO_INCREMENT for table `user`
--
ALTER TABLE `user`
  MODIFY `User_ID` int(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=50;

--
-- Constraints for dumped tables
--

--
-- Constraints for table `attendance`
--
ALTER TABLE `attendance`
  ADD CONSTRAINT `attendance_ibfk_1` FOREIGN KEY (`Participation_ID`) REFERENCES `participation` (`Participation_ID`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `event`
--
ALTER TABLE `event`
  ADD CONSTRAINT `event_ibfk_1` FOREIGN KEY (`Rule_ID`) REFERENCES `rewardrule` (`Rule_ID`) ON DELETE SET NULL ON UPDATE CASCADE,
  ADD CONSTRAINT `event_ibfk_2` FOREIGN KEY (`Category_ID`) REFERENCES `category` (`category_id`) ON DELETE SET NULL ON UPDATE CASCADE,
  ADD CONSTRAINT `event_ibfk_3` FOREIGN KEY (`Organizer_User_ID`) REFERENCES `user` (`User_ID`) ON DELETE SET NULL ON UPDATE CASCADE,
  ADD CONSTRAINT `event_ibfk_4` FOREIGN KEY (`Faculty_ID`) REFERENCES `faculty` (`Faculty_ID`) ON DELETE SET NULL ON UPDATE CASCADE;

--
-- Constraints for table `participation`
--
ALTER TABLE `participation`
  ADD CONSTRAINT `participation_ibfk_1` FOREIGN KEY (`Student_ID`) REFERENCES `student` (`Student_ID`) ON DELETE CASCADE ON UPDATE CASCADE,
  ADD CONSTRAINT `participation_ibfk_2` FOREIGN KEY (`Event_ID`) REFERENCES `event` (`Event_ID`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `staff`
--
ALTER TABLE `staff`
  ADD CONSTRAINT `staff_ibfk_1` FOREIGN KEY (`User_ID`) REFERENCES `user` (`User_ID`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `student`
--
ALTER TABLE `student`
  ADD CONSTRAINT `student_ibfk_1` FOREIGN KEY (`User_ID`) REFERENCES `user` (`User_ID`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `studentsummary`
--
ALTER TABLE `studentsummary`
  ADD CONSTRAINT `studentsummary_ibfk_1` FOREIGN KEY (`Student_ID`) REFERENCES `student` (`Student_ID`) ON DELETE CASCADE ON UPDATE CASCADE;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
