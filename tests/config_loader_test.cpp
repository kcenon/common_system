// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <gtest/gtest.h>

#include <kcenon/common/config/config_loader.h>

#include <cstdlib>
#include <fstream>
#include <filesystem>

using namespace kcenon::common::config;

// ============================================================================
// Helper class for environment variable management
// ============================================================================

class EnvVarGuard {
public:
    EnvVarGuard(const std::string& name, const std::string& value)
        : name_(name) {
        const char* old = std::getenv(name.c_str());
        if (old) {
            had_value_ = true;
            old_value_ = old;
        }
        setenv(name.c_str(), value.c_str(), 1);
    }

    ~EnvVarGuard() {
        if (had_value_) {
            setenv(name_.c_str(), old_value_.c_str(), 1);
        } else {
            unsetenv(name_.c_str());
        }
    }

private:
    std::string name_;
    std::string old_value_;
    bool had_value_ = false;
};

// ============================================================================
// Defaults Tests
// ============================================================================

TEST(ConfigLoaderTest, Defaults_ReturnsDefaultConfig) {
    auto config = config_loader::defaults();

    EXPECT_EQ(config.thread.pool_size, 0);
    EXPECT_EQ(config.thread.queue_type, "lockfree");
    EXPECT_EQ(config.logger.level, "info");
    EXPECT_TRUE(config.monitoring.enabled);
    EXPECT_FALSE(config.monitoring.tracing.enabled);
}

// ============================================================================
// Environment Variable Expansion Tests
// ============================================================================

TEST(ConfigLoaderTest, ExpandEnvVars_NoVariables) {
    std::string input = "Hello, World!";
    std::string result = config_loader::expand_env_vars(input);
    EXPECT_EQ(result, "Hello, World!");
}

TEST(ConfigLoaderTest, ExpandEnvVars_SingleVariable) {
    EnvVarGuard guard("TEST_VAR_SINGLE", "expanded_value");

    std::string input = "prefix_${TEST_VAR_SINGLE}_suffix";
    std::string result = config_loader::expand_env_vars(input);
    EXPECT_EQ(result, "prefix_expanded_value_suffix");
}

TEST(ConfigLoaderTest, ExpandEnvVars_MultipleVariables) {
    EnvVarGuard guard1("TEST_VAR_A", "valueA");
    EnvVarGuard guard2("TEST_VAR_B", "valueB");

    std::string input = "${TEST_VAR_A} and ${TEST_VAR_B}";
    std::string result = config_loader::expand_env_vars(input);
    EXPECT_EQ(result, "valueA and valueB");
}

TEST(ConfigLoaderTest, ExpandEnvVars_UndefinedVariable_KeepsOriginal) {
    std::string input = "Value is ${UNDEFINED_TEST_VAR_12345}";
    std::string result = config_loader::expand_env_vars(input);
    EXPECT_EQ(result, "Value is ${UNDEFINED_TEST_VAR_12345}");
}

TEST(ConfigLoaderTest, ExpandEnvVars_EmptyInput) {
    std::string input = "";
    std::string result = config_loader::expand_env_vars(input);
    EXPECT_EQ(result, "");
}

// ============================================================================
// Environment Override Tests
// ============================================================================

TEST(ConfigLoaderTest, LoadFromEnv_AppliesOverrides) {
    EnvVarGuard guard1("UNIFIED_THREAD_POOL_SIZE", "32");
    EnvVarGuard guard2("UNIFIED_LOGGER_LEVEL", "debug");
    EnvVarGuard guard3("UNIFIED_MONITORING_ENABLED", "false");

    auto result = config_loader::load_from_env();
    ASSERT_TRUE(result.is_ok());

    auto config = result.value();
    EXPECT_EQ(config.thread.pool_size, 32);
    EXPECT_EQ(config.logger.level, "debug");
    EXPECT_FALSE(config.monitoring.enabled);
}

TEST(ConfigLoaderTest, LoadFromEnv_BooleanValues) {
    // Test various boolean formats
    {
        EnvVarGuard guard("UNIFIED_LOGGER_ASYNC", "true");
        auto result = config_loader::load_from_env();
        ASSERT_TRUE(result.is_ok());
        EXPECT_TRUE(result.value().logger.async);
    }
    {
        EnvVarGuard guard("UNIFIED_LOGGER_ASYNC", "1");
        auto result = config_loader::load_from_env();
        ASSERT_TRUE(result.is_ok());
        EXPECT_TRUE(result.value().logger.async);
    }
    {
        EnvVarGuard guard("UNIFIED_LOGGER_ASYNC", "yes");
        auto result = config_loader::load_from_env();
        ASSERT_TRUE(result.is_ok());
        EXPECT_TRUE(result.value().logger.async);
    }
    {
        EnvVarGuard guard("UNIFIED_LOGGER_ASYNC", "false");
        auto result = config_loader::load_from_env();
        ASSERT_TRUE(result.is_ok());
        EXPECT_FALSE(result.value().logger.async);
    }
    {
        EnvVarGuard guard("UNIFIED_LOGGER_ASYNC", "0");
        auto result = config_loader::load_from_env();
        ASSERT_TRUE(result.is_ok());
        EXPECT_FALSE(result.value().logger.async);
    }
    {
        EnvVarGuard guard("UNIFIED_LOGGER_ASYNC", "no");
        auto result = config_loader::load_from_env();
        ASSERT_TRUE(result.is_ok());
        EXPECT_FALSE(result.value().logger.async);
    }
}

TEST(ConfigLoaderTest, LoadFromEnv_MillisecondValues) {
    EnvVarGuard guard("UNIFIED_MONITORING_METRICS_INTERVAL_MS", "10000");

    auto result = config_loader::load_from_env();
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().monitoring.metrics_interval.count(), 10000);
}

TEST(ConfigLoaderTest, LoadFromEnv_DoubleValues) {
    EnvVarGuard guard("UNIFIED_MONITORING_TRACING_SAMPLING_RATE", "0.5");

    auto result = config_loader::load_from_env();
    ASSERT_TRUE(result.is_ok());
    EXPECT_DOUBLE_EQ(result.value().monitoring.tracing.sampling_rate, 0.5);
}

TEST(ConfigLoaderTest, LoadFromEnv_VectorValues) {
    EnvVarGuard guard("UNIFIED_LOGGER_WRITERS", "console, file, json");

    auto result = config_loader::load_from_env();
    ASSERT_TRUE(result.is_ok());

    auto& writers = result.value().logger.writers;
    EXPECT_EQ(writers.size(), 3);
    EXPECT_EQ(writers[0], "console");
    EXPECT_EQ(writers[1], "file");
    EXPECT_EQ(writers[2], "json");
}

TEST(ConfigLoaderTest, LoadFromEnv_InvalidNumericValue_Ignored) {
    EnvVarGuard guard("UNIFIED_THREAD_POOL_SIZE", "not_a_number");

    auto result = config_loader::load_from_env();
    ASSERT_TRUE(result.is_ok());
    // Should keep default value
    EXPECT_EQ(result.value().thread.pool_size, 0);
}

// ============================================================================
// Validation Tests
// ============================================================================

TEST(ConfigLoaderTest, Validate_ValidConfig_Succeeds) {
    auto config = config_loader::defaults();
    auto result = config_loader::validate(config);
    EXPECT_TRUE(result.is_ok());
}

TEST(ConfigLoaderTest, Validate_InvalidQueueType_Fails) {
    auto config = config_loader::defaults();
    config.thread.queue_type = "invalid_type";

    auto result = config_loader::validate(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_NE(result.error().message.find("thread.queue_type"), std::string::npos);
}

TEST(ConfigLoaderTest, Validate_InvalidLogLevel_Fails) {
    auto config = config_loader::defaults();
    config.logger.level = "invalid_level";

    auto result = config_loader::validate(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_NE(result.error().message.find("logger.level"), std::string::npos);
}

TEST(ConfigLoaderTest, Validate_InvalidSamplingRate_Fails) {
    auto config = config_loader::defaults();
    config.monitoring.tracing.sampling_rate = 1.5;  // > 1.0

    auto result = config_loader::validate(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_NE(result.error().message.find("sampling_rate"), std::string::npos);
}

TEST(ConfigLoaderTest, Validate_InvalidPoolSize_Fails) {
    auto config = config_loader::defaults();
    config.database.pool.min_size = 50;
    config.database.pool.max_size = 10;  // min > max

    auto result = config_loader::validate(config);
    EXPECT_TRUE(result.is_err());
}

TEST(ConfigLoaderTest, Validate_ZeroMaxPoolSize_Fails) {
    auto config = config_loader::defaults();
    config.database.pool.max_size = 0;

    auto result = config_loader::validate(config);
    EXPECT_TRUE(result.is_err());
}

TEST(ConfigLoaderTest, Validate_InvalidTlsVersion_Fails) {
    auto config = config_loader::defaults();
    config.network.tls.version = "1.0";  // Not supported

    auto result = config_loader::validate(config);
    EXPECT_TRUE(result.is_err());
}

TEST(ConfigLoaderTest, Validate_InvalidCompression_Fails) {
    auto config = config_loader::defaults();
    config.network.compression = "invalid";

    auto result = config_loader::validate(config);
    EXPECT_TRUE(result.is_err());
}

// ============================================================================
// Validation Issues Tests
// ============================================================================

TEST(ConfigLoaderTest, GetValidationIssues_ReturnsWarnings) {
    auto config = config_loader::defaults();
    config.logger.buffer_size = 512;  // Very small for async
    config.logger.async = true;

    auto issues = config_loader::get_validation_issues(config);

    // Should have a warning about buffer size
    bool found_warning = false;
    for (const auto& issue : issues) {
        if (issue.field_path == "logger.buffer_size" && issue.is_warning) {
            found_warning = true;
            break;
        }
    }
    EXPECT_TRUE(found_warning);
}

TEST(ConfigLoaderTest, GetValidationIssues_SmallMetricsInterval_Warning) {
    auto config = config_loader::defaults();
    config.monitoring.metrics_interval = std::chrono::milliseconds{100};  // Very short

    auto issues = config_loader::get_validation_issues(config);

    bool found_warning = false;
    for (const auto& issue : issues) {
        if (issue.field_path == "monitoring.metrics_interval" && issue.is_warning) {
            found_warning = true;
            break;
        }
    }
    EXPECT_TRUE(found_warning);
}

// ============================================================================
// File Loading Tests (without yaml-cpp)
// ============================================================================

#ifndef BUILD_WITH_YAML_CPP
TEST(ConfigLoaderTest, Load_WithoutYamlCpp_ReturnsError) {
    auto result = config_loader::load("nonexistent.yaml");
    EXPECT_TRUE(result.is_err());
    EXPECT_NE(result.error().message.find("YAML support not available"), std::string::npos);
}

TEST(ConfigLoaderTest, LoadFromString_WithoutYamlCpp_ReturnsError) {
    auto result = config_loader::load_from_string("key: value");
    EXPECT_TRUE(result.is_err());
    EXPECT_NE(result.error().message.find("YAML support not available"), std::string::npos);
}
#endif

// ============================================================================
// File Loading Tests (with yaml-cpp)
// ============================================================================

#ifdef BUILD_WITH_YAML_CPP

class TempFileGuard {
public:
    TempFileGuard(const std::string& content)
        : path_(std::filesystem::temp_directory_path() / ("config_test_" + std::to_string(std::rand()) + ".yaml")) {
        std::ofstream file(path_);
        file << content;
    }

    ~TempFileGuard() {
        std::filesystem::remove(path_);
    }

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

TEST(ConfigLoaderTest, Load_ValidYaml_Succeeds) {
    TempFileGuard file(R"(
thread:
  pool_size: 16
  queue_type: bounded

logger:
  level: debug
  writers:
    - console
    - file
)");

    auto result = config_loader::load(file.path().string());
    ASSERT_TRUE(result.is_ok());

    auto config = result.value();
    EXPECT_EQ(config.thread.pool_size, 16);
    EXPECT_EQ(config.thread.queue_type, "bounded");
    EXPECT_EQ(config.logger.level, "debug");
    EXPECT_EQ(config.logger.writers.size(), 2);
}

TEST(ConfigLoaderTest, Load_FileNotFound_ReturnsError) {
    auto result = config_loader::load("/nonexistent/path/config.yaml");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, config_error_codes::file_not_found);
}

TEST(ConfigLoaderTest, Load_InvalidYaml_ReturnsError) {
    TempFileGuard file("invalid: yaml: content: [unclosed");

    auto result = config_loader::load(file.path().string());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, config_error_codes::parse_error);
}

TEST(ConfigLoaderTest, Load_WithUnifiedSystemKey_Succeeds) {
    TempFileGuard file(R"(
unified_system:
  thread:
    pool_size: 8
  logger:
    level: warn
)");

    auto result = config_loader::load(file.path().string());
    ASSERT_TRUE(result.is_ok());

    EXPECT_EQ(result.value().thread.pool_size, 8);
    EXPECT_EQ(result.value().logger.level, "warn");
}

TEST(ConfigLoaderTest, Load_NestedConfig_Succeeds) {
    TempFileGuard file(R"(
monitoring:
  enabled: true
  metrics_interval_ms: 10000
  tracing:
    enabled: true
    sampling_rate: 0.5
    exporter: jaeger
    endpoint: http://localhost:14268

database:
  backend: postgresql
  connection_string: postgresql://localhost:5432/test
  pool:
    min_size: 10
    max_size: 50
)");

    auto result = config_loader::load(file.path().string());
    ASSERT_TRUE(result.is_ok());

    auto config = result.value();
    EXPECT_TRUE(config.monitoring.enabled);
    EXPECT_EQ(config.monitoring.metrics_interval.count(), 10000);
    EXPECT_TRUE(config.monitoring.tracing.enabled);
    EXPECT_DOUBLE_EQ(config.monitoring.tracing.sampling_rate, 0.5);
    EXPECT_EQ(config.monitoring.tracing.exporter, "jaeger");

    EXPECT_EQ(config.database.backend, "postgresql");
    EXPECT_EQ(config.database.pool.min_size, 10);
    EXPECT_EQ(config.database.pool.max_size, 50);
}

TEST(ConfigLoaderTest, Load_EnvVarSubstitution_Succeeds) {
    EnvVarGuard guard("TEST_DB_HOST", "db.example.com");

    TempFileGuard file(R"(
database:
  connection_string: postgresql://${TEST_DB_HOST}:5432/test
)");

    auto result = config_loader::load(file.path().string());
    ASSERT_TRUE(result.is_ok());

    EXPECT_EQ(result.value().database.connection_string, "postgresql://db.example.com:5432/test");
}

TEST(ConfigLoaderTest, Load_EnvOverrideTakesPrecedence) {
    EnvVarGuard guard("UNIFIED_THREAD_POOL_SIZE", "64");

    TempFileGuard file(R"(
thread:
  pool_size: 16
)");

    auto result = config_loader::load(file.path().string());
    ASSERT_TRUE(result.is_ok());

    // Environment override should take precedence over file
    EXPECT_EQ(result.value().thread.pool_size, 64);
}

TEST(ConfigLoaderTest, LoadFromString_ValidYaml_Succeeds) {
    std::string yaml = R"(
thread:
  pool_size: 24
logger:
  level: trace
)";

    auto result = config_loader::load_from_string(yaml);
    ASSERT_TRUE(result.is_ok());

    EXPECT_EQ(result.value().thread.pool_size, 24);
    EXPECT_EQ(result.value().logger.level, "trace");
}

TEST(ConfigLoaderTest, Load_TlsConfig_Succeeds) {
    TempFileGuard file(R"(
network:
  tls:
    enabled: true
    version: "1.3"
    cert_path: /etc/ssl/cert.pem
    key_path: /etc/ssl/key.pem
    ca_path: /etc/ssl/ca.pem
    verify_peer: true
  compression: zstd
  buffer_size: 131072
)");

    auto result = config_loader::load(file.path().string());
    ASSERT_TRUE(result.is_ok());

    auto config = result.value();
    EXPECT_TRUE(config.network.tls.enabled);
    EXPECT_EQ(config.network.tls.version, "1.3");
    EXPECT_EQ(config.network.tls.cert_path, "/etc/ssl/cert.pem");
    EXPECT_EQ(config.network.tls.key_path, "/etc/ssl/key.pem");
    EXPECT_EQ(config.network.compression, "zstd");
    EXPECT_EQ(config.network.buffer_size, 131072);
}

#endif  // BUILD_WITH_YAML_CPP

// ============================================================================
// Integration Tests
// ============================================================================

TEST(ConfigLoaderTest, FullWorkflow_EnvOnly) {
    // Set up environment
    EnvVarGuard guard1("UNIFIED_THREAD_POOL_SIZE", "8");
    EnvVarGuard guard2("UNIFIED_LOGGER_LEVEL", "warn");
    EnvVarGuard guard3("UNIFIED_DATABASE_BACKEND", "postgresql");
    EnvVarGuard guard4("UNIFIED_DATABASE_CONNECTION_STRING", "postgresql://localhost/mydb");
    EnvVarGuard guard5("UNIFIED_NETWORK_COMPRESSION", "zstd");

    // Load configuration
    auto result = config_loader::load_from_env();
    ASSERT_TRUE(result.is_ok());

    // Validate configuration
    auto validate_result = config_loader::validate(result.value());
    EXPECT_TRUE(validate_result.is_ok());

    // Check values
    auto config = result.value();
    EXPECT_EQ(config.thread.pool_size, 8);
    EXPECT_EQ(config.logger.level, "warn");
    EXPECT_EQ(config.database.backend, "postgresql");
    EXPECT_EQ(config.database.connection_string, "postgresql://localhost/mydb");
    EXPECT_EQ(config.network.compression, "zstd");
}
