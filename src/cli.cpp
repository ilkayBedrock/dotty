#include "cli.hpp"


struct CmdLine::Impl {
    CLI::App cli {"Config Manager CLI tool"};
    int argc; char** argv;

    std::map<CLI::App*, std::function<int32()>> sub_commands;
};


CmdLine::CmdLine(int argc, char** argv) : impl(new Impl()) {
    impl->argc = argc;
    impl->argv = impl->cli.ensure_utf8(argv);
}

CmdLine::~CmdLine() {delete impl;}


int32 CmdLine::do_init() {
    // Check github authentication status
    cm::print("Checking GitHub CLI authentication...\n");
    int auth_status = std::system("gh auth status --hostname github.com > /dev/null 2>&1");
    if (auth_status != 0) {
        cm::terminate("gh is not authenticated. Please run 'gh auth login' first.\n");
    }
    cm::print("GitHub CLI authenticated.\n\n");

    // Prompts for making the repo
    std::string repo_name;
    cm::print("Enter a name for your dotty config repo: ");
    cm::prompt(repo_name);

    if (repo_name.empty()) {
        cm::terminate("Repo name cannot be empty.\n");
    }

    std::string visibility;
    cm::print("Repo visibility — enter 'public' or 'private' [private]: ");
    cm::prompt(visibility);

    if (visibility.empty() || visibility == "private") {
        visibility = "private";
    } else if (visibility == "public") {
        ;
    } else {
        cm::terminate("Invalid visibility '{}'. Must be 'public' or 'private'.\n", visibility);
    }

    fs::path repo_d = cm::parsePathTilde(dotty.config_d);

    cm::print("\nInitializing dotty config directory at {}...\n", repo_d.string());

    if (!fs::exists(repo_d)) {
        if (!fs::create_directories(repo_d))
            cm::terminate("Failed to create config directory: {}\n", repo_d.string());
    }

    cm::CmdStream cmd;
    cmd
        .add("git -C {} init", repo_d.string())
        .add("gh repo create {} --{} --source={} --remote=origin --push",
            repo_name, visibility, repo_d.string());
    cmd.run();

    cm::print("\nDotty initialized! Repo '{}' created as {} on GitHub.\n", repo_name, visibility);
    return EXIT_SUCCESS;
}


int32 CmdLine::do_write() {
    Cfman cfman;
    Lexer lexer;
    ConfigParser parser;
    std::ifstream conf(cm::parsePathTilde(cfman.config), std::ios::in);
    if (!conf.is_open()) cm::terminate("File could not be opened!\n");

    while (std::getline(conf, lexer.line)) {
        cm::print("Lexing config...\n");
        lexer.lexMain();
        cm::print("Parsing tokens...\n");
        parser.tokens = lexer.result();
        parser.parseMain();
        cm::print("Adding new values to the base...\n");
        cfman.path_pairs = parser.result();
    }
    cm::print("\n\nCopying configs to their destinations...\n");
    cfman.write();
    cm::print("\n\nLexed tokens:\n");
    lexer.print();

    return EXIT_SUCCESS;
}

int32 CmdLine::do_update(const char* commit_message) {
    cm::CmdStream cmd;
    cmd
        .add("cd {}", (dotty.storage_d/dotty.currentProfile() ).string())
        .add("git add .")
        .add("git commit {}", commit_message)
        .add("git push")
    ;
    cmd.run();

    return EXIT_SUCCESS;
}

int32 CmdLine::do_install() {
    cm::CmdStream cmd;
    cmd
        .add("")
    ;
    cmd.run();

    return EXIT_SUCCESS;
}


void CmdLine::newSubCmd(const char* name, const std::function<int32()>& fn, const char* desc)
{
    impl->sub_commands.emplace(
        impl->cli.add_subcommand(name, desc),
        fn
    );
}

#define BIND(_fn_name) [this](){return _fn_name;}
int32 CmdLine::setup()
{
    newSubCmd("init", BIND(do_init()), "Initialize dotty config manager in your system");
    newSubCmd("update", BIND(do_write()), "Update configs to configs storage");

    CLI11_PARSE(impl->cli, impl->argc, impl->argv);
    return EXIT_SUCCESS;
}
#undef BIND


int32 CmdLine::run()
{
    for (auto& [app_ptr, fn]  : impl->sub_commands) {
        if (impl->cli.got_subcommand(app_ptr)) {
            fn.operator()();
        }
    }

    return EXIT_SUCCESS;
}

