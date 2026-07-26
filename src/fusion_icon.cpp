#include <gtkmm.h>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <unistd.h>

bool debug_mode = false;

static void silent_log(const gchar*, GLogLevelFlags, const gchar*, gpointer) {}

class FusionIcon {
public:
    FusionIcon();
    void on_left_click();
    void on_right_click(guint button, guint time);
    void change_window_manager(const std::string& wm);
    void rebuild_wm_menu();
    void rebuild_decorator_menu();
    void change_window_decorator(const std::string& decorator);
    std::string detect_window_decorator();
    void signal_handler(int sig);
    void start_compiz_setsid();
    std::string detect_desktop_name();
    std::string detect_window_manager();
    void clean_exit();

private:
    Glib::RefPtr<Gtk::StatusIcon> icon;
    Gtk::Menu menu;
    Gtk::ImageMenuItem* item_restart;
    Gtk::ImageMenuItem* item_exit;
    Gtk::ImageMenuItem* item_ccsm;
    Gtk::ImageMenuItem* item_emerald;
    Gtk::SeparatorMenuItem separator1, separator2;
    Gtk::Menu wm_menu;
    Gtk::ImageMenuItem* item_select_wm;
    Gtk::Menu decorator_menu;
    Gtk::ImageMenuItem* item_select_decorator;
    bool gtk_window_decorator_installed;
    bool compiz_installed;
    bool ccsm_installed;
    bool metacity_installed;
    bool marco_installed;
    bool emerald_installed;
    bool compiz_restarted;
    bool compiz_started_by_fusion_icon;
    bool indirect_rendering;
    bool loose_binding;

    Gtk::Menu compiz_options_menu;
    Gtk::ImageMenuItem* item_compiz_options;
    Gtk::CheckMenuItem item_indirect, item_loose;
};

FusionIcon* global_fusion_icon = nullptr;

FusionIcon::FusionIcon() : compiz_restarted(false), compiz_started_by_fusion_icon(false),
    indirect_rendering(false), loose_binding(false) {
    icon = Gtk::StatusIcon::create(Gdk::Pixbuf::create_from_file(ICON_DIR "/fusion-icon.png"));
    icon->set_visible(true);

    compiz_installed = (std::system("which compiz > /dev/null 2>&1") == 0);
    ccsm_installed = (std::system("which ccsm > /dev/null 2>&1") == 0);
    metacity_installed = (std::system("which metacity > /dev/null 2>&1") == 0);
    marco_installed = (std::system("which marco > /dev/null 2>&1") == 0);
    emerald_installed = (std::system("which emerald-theme-manager > /dev/null 2>&1") == 0);
    gtk_window_decorator_installed = (std::system("which gtk-window-decorator > /dev/null 2>&1") == 0);

    std::string current_desktop = detect_desktop_name();
    std::string window_manager = detect_window_manager();

    if (current_desktop.empty())		{ g_error("DM not detected!!"); }
    if (current_desktop != "Unknown")	{ g_message("DM: %s", current_desktop.c_str()); }
	else								{ g_warning("Unknow DM!!"); }

	if (window_manager.empty())			{ g_error("WM not detected!!"); }
    if (window_manager != "Unknown")	{ g_message("WM: %s", window_manager.c_str()); }
	else								{ g_warning("No WM detected at all!"); start_compiz_setsid(); }

    icon->signal_activate().connect(sigc::mem_fun(*this, &FusionIcon::on_left_click));
    icon->signal_popup_menu().connect(sigc::mem_fun(*this, &FusionIcon::on_right_click));

    if (ccsm_installed) {
        auto* img = Gtk::manage(new Gtk::Image());
        img->set_from_icon_name("ccsm", Gtk::ICON_SIZE_MENU);
        item_ccsm = Gtk::manage(new Gtk::ImageMenuItem(*img, "Settings Manager"));
        item_ccsm->signal_activate().connect([]() {
			try {
				Glib::spawn_command_line_async("setsid ccsm");
				g_message("CCSM launched successfully");
			} catch (const Glib::Error& error) { g_warning("Error launching CCSM: %s", error.what().c_str()); }
        });
        menu.append(*item_ccsm);
    }

    if (emerald_installed) {
        auto* img = Gtk::manage(new Gtk::Image());
        img->set_from_icon_name("emerald-theme-manager", Gtk::ICON_SIZE_MENU);
        item_emerald = Gtk::manage(new Gtk::ImageMenuItem(*img, "Emerald Theme Manager"));
        item_emerald->signal_activate().connect([]() {
			try {
				Glib::spawn_command_line_async("setsid emerald-theme-manager");
				g_message("Emerald Theme Manager launched successfully");
			} catch (const Glib::Error& error) { g_warning("Error launching Emerald Theme Manager: %s", error.what().c_str()); }
        });
        menu.append(*item_emerald);
    }

    menu.append(separator1);

    if (compiz_installed) {
        auto* img = Gtk::manage(new Gtk::Image());
        img->set_from_icon_name("view-refresh", Gtk::ICON_SIZE_MENU);
        item_restart = Gtk::manage(new Gtk::ImageMenuItem(*img, "Restart Compiz"));
        item_restart->signal_activate().connect([this]() {
			try {
				Glib::spawn_command_line_async("setsid compiz --replace");
				g_message("Restarting Compiz...");
			} catch (const Glib::Error& error) { g_warning("Error Compiz restart: %s", error.what().c_str()); }
            compiz_restarted = true;
        });
        menu.append(*item_restart);
    }

    menu.append(separator2);

    // Select Window Manager submenu
    {
        auto* img = Gtk::manage(new Gtk::Image());
        img->set_from_icon_name("compiz", Gtk::ICON_SIZE_MENU);
        item_select_wm = Gtk::manage(new Gtk::ImageMenuItem(*img, "Select Window Manager"));
    }
    item_select_wm->set_submenu(wm_menu);
    wm_menu.signal_show().connect(sigc::mem_fun(*this, &FusionIcon::rebuild_wm_menu));

    menu.append(*item_select_wm);

    // Compiz Options submenu
    if (compiz_installed) {
        {
            auto* img = Gtk::manage(new Gtk::Image());
            img->set_from_icon_name("compiz", Gtk::ICON_SIZE_MENU);
            item_compiz_options = Gtk::manage(new Gtk::ImageMenuItem(*img, "Compiz Options"));
        }
        item_compiz_options->set_submenu(compiz_options_menu);

        item_indirect.set_label("Indirect Rendering");
        item_indirect.signal_toggled().connect([this]() {
            indirect_rendering = item_indirect.get_active();
            g_message("Indirect Rendering: %s", indirect_rendering ? "ON" : "OFF");
        });
        compiz_options_menu.append(item_indirect);

        item_loose.set_label("Loose Binding");
        item_loose.signal_toggled().connect([this]() {
            loose_binding = item_loose.get_active();
            g_message("Loose Binding: %s", loose_binding ? "ON" : "OFF");
        });
        compiz_options_menu.append(item_loose);

        menu.append(*item_compiz_options);
    }

    // Select Window Decorator submenu
    if (compiz_installed) {
        {
            auto* img = Gtk::manage(new Gtk::Image());
            img->set_from_icon_name("window-new", Gtk::ICON_SIZE_MENU);
            item_select_decorator = Gtk::manage(new Gtk::ImageMenuItem(*img, "Select Window Decorator"));
        }
        item_select_decorator->set_submenu(decorator_menu);
        decorator_menu.signal_show().connect(sigc::mem_fun(*this, &FusionIcon::rebuild_decorator_menu));

        menu.append(*item_select_decorator);
    }

    {
        auto* img = Gtk::manage(new Gtk::Image());
        img->set_from_icon_name("application-exit", Gtk::ICON_SIZE_MENU);
        item_exit = Gtk::manage(new Gtk::ImageMenuItem(*img, "Exit"));
    }
    item_exit->signal_activate().connect(sigc::mem_fun(*this, &FusionIcon::clean_exit));
    menu.append(*item_exit);

    menu.show_all();
    global_fusion_icon = this;

    std::signal(SIGINT, [](int sig) {
        if (global_fusion_icon) {
            global_fusion_icon->signal_handler(sig);
        }
    });
}

void FusionIcon::on_left_click() {
    g_message("Left click detected!");
}

void FusionIcon::on_right_click(guint button, guint time) {
    g_message("Right click detected! Showing menu...");
    menu.popup_at_pointer(nullptr);
}

void FusionIcon::change_window_manager(const std::string& wm) {
    g_message("Switching to %s...", wm.c_str());

    try {
        pid_t pid = fork();

        if (pid == 0) {
            setsid();
            signal(SIGINT, SIG_IGN);
            signal(SIGHUP, SIG_IGN);
            signal(SIGTERM, SIG_IGN);

            std::vector<const char*> args;
            args.push_back(wm.c_str());
            args.push_back("--replace");

            if (wm == "compiz") {
                if (indirect_rendering) args.push_back("--indirect-rendering");
                if (loose_binding) args.push_back("--loose-binding");
            }

            args.push_back(nullptr);

            execvp(wm.c_str(), const_cast<char* const*>(args.data()));
            _exit(1);
        } else if (pid > 0) {
            g_message("%s started with PID %d", wm.c_str(), pid);
            // Проверяем 4 раза по 500мс — если не запустился, повторяем
            std::string check_cmd = "for i in 1 2 3 4; do sleep 0.5; pgrep -x " + wm + " > /dev/null 2>&1 && exit 0; done; notify-send 'Fusion Icon 2' '" + wm + " failed to start, retrying...' && setsid " + wm + " --replace &";
            int ret = std::system(("bash -c '" + check_cmd + "' &").c_str());
            (void)ret;
        } else {
            g_warning("Error forking process for %s", wm.c_str());
        }
    } catch (const std::exception& error) {
        g_warning("Error switching to %s: %s", wm.c_str(), error.what());
    }
}

std::string FusionIcon::detect_desktop_name() {
    char* desktop_env = getenv("XDG_CURRENT_DESKTOP");
    return desktop_env ? std::string(desktop_env) : "Unknown";
}

std::string FusionIcon::detect_window_manager() {
    const char* wm_list[] = {"compiz", "marco", "metacity"};
    for (const char* wm : wm_list) {
        if (std::system(("pgrep -f " + std::string(wm) + " > /dev/null 2>&1").c_str()) == 0)
            return wm;
    }
    return "Unknown";
}

std::string FusionIcon::detect_window_decorator() {
    // pgrep -x для emerald (короткое имя), pgrep -f для gtk-window-decorator
    // Исключаем свой PID чтобы не матчится сам fusion-icon2
    pid_t my_pid = getpid();
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pgrep -x emerald | grep -v ^%d$ > /dev/null 2>&1", my_pid);
    if (std::system(cmd) == 0)
        return "emerald";
    snprintf(cmd, sizeof(cmd), "pgrep -f gtk-window-decorator | grep -v ^%d$ > /dev/null 2>&1", my_pid);
    if (std::system(cmd) == 0)
        return "gtk-window-decorator";
    return "Unknown";
}

void FusionIcon::change_window_decorator(const std::string& decorator) {
    g_message("Switching decorator to %s...", decorator.c_str());

    std::string cmd = "setsid " + decorator + " --replace &";
    int ret = std::system(cmd.c_str());
    (void)ret;

    std::string pgrep_cmd = (decorator == "emerald") ? "pgrep -x emerald" : "pgrep -f gtk-window-decorator";
    std::string check_cmd = "for i in 1 2 3 4; do sleep 0.5; " + pgrep_cmd + " > /dev/null 2>&1 && exit 0; done; notify-send 'Fusion Icon 2' '" + decorator + " failed to start, retrying...' && setsid " + decorator + " --replace &";
    int ret2 = std::system(("bash -c '" + check_cmd + "' &").c_str());
    (void)ret2;
}

void FusionIcon::rebuild_wm_menu() {
    auto children = wm_menu.get_children();
    for (auto* child : children)
        wm_menu.remove(*child);

    std::string current_wm = detect_window_manager();

    auto* parent_img = dynamic_cast<Gtk::Image*>(item_select_wm->get_image());
    if (parent_img) {
        if (current_wm == "compiz")
            parent_img->set_from_icon_name("compiz", Gtk::ICON_SIZE_MENU);
        else if (current_wm == "marco")
            parent_img->set_from_icon_name("marco", Gtk::ICON_SIZE_MENU);
        else if (current_wm == "metacity")
            parent_img->set_from_icon_name("metacity", Gtk::ICON_SIZE_MENU);
        else
            parent_img->set_from_icon_name("compiz", Gtk::ICON_SIZE_MENU);
    }

    // Вычисляем максимальную длину для выравнивания
    size_t max_len = 0;
    if (compiz_installed) max_len = std::max(max_len, std::string("compiz").size());
    if (marco_installed) max_len = std::max(max_len, std::string("marco").size());
    if (metacity_installed) max_len = std::max(max_len, std::string("metacity").size());

    auto add_item = [&](const std::string& wm, const char* icon_name) {
        auto* img = Gtk::manage(new Gtk::Image());
        if (wm == "marco")
            img->set_from_icon_name("marco", Gtk::ICON_SIZE_MENU);
        else
            img->set_from_icon_name(icon_name, Gtk::ICON_SIZE_MENU);
        auto* item = Gtk::manage(new Gtk::ImageMenuItem(*img, wm));

        // Паддинг имени WM до max_len
        std::string padded = wm + std::string(max_len - wm.size(), ' ');

        Gtk::Label* label = dynamic_cast<Gtk::Label*>(item->get_child());
        if (label) {
            label->set_use_markup(true);
            if (current_wm == wm)
                label->set_markup("<tt>▶ " + padded + " ◀</tt>");
            else
                label->set_markup("<tt>  " + padded + "  </tt>");
        }

        item->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &FusionIcon::change_window_manager), wm));
        wm_menu.append(*item);
    };

    if (compiz_installed)
        add_item("compiz", "compiz");
    if (marco_installed)
        add_item("marco", "marco");
    if (metacity_installed)
        add_item("metacity", "metacity");

    wm_menu.show_all();
}

void FusionIcon::rebuild_decorator_menu() {
    auto children = decorator_menu.get_children();
    for (auto* child : children)
        decorator_menu.remove(*child);

    std::string current_decorator = detect_window_decorator();
    std::string current_wm = detect_window_manager();

    auto add_item = [&](const std::string& dec, const char* icon_name, bool sensitive = true) {
        auto* img = Gtk::manage(new Gtk::Image());
        img->set_from_icon_name(icon_name, Gtk::ICON_SIZE_MENU);
        std::string label = (current_decorator == dec) ? ("[ " + dec + " ]") : dec;
        auto* item = Gtk::manage(new Gtk::ImageMenuItem(*img, label));
        item->set_sensitive(sensitive);
        item->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &FusionIcon::change_window_decorator), dec));
        decorator_menu.append(*item);
    };

    bool is_compiz = (current_wm == "compiz");

    if (emerald_installed)
        add_item("emerald", "emerald-theme-manager", is_compiz);
    if (gtk_window_decorator_installed)
        add_item("gtk-window-decorator", "preferences-system", true);

    decorator_menu.show_all();
}

void FusionIcon::start_compiz_setsid() {
    if (std::system("pgrep -x compiz > /dev/null 2>&1") != 0) {
        g_message("Запускаем Compiz...");
        int result = std::system("setsid compiz --replace &");
        compiz_started_by_fusion_icon = true;
    }
}

void FusionIcon::signal_handler(int sig) {
    g_message("Caught signal: %d", sig);
    clean_exit();
}

void FusionIcon::clean_exit() {
    g_message("Exiting Fusion-icon-2...");
    Gtk::Main::quit();
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-d" || std::string(argv[i]) == "--debug")
            debug_mode = true;
        else
            fprintf(stderr, "Unknown argument: %s, ignoring...\n", argv[i]);
    }

    if (!debug_mode) {
        g_log_set_handler("Gtk", (GLogLevelFlags)(G_LOG_LEVEL_MASK), silent_log, nullptr);
        g_log_set_handler("fusion-icon", (GLogLevelFlags)(G_LOG_LEVEL_MASK), silent_log, nullptr);
        g_log_set_default_handler(silent_log, nullptr);
    }

    Gtk::Main kit(argc, argv);
    FusionIcon fusionIcon;
    Gtk::Main::run();
}
