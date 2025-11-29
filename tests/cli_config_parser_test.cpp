// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <gtest/gtest.h>

#include <kcenon/common/config/cli_config_parser.h>

#include <cstdlib>
#include <vector>
#include <string>

using namespace kcenon::common::config;

// ============================================================================
// Helper class for managing argv arrays
// ============================================================================

class ArgvBuilder {
public:
    ArgvBuilder() = default;

    ArgvBuilder& add(const std::string& arg) {
        args_.push_back(arg);
        return *this;
    }

    int argc() const { return static_cast<int>(args_.size()); }

    char** argv() {
        ptrs_.clear();
        for (auto& arg : args_) {
            ptrs_.push_back(const_cast<char*>(arg.c_str()));
        }
        ptrs_.push_back(nullptr);
        return ptrs_.data();
    }

private:
    std::vector<std::string> args_;
    std::vector<char*> ptrs_;
};

// Helper class for environment variable management
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
// Parse Tests
// ============================================================================

TEST(CliConfigParserTest, Parse_NoArgs_ReturnsEmpty) {
    ArgvBuilder args;
    args.add("program");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    auto parsed = result.value();
    EXPECT_TRUE(parsed.config_path.empty());
    EXPECT_TRUE(parsed.overrides.empty());
    EXPECT_FALSE(parsed.show_help);
    EXPECT_FALSE(parsed.show_version);
}

TEST(CliConfigParserTest, Parse_HelpFlag_Long) {
    ArgvBuilder args;
    args.add("program").add("--help");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value().show_help);
}

TEST(CliConfigParserTest, Parse_HelpFlag_Short) {
    ArgvBuilder args;
    args.add("program").add("-h");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value().show_help);
}

TEST(CliConfigParserTest, Parse_VersionFlag_Long) {
    ArgvBuilder args;
    args.add("program").add("--version");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value().show_version);
}

TEST(CliConfigParserTest, Parse_VersionFlag_Short) {
    ArgvBuilder args;
    args.add("program").add("-v");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value().show_version);
}

TEST(CliConfigParserTest, Parse_ConfigFlag_Equals) {
    ArgvBuilder args;
    args.add("program").add("--config=path/to/config.yaml");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().config_path, "path/to/config.yaml");
}

TEST(CliConfigParserTest, Parse_ConfigFlag_Separate) {
    ArgvBuilder args;
    args.add("program").add("--config").add("path/to/config.yaml");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().config_path, "path/to/config.yaml");
}

TEST(CliConfigParserTest, Parse_ConfigFlag_MissingValue) {
    ArgvBuilder args;
    args.add("program").add("--config");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, cli_error_codes::missing_value);
}

TEST(CliConfigParserTest, Parse_SetFlag_Equals) {
    ArgvBuilder args;
    args.add("program").add("--set=logger.level=debug");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    auto& overrides = result.value().overrides;
    ASSERT_EQ(overrides.size(), 1);
    EXPECT_EQ(overrides[0].first, "logger.level");
    EXPECT_EQ(overrides[0].second, "debug");
}

TEST(CliConfigParserTest, Parse_SetFlag_Separate) {
    ArgvBuilder args;
    args.add("program").add("--set").add("logger.level=debug");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    auto& overrides = result.value().overrides;
    ASSERT_EQ(overrides.size(), 1);
    EXPECT_EQ(overrides[0].first, "logger.level");
    EXPECT_EQ(overrides[0].second, "debug");
}

TEST(CliConfigParserTest, Parse_SetFlag_MissingValue) {
    ArgvBuilder args;
    args.add("program").add("--set");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, cli_error_codes::missing_value);
}

TEST(CliConfigParserTest, Parse_SetFlag_InvalidFormat) {
    ArgvBuilder args;
    args.add("program").add("--set=no_equals_sign");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, cli_error_codes::invalid_format);
}

TEST(CliConfigParserTest, Parse_MultipleOverrides) {
    ArgvBuilder args;
    args.add("program")
        .add("--set=logger.level=debug")
        .add("--set").add("thread.pool_size=16")
        .add("--set=monitoring.enabled=false");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    auto& overrides = result.value().overrides;
    ASSERT_EQ(overrides.size(), 3);

    EXPECT_EQ(overrides[0].first, "logger.level");
    EXPECT_EQ(overrides[0].second, "debug");

    EXPECT_EQ(overrides[1].first, "thread.pool_size");
    EXPECT_EQ(overrides[1].second, "16");

    EXPECT_EQ(overrides[2].first, "monitoring.enabled");
    EXPECT_EQ(overrides[2].second, "false");
}

TEST(CliConfigParserTest, Parse_UnknownLongArg) {
    ArgvBuilder args;
    args.add("program").add("--unknown-arg");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, cli_error_codes::invalid_argument);
}

TEST(CliConfigParserTest, Parse_UnknownShortArg) {
    ArgvBuilder args;
    args.add("program").add("-x");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, cli_error_codes::invalid_argument);
}

TEST(CliConfigParserTest, Parse_PositionalArgs) {
    ArgvBuilder args;
    args.add("program").add("file1.txt").add("file2.txt");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    auto& positional = result.value().positional_args;
    ASSERT_EQ(positional.size(), 2);
    EXPECT_EQ(positional[0], "file1.txt");
    EXPECT_EQ(positional[1], "file2.txt");
}

TEST(CliConfigParserTest, Parse_MixedArgs) {
    ArgvBuilder args;
    args.add("program")
        .add("--config=config.yaml")
        .add("--set=logger.level=debug")
        .add("input.txt")
        .add("--set=thread.pool_size=8");

    auto result = cli_config_parser::parse(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    auto parsed = result.value();
    EXPECT_EQ(parsed.config_path, "config.yaml");
    EXPECT_EQ(parsed.overrides.size(), 2);
    EXPECT_EQ(parsed.positional_args.size(), 1);
    EXPECT_EQ(parsed.positional_args[0], "input.txt");
}

// ============================================================================
// Load With CLI Overrides Tests
// ============================================================================

TEST(CliConfigParserTest, LoadWithCliOverrides_NoArgs) {
    ArgvBuilder args;
    args.add("program");

    auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    // Should have default values
    auto config = result.value();
    EXPECT_EQ(config.thread.pool_size, 0);
    EXPECT_EQ(config.logger.level, "info");
}

TEST(CliConfigParserTest, LoadWithCliOverrides_AppliesOverrides) {
    ArgvBuilder args;
    args.add("program")
        .add("--set=logger.level=debug")
        .add("--set=thread.pool_size=16")
        .add("--set=monitoring.enabled=false");

    auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    auto config = result.value();
    EXPECT_EQ(config.logger.level, "debug");
    EXPECT_EQ(config.thread.pool_size, 16);
    EXPECT_FALSE(config.monitoring.enabled);
}

TEST(CliConfigParserTest, LoadWithCliOverrides_OverridesEnv) {
    // Set environment variable
    EnvVarGuard guard("UNIFIED_LOGGER_LEVEL", "warn");

    ArgvBuilder args;
    args.add("program").add("--set=logger.level=error");

    auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    // CLI override should take precedence over environment
    EXPECT_EQ(result.value().logger.level, "error");
}

TEST(CliConfigParserTest, LoadWithCliOverrides_InvalidKey) {
    ArgvBuilder args;
    args.add("program").add("--set=invalid.key.path=value");

    auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, cli_error_codes::invalid_key);
}

TEST(CliConfigParserTest, LoadWithCliOverrides_ValidationFails) {
    ArgvBuilder args;
    args.add("program").add("--set=logger.level=invalid_level");

    auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
    EXPECT_TRUE(result.is_err());
}

TEST(CliConfigParserTest, LoadWithCliOverrides_BooleanValues) {
    // Test various boolean formats
    {
        ArgvBuilder args;
        args.add("program").add("--set=logger.async=true");
        auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
        ASSERT_TRUE(result.is_ok());
        EXPECT_TRUE(result.value().logger.async);
    }
    {
        ArgvBuilder args;
        args.add("program").add("--set=logger.async=false");
        auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
        ASSERT_TRUE(result.is_ok());
        EXPECT_FALSE(result.value().logger.async);
    }
    {
        ArgvBuilder args;
        args.add("program").add("--set=logger.async=1");
        auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
        ASSERT_TRUE(result.is_ok());
        EXPECT_TRUE(result.value().logger.async);
    }
    {
        ArgvBuilder args;
        args.add("program").add("--set=logger.async=0");
        auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
        ASSERT_TRUE(result.is_ok());
        EXPECT_FALSE(result.value().logger.async);
    }
}

TEST(CliConfigParserTest, LoadWithCliOverrides_NumericValues) {
    ArgvBuilder args;
    args.add("program")
        .add("--set=thread.pool_size=32")
        .add("--set=network.buffer_size=131072")
        .add("--set=monitoring.tracing.sampling_rate=0.5");

    auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    auto config = result.value();
    EXPECT_EQ(config.thread.pool_size, 32);
    EXPECT_EQ(config.network.buffer_size, 131072);
    EXPECT_DOUBLE_EQ(config.monitoring.tracing.sampling_rate, 0.5);
}

TEST(CliConfigParserTest, LoadWithCliOverrides_DurationValues) {
    ArgvBuilder args;
    args.add("program")
        .add("--set=monitoring.metrics_interval_ms=10000")
        .add("--set=network.connect_timeout_ms=3000");

    auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    auto config = result.value();
    EXPECT_EQ(config.monitoring.metrics_interval.count(), 10000);
    EXPECT_EQ(config.network.connect_timeout.count(), 3000);
}

TEST(CliConfigParserTest, LoadWithCliOverrides_NestedKeys) {
    ArgvBuilder args;
    args.add("program")
        .add("--set=monitoring.tracing.enabled=true")
        .add("--set=monitoring.tracing.exporter=jaeger")
        .add("--set=database.pool.min_size=10")
        .add("--set=network.tls.version=1.2");

    auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    auto config = result.value();
    EXPECT_TRUE(config.monitoring.tracing.enabled);
    EXPECT_EQ(config.monitoring.tracing.exporter, "jaeger");
    EXPECT_EQ(config.database.pool.min_size, 10);
    EXPECT_EQ(config.network.tls.version, "1.2");
}

// ============================================================================
// Help and Version Tests
// ============================================================================

TEST(CliConfigParserTest, LoadWithCliOverrides_HelpReturnsError) {
    ArgvBuilder args;
    args.add("program").add("--help");

    auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().message, "help_requested");
}

TEST(CliConfigParserTest, LoadWithCliOverrides_VersionReturnsError) {
    ArgvBuilder args;
    args.add("program").add("--version");

    auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().message, "version_requested");
}

// ============================================================================
// Print Help Test (just verify it doesn't crash)
// ============================================================================

TEST(CliConfigParserTest, PrintHelp_DoesNotCrash) {
    // Redirect stdout to suppress output
    testing::internal::CaptureStdout();

    cli_config_parser::print_help("test_program");

    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(output.empty());
    EXPECT_NE(output.find("test_program"), std::string::npos);
    EXPECT_NE(output.find("--config"), std::string::npos);
    EXPECT_NE(output.find("--set"), std::string::npos);
    EXPECT_NE(output.find("--help"), std::string::npos);
}

TEST(CliConfigParserTest, PrintVersion_DoesNotCrash) {
    testing::internal::CaptureStdout();

    cli_config_parser::print_version("2.0.0");

    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(output.empty());
    EXPECT_NE(output.find("2.0.0"), std::string::npos);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(CliConfigParserTest, FullWorkflow) {
    ArgvBuilder args;
    args.add("myapp")
        .add("--set=thread.pool_size=8")
        .add("--set=logger.level=warn")
        .add("--set=database.backend=postgresql")
        .add("--set=database.connection_string=postgresql://localhost/mydb")
        .add("--set=network.compression=zstd");

    auto result = cli_config_parser::load_with_cli_overrides(args.argc(), args.argv());
    ASSERT_TRUE(result.is_ok());

    auto config = result.value();
    EXPECT_EQ(config.thread.pool_size, 8);
    EXPECT_EQ(config.logger.level, "warn");
    EXPECT_EQ(config.database.backend, "postgresql");
    EXPECT_EQ(config.database.connection_string, "postgresql://localhost/mydb");
    EXPECT_EQ(config.network.compression, "zstd");
}
