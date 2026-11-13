-- Local Online Judge Database Schema
-- Compatible with MySQL 8.0+

CREATE DATABASE IF NOT EXISTS local_online_judge
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE local_online_judge;

-- Table: contestants
CREATE TABLE IF NOT EXISTS contestants (
    id INT AUTO_INCREMENT PRIMARY KEY,
    student_id VARCHAR(64) NOT NULL UNIQUE,
    student_name VARCHAR(128) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- Table: problems
CREATE TABLE IF NOT EXISTS problems (
    id INT AUTO_INCREMENT PRIMARY KEY,
    problem_code VARCHAR(64) NOT NULL UNIQUE,
    problem_name VARCHAR(128) NOT NULL,
    time_limit_ms INT NOT NULL DEFAULT 1000,
    memory_limit_mb INT NOT NULL DEFAULT 256,
    grading_mode ENUM('ICPC', 'IOI', 'EQUAL') NOT NULL DEFAULT 'EQUAL',
    total_score DOUBLE NOT NULL DEFAULT 100.0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- Table: submissions
CREATE TABLE IF NOT EXISTS submissions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    contestant_id INT NOT NULL,
    problem_id INT NOT NULL,
    source_file VARCHAR(255) NOT NULL,
    verdict ENUM('AC', 'WA', 'TLE', 'MLE', 'RTE', 'CE', 'PENDING') NOT NULL DEFAULT 'PENDING',
    score DOUBLE NOT NULL DEFAULT 0.0,
    total_time_ms INT NOT NULL DEFAULT 0,
    max_memory_kb INT NOT NULL DEFAULT 0,
    compiler_output TEXT,
    grading_mode ENUM('ICPC', 'IOI', 'EQUAL') NOT NULL,
    submitted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_submission_contestant
        FOREIGN KEY (contestant_id) REFERENCES contestants(id)
        ON DELETE CASCADE,
    CONSTRAINT fk_submission_problem
        FOREIGN KEY (problem_id) REFERENCES problems(id)
        ON DELETE CASCADE
) ENGINE=InnoDB;

-- Table: submission_testcases
CREATE TABLE IF NOT EXISTS submission_testcases (
    id INT AUTO_INCREMENT PRIMARY KEY,
    submission_id INT NOT NULL,
    testcase_index INT NOT NULL,
    verdict ENUM('AC', 'WA', 'TLE', 'MLE', 'RTE', 'CE') NOT NULL,
    execution_time_ms INT NOT NULL DEFAULT 0,
    peak_memory_kb INT NOT NULL DEFAULT 0,
    score DOUBLE NOT NULL DEFAULT 0.0,
    error_message TEXT,
    CONSTRAINT fk_testcase_submission
        FOREIGN KEY (submission_id) REFERENCES submissions(id)
        ON DELETE CASCADE
) ENGINE=InnoDB;

-- Indexes for fast Leaderboard queries
CREATE INDEX idx_submission_contestant ON submissions(contestant_id);
CREATE INDEX idx_submission_problem ON submissions(problem_id);
CREATE INDEX idx_submission_verdict ON submissions(verdict);
CREATE INDEX idx_testcases_submission ON submission_testcases(submission_id);
