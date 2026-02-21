/**
 * @file database_interface_test.cpp
 * @brief Unit tests for database interface types and IDatabase contract.
 *
 * Tests the database type aliases and mock IDatabase implementation:
 * - database_value variant construction with all 5 types
 * - database_row map construction and key lookup
 * - database_result multi-row iteration
 * - database_null comparison semantics
 * - Mock IDatabase interface contract
 *
 * @date 2026-02-21
 */

#include <gtest/gtest.h>
#include <kcenon/common/interfaces/database_interface.h>

using namespace kcenon::common;
using namespace kcenon::common::interfaces;

// ============================================================================
// Mock IDatabase implementation for interface contract testing
// ============================================================================

class MockDatabase : public IDatabase {
public:
    VoidResult connect(const std::string& connection_string) override
    {
        connection_string_ = connection_string;
        connected_ = true;
        return VoidResult::ok({});
    }

    VoidResult disconnect() override
    {
        connected_ = false;
        return VoidResult::ok({});
    }

    Result<database_result> execute_query(const std::string& query) override
    {
        last_query_ = query;
        return Result<database_result>::ok(query_result_);
    }

    VoidResult execute_command(const std::string& command) override
    {
        last_command_ = command;
        return VoidResult::ok({});
    }

    VoidResult begin_transaction() override
    {
        in_transaction_ = true;
        return VoidResult::ok({});
    }

    VoidResult commit() override
    {
        in_transaction_ = false;
        return VoidResult::ok({});
    }

    VoidResult rollback() override
    {
        in_transaction_ = false;
        return VoidResult::ok({});
    }

    bool is_connected() const override
    {
        return connected_;
    }

    // Test helpers
    void set_query_result(database_result result) { query_result_ = std::move(result); }
    std::string last_query() const { return last_query_; }
    std::string last_command() const { return last_command_; }
    bool in_transaction() const { return in_transaction_; }

private:
    bool connected_ = false;
    bool in_transaction_ = false;
    std::string connection_string_;
    std::string last_query_;
    std::string last_command_;
    database_result query_result_;
};

// ============================================================================
// database_null tests
// ============================================================================

TEST(DatabaseInterfaceTest, DatabaseNullDefaultConstruction)
{
    database_null null1;
    database_null null2;
    // database_null is an empty struct - verify it can be constructed
    (void)null1;
    (void)null2;
}

// ============================================================================
// database_value variant tests
// ============================================================================

TEST(DatabaseInterfaceTest, DatabaseValueNullType)
{
    database_value val{database_null{}};
    EXPECT_TRUE(std::holds_alternative<database_null>(val));
    EXPECT_EQ(val.index(), 0);
}

TEST(DatabaseInterfaceTest, DatabaseValueStringType)
{
    database_value val{std::string("hello")};
    EXPECT_TRUE(std::holds_alternative<std::string>(val));
    EXPECT_EQ(std::get<std::string>(val), "hello");
}

TEST(DatabaseInterfaceTest, DatabaseValueInt64Type)
{
    database_value val{static_cast<std::int64_t>(42)};
    EXPECT_TRUE(std::holds_alternative<std::int64_t>(val));
    EXPECT_EQ(std::get<std::int64_t>(val), 42);
}

TEST(DatabaseInterfaceTest, DatabaseValueDoubleType)
{
    database_value val{3.14};
    EXPECT_TRUE(std::holds_alternative<double>(val));
    EXPECT_DOUBLE_EQ(std::get<double>(val), 3.14);
}

TEST(DatabaseInterfaceTest, DatabaseValueBoolType)
{
    database_value val_true{true};
    database_value val_false{false};
    EXPECT_TRUE(std::holds_alternative<bool>(val_true));
    EXPECT_TRUE(std::get<bool>(val_true));
    EXPECT_FALSE(std::get<bool>(val_false));
}

TEST(DatabaseInterfaceTest, DatabaseValueReassignment)
{
    database_value val{std::string("initial")};
    EXPECT_TRUE(std::holds_alternative<std::string>(val));

    val = static_cast<std::int64_t>(100);
    EXPECT_TRUE(std::holds_alternative<std::int64_t>(val));
    EXPECT_EQ(std::get<std::int64_t>(val), 100);
}

// ============================================================================
// database_row tests
// ============================================================================

TEST(DatabaseInterfaceTest, DatabaseRowConstruction)
{
    database_row row;
    row["name"] = database_value{std::string("Alice")};
    row["age"] = database_value{static_cast<std::int64_t>(30)};
    row["score"] = database_value{95.5};
    row["active"] = database_value{true};
    row["nickname"] = database_value{database_null{}};

    EXPECT_EQ(row.size(), 5u);
}

TEST(DatabaseInterfaceTest, DatabaseRowKeyLookup)
{
    database_row row;
    row["id"] = database_value{static_cast<std::int64_t>(1)};
    row["email"] = database_value{std::string("test@example.com")};

    EXPECT_EQ(std::get<std::int64_t>(row["id"]), 1);
    EXPECT_EQ(std::get<std::string>(row["email"]), "test@example.com");
}

TEST(DatabaseInterfaceTest, DatabaseRowMissingKey)
{
    database_row row;
    row["name"] = database_value{std::string("Bob")};

    // Accessing non-existent key via find
    auto it = row.find("missing");
    EXPECT_EQ(it, row.end());
}

// ============================================================================
// database_result tests
// ============================================================================

TEST(DatabaseInterfaceTest, DatabaseResultMultiRowIteration)
{
    database_result result;

    database_row row1;
    row1["id"] = database_value{static_cast<std::int64_t>(1)};
    row1["name"] = database_value{std::string("Alice")};

    database_row row2;
    row2["id"] = database_value{static_cast<std::int64_t>(2)};
    row2["name"] = database_value{std::string("Bob")};

    result.push_back(row1);
    result.push_back(row2);

    EXPECT_EQ(result.size(), 2u);

    // Iterate and verify
    std::vector<std::string> names;
    for (const auto& row : result) {
        names.push_back(std::get<std::string>(row.at("name")));
    }
    EXPECT_EQ(names[0], "Alice");
    EXPECT_EQ(names[1], "Bob");
}

TEST(DatabaseInterfaceTest, DatabaseResultEmpty)
{
    database_result result;
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.size(), 0u);
}

// ============================================================================
// Mock IDatabase interface contract tests
// ============================================================================

TEST(DatabaseInterfaceTest, MockConnectAndDisconnect)
{
    MockDatabase db;
    EXPECT_FALSE(db.is_connected());

    auto connect_result = db.connect("host=localhost dbname=test");
    EXPECT_TRUE(connect_result.is_ok());
    EXPECT_TRUE(db.is_connected());

    auto disconnect_result = db.disconnect();
    EXPECT_TRUE(disconnect_result.is_ok());
    EXPECT_FALSE(db.is_connected());
}

TEST(DatabaseInterfaceTest, MockExecuteQuery)
{
    MockDatabase db;
    db.connect("host=localhost");

    // Set up expected result
    database_result expected;
    database_row row;
    row["count"] = database_value{static_cast<std::int64_t>(42)};
    expected.push_back(row);
    db.set_query_result(expected);

    auto result = db.execute_query("SELECT count(*) FROM users");
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(result.value()[0].at("count")), 42);
    EXPECT_EQ(db.last_query(), "SELECT count(*) FROM users");
}

TEST(DatabaseInterfaceTest, MockExecuteCommand)
{
    MockDatabase db;
    db.connect("host=localhost");

    auto result = db.execute_command("INSERT INTO users (name) VALUES ('test')");
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(db.last_command(), "INSERT INTO users (name) VALUES ('test')");
}

TEST(DatabaseInterfaceTest, MockTransactionLifecycle)
{
    MockDatabase db;
    db.connect("host=localhost");

    EXPECT_FALSE(db.in_transaction());

    auto begin_result = db.begin_transaction();
    EXPECT_TRUE(begin_result.is_ok());
    EXPECT_TRUE(db.in_transaction());

    auto commit_result = db.commit();
    EXPECT_TRUE(commit_result.is_ok());
    EXPECT_FALSE(db.in_transaction());
}

TEST(DatabaseInterfaceTest, MockTransactionRollback)
{
    MockDatabase db;
    db.connect("host=localhost");

    db.begin_transaction();
    EXPECT_TRUE(db.in_transaction());

    auto rollback_result = db.rollback();
    EXPECT_TRUE(rollback_result.is_ok());
    EXPECT_FALSE(db.in_transaction());
}

TEST(DatabaseInterfaceTest, PolymorphicAccess)
{
    auto db = std::make_unique<MockDatabase>();
    IDatabase* base_ptr = db.get();

    auto result = base_ptr->connect("host=localhost");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(base_ptr->is_connected());
    base_ptr->disconnect();
}
