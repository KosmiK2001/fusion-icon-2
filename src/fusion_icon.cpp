#include <gtkmm.h>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
//#include <sstream>
//#include <fstream>

class FusionIcon {
public:
    FusionIcon();
    void on_left_click();
    void on_right_click(guint button, guint time);
    void change_window_manager(const std::string& wm);
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
	Gtk::Menu wm_menu;
	Gtk::ImageMenuItem* item_select_wm;
	Gtk::RadioMenuItem::Group wm_group;
    Gtk::ImageMenuItem* item_ccsm;
    Gtk::ImageMenuItem* item_emerald;
    Gtk::SeparatorMenuItem separator1, separator2;
    bool compiz_installed;
    bool ccsm_installed;
    bool metacity_installed;
    bool marco_installed;
    bool emerald_installed;
    bool compiz_restarted;
    bool compiz_started_by_fusion_icon;
    bool indirect_rendering;
    bool loose_binding;
	std::string selected_window_manager;

    Gtk::Menu compiz_options_menu;
    Gtk::ImageMenuItem* item_compiz_options;
    Gtk::CheckMenuItem item_indirect, item_loose;
};

// Глобальный указатель на объект FusionIcon
FusionIcon* global_fusion_icon = nullptr;

FusionIcon::FusionIcon() : compiz_restarted(false), compiz_started_by_fusion_icon(false),
    indirect_rendering(false), loose_binding(false) {
    icon = Gtk::StatusIcon::create(Gdk::Pixbuf::create_from_file("/usr/share/icons/hicolor/48x48/apps/fusion-icon.png"));
    icon->set_visible(true);

    // Проверка наличия программ
    compiz_installed = (std::system("which compiz > /dev/null 2>&1") == 0);
    ccsm_installed = (std::system("which ccsm > /dev/null 2>&1") == 0);
    metacity_installed = (std::system("which metacity > /dev/null 2>&1") == 0);
    marco_installed = (std::system("which marco > /dev/null 2>&1") == 0);
    emerald_installed = (std::system("which emerald-theme-manager > /dev/null 2>&1") == 0);

    // Определяем текущее окружение
    std::string current_desktop = detect_desktop_name();
    std::string window_manager = detect_window_manager();
	selected_window_manager = window_manager;

    if (current_desktop.empty())		{ g_error("DM not detected!!"); }
    if (current_desktop != "Unknown")	{ g_message("DM: %s", current_desktop.c_str());}
	else								{ g_warning("Unknow DM!!"); }

	if (window_manager.empty())			{ g_error("WM not detected!!"); }
    if (window_manager != "Unknown")	{ g_message("WM: %s", window_manager.c_str()); }
	else								{ g_warning("No WM detected at all!"); start_compiz_setsid(); }

    // Подключаем сигналы кликов
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
        item_restart = Gtk::manage(new Gtk::ImageMenuItem(*img, "Reload Window Manager"));
		item_restart->signal_activate().connect([this]() {

		    if (selected_window_manager.empty() ||
	        selected_window_manager == "Unknown")
		    {
		        g_warning("No window manager selected.");
	        	return;
		    }

	    change_window_manager(selected_window_manager);
	    compiz_restarted = true;
	});
        menu.append(*item_restart);
    }

    menu.append(separator2);

    {
        auto* img = Gtk::manage(new Gtk::Image());
        img->set_from_icon_name("compiz", Gtk::ICON_SIZE_MENU);
        item_select_wm = Gtk::manage(new Gtk::ImageMenuItem(*img, "Select Window Manager"));
    }
	item_select_wm->set_submenu(wm_menu);

	auto add_wm =
	[&](const std::string& wm)
	{
	    auto *item =
	        Gtk::manage(new Gtk::RadioMenuItem(wm_group, wm));

	    if (wm == selected_window_manager)
	        item->set_active(true);

	    item->signal_activate().connect(
        	sigc::bind(
	            sigc::mem_fun(*this,
	            &FusionIcon::change_window_manager),
	            wm));

	    wm_menu.append(*item);
	};

	if (compiz_installed)
    	add_wm("Compiz");

	if (marco_installed)
	    add_wm("Marco");

	if (metacity_installed)
	    add_wm("Metacity");

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
	selected_window_manager = wm;
    g_message("Switching to %s...", wm.c_str());

    try {
        // Создаем fork процесса
        pid_t pid = fork();

        if (pid == 0) {
            // Дочерний процесс
            // Создаем новую сессию, чтобы отсоединить от терминала
            setsid();

            // Устанавливаем обработчики сигналов на игнорирование
            signal(SIGINT, SIG_IGN);
            signal(SIGHUP, SIG_IGN);
            signal(SIGTERM, SIG_IGN);

            // Подготавливаем аргументы для execvp
            std::vector<const char*> args;
            args.push_back(wm.c_str());
            args.push_back("--replace");

            // Добавляем опции Compiz если выбран Compiz
            if (wm == "Compiz") {
                if (indirect_rendering) args.push_back("--indirect-rendering");
                if (loose_binding) args.push_back("--loose-binding");
            }

            args.push_back(nullptr);

            // Выполняем замену текущего процесса на wm
            execvp(wm.c_str(), const_cast<char* const*>(args.data()));

            // Если execvp вернул управление, значит произошла ошибка
            _exit(1);
        } else if (pid > 0) {
            // Родительский процесс
            g_message("%s started with PID %d", wm.c_str(), pid);
        } else {
            // Ошибка fork
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
        std::string cmd = "pgrep -x " + std::string(wm);
        if ( std::system((cmd + " > /dev/null 2>&1").c_str()) == 0) {
            return wm;
        }
    }

    return "Unknown";
}

void FusionIcon::start_compiz_setsid() {
    if (std::system("pgrep -x compiz > /dev/null 2>&1") != 0) {
        g_message("Запускаем Compiz...");
        std::string cmd = "setsid compiz --replace";
        if (indirect_rendering) cmd += " --indirect-rendering";
        if (loose_binding) cmd += " --loose-binding";
        cmd += " &";
        int result = std::system(cmd.c_str());
        compiz_started_by_fusion_icon = true;
    }
}

void FusionIcon::signal_handler(int sig) {
    std::cout << "Caught signal: " << sig << std::endl;
    clean_exit();
}

void FusionIcon::clean_exit() {
    g_message("Exiting Fusion-icon-2...");
    std::exit(0);
}

int main() {
    Gtk::Main kit;
    FusionIcon fusionIcon;
    Gtk::Main::run();
}
