/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/GenericLexer.h>
#include <LibCore/Directory.h>
#include <LibCore/Process.h>
#include <LibCore/System.h>
#include <LibDiff/Format.h>
#include <LibDiff/Generator.h>
#include <LibFileSystem/FileSystem.h>
#include <LibFileSystem/TempFile.h>
#include <LibTest/TestCase.h>

static auto const path_to_compiler_binary = [] {
    auto path_to_self = LexicalPath(MUST(Core::System::current_executable_path())).parent();
    return LexicalPath::join(path_to_self.string(), "ClangPlugin"sv);
}();

static ByteString extract_expected_error(ByteBuffer const& buf)
{
    GenericLexer lexer { buf.bytes() };
    ByteBuffer error;

    while (lexer.consume_specific("/// "sv)) {
        error.append(lexer.consume_until('\n').bytes());
        error.append('\n');
        lexer.ignore();
    }

    if (error.is_empty())
        return {};

    return ByteString { error.bytes() };
}

ErrorOr<ByteBuffer> read(ByteString const& path)
{
    auto file = TRY(Core::File::open(path, Core::File::OpenMode::Read));
    return MUST(file->read_until_eof());
}

void find_tests(StringView path_to_search)
{
    auto captured_stderr_file = MUST(FileSystem::TempFile::create_temp_file());
    auto path_to_captured_stderr = captured_stderr_file->path().to_byte_string();

    MUST(Core::Directory::for_each_entry(path_to_search, Core::DirIterator::Flags::SkipParentAndBaseDir, [&](auto const& entry, auto const& directory) -> ErrorOr<IterationDecision> {
        auto path = LexicalPath::join(directory.path().string(), entry.name);
        auto absolute_path = MUST(FileSystem::absolute_path(path.string()));

        if (entry.type == Core::DirectoryEntry::Type::Directory) {
            find_tests(absolute_path);
            return IterationDecision::Continue;
        }

        Test::add_test_case_to_suite(adopt_ref(*new Test::TestCase {
            ByteString::formatted("clang_plugin_test_{}", entry.name),
            [=] {
                auto file = TRY_OR_FAIL(Core::File::open(absolute_path, Core::File::OpenMode::Read));
                auto content = TRY_OR_FAIL(file->read_until_eof());
                auto expected_error = extract_expected_error(content);

                Vector<ByteString> arguments {
                    "--test-mode",
                    ByteString(absolute_path),
                };

                auto process = TRY_OR_FAIL(Core::Process::spawn({
                    .executable = path_to_compiler_binary.string(),
                    .arguments = move(arguments),
                    .file_actions = {
                        Core::FileAction::OpenFile {
                            .path = path_to_captured_stderr,
                            .mode = Core::File::OpenMode::Write,
                            .fd = STDERR_FILENO,
                        },
                    },
                }));

                bool exited_with_code_0 = TRY_OR_FAIL(process.wait_for_termination());
                auto captured_stderr = [&]() -> ByteString {
                    auto captured_output = read(path_to_captured_stderr);
                    if (captured_output.is_error())
                        return {};
                    auto buffer = captured_output.release_value();
                    if (buffer.is_empty())
                        return {};
                    return ByteString { buffer.bytes() };
                }();

                auto has_stderr_if_failed = captured_stderr.is_empty() == exited_with_code_0;
                EXPECT(has_stderr_if_failed);
                if (!has_stderr_if_failed)
                    return;

                auto stderr_matches_expected_output = captured_stderr == expected_error;
                EXPECT(stderr_matches_expected_output);
                if (!stderr_matches_expected_output) {
                    dbgln("Error emitted differs from expected error:");
                    auto maybe_diff = Diff::from_text(expected_error, captured_stderr);
                    if (!maybe_diff.is_error()) {
                        auto out = TRY_OR_FAIL(Core::File::standard_error());
                        for (auto const& hunk : maybe_diff.value())
                            TRY_OR_FAIL(Diff::write_unified(hunk, *out, Diff::ColorOutput::Yes));
                    }
                }
            },
            false }));

        return IterationDecision::Continue;
    }));
}

static auto init_test_cases = [] {
    find_tests("./Tests"sv);
    return 0;
}();
