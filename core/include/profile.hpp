#pragma once
#include "core.hpp"

struct SrcDest { fs::path src, dest; };

struct Profile {
    std::string name;
    std::string github_host;
    std::string repo_name;
    bool is_external = false;

    Profile(
        const std::string& name, const std::string& github_name,
        const std::string& repo_url, bool is_external
    );

    std::string repoUrl() const;
    fs::path getConfigPath() const;

};
