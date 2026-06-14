#pragma once
#include "config_parser.hpp"


// methods with 'r*' are  readonly,
// whereas 'w*' ones are config writes.
struct MasterConfigParser {
    toml::table m_table;
    std::map<std::string, std::string> vars;
    std::vector<Profile> profiles;

    // Properties of a profile
    static COMPTIME_STR P_ACTIVE_PROF   = "active-profile";
    static COMPTIME_STR PP_PROFILE  = "profile";
    static COMPTIME_STR PP_GH_HOST  = "gh-host";
    static COMPTIME_STR PP_REPO_URL = "repo-url";
    static COMPTIME_STR PP_EXTERNAL = "external";

    Report rParse(const fs::path& path)
    {
        try {
            m_table = toml::parse_file(path.string());
        } catch (const toml::parse_error& e) {
            return Report::Bad("Master config parse-error: {}", e.description());
        }
        return Report::Good();
    }

    void rEval()
    {
        if (auto v = m_table["active"].value<std::string>()) {
            vars["active-profile"] = *v;
        }

        if (auto* profiles_array = m_table["profiles"].as_table()) {
            for (auto& [key, value]  : *profiles_array) {
                auto& prof = *value.as_table();

                std::string name = std::string(key.str());  // name of the table itself
                std::string gh   = prof[PP_GH_HOST].value_or("");
                std::string url  = prof[PP_REPO_URL].value_or("");
                bool        ext  = prof[PP_EXTERNAL].value_or(false);

                if (name.empty() || url.empty() || gh.empty()) {
                    ;
                } else {
                    profiles.emplace_back(name, gh, url, ext);
                }
            }
        }
    }

    Report rUnwrap() {
        Report report;

        for (auto& prof_1  : profiles) {
            for (auto& prof_2  : profiles) {
                if (&prof_1 == &prof_2) continue;  // skip same object
                if (prof_1.name == prof_2.name) {  // check if they have same name
                    report.addComplain("Duplicate-profile: {}", prof_1.name);
                }
            }
        }

        return report? Report::Bad("\nBad config!") : Report::Good();
    }


    Report wAddProfile(const Profile& prof) {
        auto profile_tb = toml::table{};
        profile_tb.insert(PP_REPO_URL, prof.repoUrl());
        profile_tb.insert(PP_EXTERNAL, prof.is_external);
        profile_tb.insert(PP_GH_HOST, prof.github_host);

        m_table["profiles"].as_table()->insert(prof.name, profile_tb);
        return Report::Good();
    }

    Report wRemoveProfile(const strview name) {
        m_table["profile"].as_table()->erase(name);
        return Report::Good();
    }

    Report wActivateProfile(const strview name) {
        m_table.insert_or_assign(P_ACTIVE_PROF, name);
        return Report::Good();
    }

    Report wSaveConfig(const fs::path& path) {
        std::ofstream master_cfg(path, std::ios::out | std::ios::trunc);
        if (master_cfg.is_open()) {
            master_cfg << m_table;
        } else {
            return Report::Bad("Couldn't open master config for writing.");
        }
        return Report::Good();
    }
};

