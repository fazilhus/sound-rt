#pragma once

#include <filesystem>


namespace fs {
    static std::filesystem::path project_root = std::filesystem::current_path().parent_path().parent_path();

    static std::filesystem::path create_path_from_rel(const std::string& filepath) {
        auto new_path = project_root / filepath;
        return new_path.make_preferred();
    }

    static std::string create_path_from_rel_s(const std::string& filepath) {
        return create_path_from_rel(filepath).string();
    }
} // namespace fs