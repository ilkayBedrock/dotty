#pragma once
#include "core.h"

struct Cfman
{
    const fs::path user_home = cm::userHomePath(true, "$HOME is empty");
    const fs::path master_config = "~/dotty.toml";

    fs::path config_d = "~/.config/dotty";
    fs::path profile_name = "main";
    const char* config_source_name = "config";
    fs::path config = config_d/profile_name/config_source_name;

    fs::path storage_d = "~/.local/share/dotty";

    using SrcDest = struct{ fs::path src, path; };
    std::vector<SrcDest> path_pairs = {};

    enum class Res : uint8_t {
        OK=0,
        ERR=1,
        FileCouldNotBeOpened,
        DirectoryCouldNotBeCreated,
        ProfileDoesNotExist,
        ProfileAlreadyExists,
        ProfileAlreadySet,
    };

    // Get current profile as string
    [[gnu::pure]]
    std::string currentProfile() {
        std::string current = profile_name;
        return current;
    }

    // Create a folder and register a new profile
    Res newProfile(const std::string& name) {
        if (fs::exists(config_d/name)) return Res::ProfileAlreadyExists;
        if (fs::create_directory(config_d/name)) {
            ;
            if (cm::newFile(config_d/name/"config")) {
                ;
            } else return Res::FileCouldNotBeOpened;
        } else return Res::DirectoryCouldNotBeCreated;
        return Res::OK;
    }

    // Set current dotty profile
    Res setProfile(const std::string& name) {
        if (!fs::exists(config_d/name)) return Res::ProfileDoesNotExist;
        if (profile_name.string() == name) return Res::ProfileAlreadySet;
        profile_name = name;
        return Res::OK;
    }

    // Copy all source files to destination files, pairs defined by a member
    void write() {
        for (auto [src, dest] : path_pairs) {
            fs::copy_file(cm::parsePathTilde(src), cm::parsePathTilde(dest), fs::copy_options::update_existing);
        }
    }
};

inline Cfman dotty;


struct Token {
    enum TT {
        STRING='s',
        REDIRECTOR='r',

        IDENT='a',
        EQUAL='=',

        NONE='_',
        UNKNOWN='!'
    } type;
    std::string name;
};

struct Lexer {
    std::string line;
    uint32 pos;
    std::vector<Token> tokens;

    char get() { return line[pos]; }
    bool checks() { return line.size() > pos; }
    void step() { if(checks()) ++pos; }
    void skipws() { while(checks() && get()==' ') step(); }

    std::string lexString() {
        step();
        std::string string;
        uint32 len = 0;
        while (checks() && ++len) {
            if (get() == '"') goto ret;
            if (len > 1024) cm::terminate("String length too much!\n");
            string += get();
            step();
        }
    ret:
        step();
        return string;
    }

    std::string lexRedirector() {
        step();
        bool bad = true;
        if (get() == '>') bad = false;

        if (bad) cm::terminate();
        else return (step(), std::string(1, '>') + '>');
    }

    std::string lexEqual() {
        step();
        return "=";
    }

    void print() {
        for (uint32 i=0;  i < tokens.size();  ++i) {
            cm::print((char)tokens[i].type, " : ", tokens[i].name, "\n");
        }
    }

    void lexMain() {
        tokens.clear();
        pos = 0;
        Token tok = {Token::NONE, "<none>"};

        while(checks())
        {
            skipws();

            if (get() == '"') {
                tok.name = lexString();
                tok.type = Token::STRING;
            }
            else if (get() == '>') {
                tok.name = lexRedirector();
                tok.type = Token::REDIRECTOR;
            }
            else if (get() == '=') {
                tok.name = lexEqual();
                tok.type = Token::EQUAL;
            }
            else
            {
                tok.name = "<error>";
                tok.type = Token::UNKNOWN;
            }

            skipws();
            tokens.emplace_back(tok);
        }
    }

    auto result() {
        return tokens;
    }
};




struct ConfigParser {
    std::vector<Token> tokens;
    uint32 idx;
    std::vector<Cfman::SrcDest> path_pairs;

    Token get() { return tokens[idx]; }
    bool checks() { return tokens.size() > idx; }
    void advance() { if (checks()) ++idx; }

    void parseMain()
    {
        idx = 0;

        while(checks())
        {
            if (get().type == Token::STRING) {
                std::string src = get().name;

                advance();
                if (get().type == Token::REDIRECTOR) {
                    ;
                    advance();
                    if (get().type == Token::STRING) {
                        std::string dest = get().name;

                        path_pairs.emplace_back(Cfman::SrcDest{src, dest});
                    }
                    else cm::terminate("After a REDIRECTOR token next token.type should be STRING\n");
                }
                else cm::terminate("After a STRING token next token.type should be REDIRECTOR\n");
            }
            else cm::terminate("First token.type should be STRING\n");

            advance();
        }
    }

    auto result() {
        return path_pairs;
    }
};




struct MasterConfigParser {
    std::vector<Token> tokens;
    uint32 idx;
    std::map<std::string, std::string> table;
    // distributions of table
    std::map<std::string, std::string> vars;
    std::vector<std::string> profiles;

// builds property cstring
#define PROP(obj, prop) cm::concats(obj, prop)

    Token get() { return tokens[idx]; }
    bool checks() { return tokens.size() > idx; }
    void advance() { if (checks()) ++idx; }


    void parse()
    {
        idx = 0;

        while(checks())
        {
            if (get().type == Token::IDENT) {
                std::string key = get().name;

                advance();
                if (get().type == Token::EQUAL) {
                    ;
                    advance();
                    if (get().type == Token::STRING) {
                        std::string value = get().name;

                        table.insert({key, value});
                    }
                    else cm::terminate("After a EQUAL token next token.type should be STRING\n");
                }
                else cm::terminate("After an IDENT token next token.type should be EQUAL\n");
            }
            else cm::terminate("First token.type should be IDENT\n");

            advance();
        }
    }

    void eval()
    {
        COMPTIME_STR PROFILE = "profile";
            COMPTIME_STR PROFILE_ADD = ".add";
            COMPTIME_STR PROFILE_ACTIVE = ".active";
        COMPTIME_STR MENTION = "@";
            COMPTIME_STR MENTION_GH_ACC = "gh-acc";
            COMPTIME_STR MENTION_REPO_URL = "repo-url";


        for (const auto& [first, second]  : table)
        {
            std::string key = first;

            if (cm::prefix_strip(key, PROFILE, &key)) {
                if (cm::prefix_strip(key, PROFILE_ADD, &key)) {
                    profiles.emplace_back(second);
                }
                else if (cm::prefix_strip(key, PROFILE_ACTIVE, &key)) {
                    vars[PROP(PROFILE, PROFILE_ACTIVE)] = second;
                    dotty.setProfile(vars[PROP(PROFILE, PROFILE_ACTIVE)]);
                }
            }
            else if (cm::prefix_strip(key, MENTION, &key)) {
                if (!cm::vec_contains(profiles, key)) {
                    cm::print("Master-Config: Profile does not exist: ", key);
                }
                const char* mentioned_profile = key.data();
                if (cm::prefix_strip(key, MENTION_GH_ACC, &key)) {
                    vars[PROP(mentioned_profile, MENTION_GH_ACC)] = second;
                }
                else if (cm::prefix_strip(key, MENTION_REPO_URL, &key)) {
                    vars[PROP(mentioned_profile, MENTION_REPO_URL)] = second;
                }
            }
            else
            {
                cm::print("Master-Config: Eval-Error: Invalid tokens: '", first,"', '",second, "'\n");
            }
        }
    }
};

