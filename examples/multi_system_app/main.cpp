// examples/multi_system_app/main.cpp
// Multi-System Integration Example
//
// Demonstrates integration of 4 kcenon systems:
// - common_system: Foundation (Result<T>, interfaces)
// - thread_system: Async processing
// - logger_system: Structured logging
// - database_system: Data persistence

#include <kcenon/common/di/unified_bootstrapper.h>
#include <kcenon/common/config/config_reader.h>
#include <kcenon/database/core/database.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/logger/core/logger.h>

#include <iostream>
#include <csignal>
#include <atomic>

using namespace kcenon;

// Application state
std::atomic<bool> g_running{true};

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down...\n";
    g_running = false;
}

// User data model
struct User {
    int id;
    std::string name;
    std::string email;
};

// Business logic: Process user registration
auto register_user(
    std::shared_ptr<database::database> db,
    std::shared_ptr<common::interfaces::ILogger> logger,
    const std::string& name,
    const std::string& email
) -> common::Result<User> {
    // Validate input
    if (name.empty() || email.empty()) {
        return common::make_error<User>(
            common::error_codes::INVALID_ARGUMENT,
            "Name and email are required",
            "register_user"
        );
    }

    logger->log(common::log_level::info,
               "Registering user: " + name + " <" + email + ">");

    // Check if user already exists
    auto check_query = "SELECT COUNT(*) FROM users WHERE email = '" + email + "'";
    auto check_result = db->query(check_query);

    if (check_result.is_err()) {
        return common::make_error<User>(check_result.error());
    }

    if (check_result.value().rows[0].get<int>(0) > 0) {
        return common::make_error<User>(
            common::error_codes::ALREADY_EXISTS,
            "User with email already exists",
            "register_user"
        );
    }

    // Insert new user
    auto insert_query = "INSERT INTO users (name, email) VALUES ('"
                       + name + "', '" + email + "')";
    auto insert_result = db->execute(insert_query);

    if (insert_result.is_err()) {
        logger->log(common::log_level::error,
                   "Failed to insert user: " + insert_result.error().message);
        return common::make_error<User>(insert_result.error());
    }

    // Retrieve inserted user
    auto user_id = db->last_insert_id();

    logger->log(common::log_level::info,
               "User registered successfully with ID: " + std::to_string(user_id));

    return common::ok(User{
        .id = static_cast<int>(user_id),
        .name = name,
        .email = email
    });
}

int main(int argc, char** argv) {
    // Install signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "=== Multi-System Application Demo ===\n\n";

    // Step 1: Load Configuration
    std::cout << "1. Loading configuration...\n";
    auto config_result = common::config::read_yaml("app.yaml");
    if (config_result.is_err()) {
        std::cerr << "Config error: " << config_result.error().message << "\n";
        std::cerr << "Using default configuration\n";
    }
    auto cfg = config_result.is_ok() ? config_result.value()
                                      : common::config::Config::defaults();

    // Step 2: Initialize Systems
    std::cout << "2. Initializing systems...\n";
    common::di::bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_database = true;
    opts.config_path = "app.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        std::cerr << "Initialization failed: "
                  << init_result.error().message << "\n";
        return 1;
    }

    std::cout << "   ✓ Systems initialized successfully\n\n";

    // Step 3: Resolve Services
    auto& services = common::di::unified_bootstrapper::services();
    auto logger = services.resolve<common::interfaces::ILogger>();
    auto executor = services.resolve<common::interfaces::IExecutor>();

    // Step 4: Setup Database
    std::cout << "3. Setting up database...\n";
    database::connection_config db_config;
    db_config.type = database::db_type::sqlite;
    db_config.path = "users.db";

    auto db_result = database::database::connect(db_config);
    if (db_result.is_err()) {
        std::cerr << "Database connection failed: "
                  << db_result.error().message << "\n";
        common::di::unified_bootstrapper::shutdown();
        return 1;
    }
    auto db = db_result.value();

    // Create users table
    auto create_table_result = db->execute(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  email TEXT UNIQUE NOT NULL"
        ")"
    );

    if (create_table_result.is_err()) {
        std::cerr << "Table creation failed\n";
        common::di::unified_bootstrapper::shutdown();
        return 1;
    }

    std::cout << "   ✓ Database ready\n\n";

    // Step 5: Application Logic
    std::cout << "4. Running application...\n";
    logger->log(common::log_level::info, "Application started");

    // Register sample users
    std::vector<std::pair<std::string, std::string>> sample_users = {
        {"Alice Smith", "alice@example.com"},
        {"Bob Johnson", "bob@example.com"},
        {"Carol Williams", "carol@example.com"}
    };

    for (const auto& [name, email] : sample_users) {
        auto result = register_user(db, logger, name, email);

        if (result.is_ok()) {
            const auto& user = result.value();
            std::cout << "   ✓ Registered: " << user.name
                      << " (ID: " << user.id << ")\n";
        } else {
            std::cout << "   ✗ Failed: " << name
                      << " - " << result.error().message << "\n";
        }

        // Simulate some processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (!g_running) {
            break;
        }
    }

    // Query all users
    std::cout << "\n5. Querying all users...\n";
    auto users_result = db->query("SELECT id, name, email FROM users");

    if (users_result.is_ok()) {
        const auto& rows = users_result.value().rows;
        std::cout << "   Total users: " << rows.size() << "\n";

        for (const auto& row : rows) {
            std::cout << "   - " << row.get<std::string>(1)
                      << " <" << row.get<std::string>(2) << ">\n";
        }
    }

    logger->log(common::log_level::info, "Application completed");

    // Step 6: Graceful Shutdown
    std::cout << "\n6. Shutting down...\n";
    auto shutdown_result = common::di::unified_bootstrapper::shutdown(
        std::chrono::seconds(5)
    );

    if (shutdown_result.is_err()) {
        std::cerr << "Shutdown error: "
                  << shutdown_result.error().message << "\n";
        return 1;
    }

    std::cout << "   ✓ Shutdown complete\n";
    std::cout << "\n=== Application Finished ===\n";

    return 0;
}
