#include <CLI/CLI.hpp>
#include "cli.hpp"

struct CmdLine::Impl {
    CLI::App cli {"Config Manager CLI tool"};

    int argc; char** argv;

    const char* const default_msg = {
        "Dotty: Config/Dotfiles manager\n"
    };


    // `prof_agnostic` set to `true` means that this subcommand doesn't require an active profile
    struct CmdEntry { std::function<int32()> action; bool prof_agnostic=true; };
    std::map<CLI::App*, CmdEntry> sub_commands;

    // cache values
    struct {
        // strview clean_configs;
        std::string push_commit_msg;
        strview config_;
        std::string list_properties;
        std::string new_name;
            std::string new_repo_name;
            std::string new_repo_visb;
        strview delete_profile;
    } v;
};

#define APP (impl->cli)



CmdLine::CmdLine(int argc, char** argv) : impl(new Impl()) {
    impl->argc = argc;
    impl->argv = impl->cli.ensure_utf8(argv);
}

CmdLine::~CmdLine() {delete impl;}


CLI::App* CmdLine::newSubCmd(
    CLI::App* parent, const char* name, const std::function<int32()>& fn, const char* desc,
    bool profile_agnostic, int32 require_subcommands
) {
    auto* sub_cmd = parent->add_subcommand(name, desc);
    sub_cmd->require_subcommand(require_subcommands);

    impl->sub_commands.emplace(sub_cmd, fn);
    return sub_cmd;
}


#define BIND(_fn_name) [this](){return _fn_name;}
int32 CmdLine::setup()
{
    using SubCmd = CLI::App;

    SubCmd* sc_init = newSubCmd(&APP, "init",
        BIND(do_init()), "Initialize dotty config manager in your system", true, 0);
    //
    SubCmd* sc_update = newSubCmd(&APP, "update",
        BIND(do_update()), "Write configs to configs storage", false, 0);
        sc_update->alias("u");
    //
    SubCmd* sc_push = newSubCmd(&APP, "push",
        BIND(do_push(impl->v.push_commit_msg.c_str())), "Push config storage to the github repo", false, 0);
        sc_push->add_option("commit message", impl->v.push_commit_msg, "Push with commit message")->required();
    //
    SubCmd* sc_pull = newSubCmd(&APP, "pull",
        BIND(do_pull()), "Pull your config from the repo", false, 0);
    //
    SubCmd* sc_config = newSubCmd(&APP, "config",
        BIND(do_config(impl->v.config_)), "Configuration utilities", false, 0);
        sc_config->alias("c")->alias("cfg");
        sc_config->add_option("configuration", impl->v.config_, "Configuire master");
    //
    SubCmd* sc_profile_ = newSubCmd(&APP, "profile",
        BIND(do_profile_("")), "Profile related commands", true, 1)
        ->alias("p")->alias("pro");
    //
        SubCmd* ssc_list = newSubCmd(sc_profile_, "list",
            BIND(do_p_list(impl->v.list_properties)), "List all existing profiles", true, 0);
            ssc_list->alias("l")->alias("ls");
            ssc_list->add_option("properties", impl->v.list_properties, "List profiles and opted properties")->default_val("name,url");
    //
        SubCmd* ssc_new = newSubCmd(sc_profile_, "new",
            BIND(do_p_new(impl->v.new_name, impl->v.new_repo_name, impl->v.new_repo_visb)), "Create a new profile", true, 1);
            ssc_new->alias("n")->alias("+");
            ssc_new->add_option("-r");
    //
        SubCmd* ssc_delete = newSubCmd(sc_profile_, "delete",
            BIND(do_p_delete(impl->v.delete_profile)), "Delete a profile", true, 1);
            ssc_delete->alias("d")->alias("-");
            ssc_delete->add_option("profile", impl->v.delete_profile, "Delete ")->required();

    //
    // SC sc_clean = newSubCmd("clean", BIND(do_clean(impl->v.clean_configs)), "Clean all configs for current profile", 1);
    //     sc_clean->add_option("config", impl->v.clean_configs, "Clean configs");
    //

    CLI11_PARSE(impl->cli, impl->argc, impl->argv);
    return EXIT_SUCCESS;
}
#undef BIND


int32 CmdLine::run()
{
    std::string active_p = dotty.activeProf();

    if (APP.count_all() == 1) {
        cm::print(impl->default_msg);

        dotty.load(true);
        if (active_p == dotty.NO_PROFILE) {
            cm::print("Active profile is not set yet!\n");
        } else {
            cm::print("Active-profile: ", "\033[32m", active_p, "\033[0m", "\n");
        }
    }

    dotty.load(true);

    for (auto& [sub_cmd, data]  : impl->sub_commands) {
        if (sub_cmd->parsed()) {
            if (!data.prof_agnostic && (active_p == dotty.NO_PROFILE)) {
                cm::terminate("", sub_cmd->get_name(), " command requires an active profile!\n");
            }
            data.action.operator()();
        }
    }

    return EXIT_SUCCESS;
}

