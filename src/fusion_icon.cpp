#include <gtkmm.h>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <map>

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
    std::string detect_window_decorator(const std::string& wm = "");
    bool monitor_state();
    bool reap_zombies();
    void signal_handler(int sig);
    void start_compiz_setsid();
    std::string detect_desktop_name();
    std::string detect_window_manager();
    void clean_exit();
    void save_config();
    void load_config();

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
    bool nvidia_installed;
    bool compiz_installed;
    bool ccsm_installed;
    bool metacity_installed;
    bool marco_installed;
    bool emerald_installed;
    bool compiz_restarted;
    bool compiz_started_by_fusion_icon;
    bool indirect_rendering;
    bool loose_binding;

    std::string last_known_wm;
    std::string last_known_decorator;
    bool active_monitoring;

    Gtk::Menu compiz_options_menu;
    Gtk::ImageMenuItem* item_compiz_options;
    Gtk::CheckMenuItem item_indirect, item_loose;

    Gtk::Menu nvidia_menu;
    Gtk::ImageMenuItem* item_nvidia;

    Gtk::CheckMenuItem* item_fcp_ptr;
    Gtk::CheckMenuItem* item_ffcp_ptr;

    std::map<std::string, std::string> config;
    std::string config_path;
};

FusionIcon* global_fusion_icon = nullptr;

FusionIcon::FusionIcon() : compiz_restarted(false), compiz_started_by_fusion_icon(false),
    indirect_rendering(false), loose_binding(false), active_monitoring(false),
    item_fcp_ptr(nullptr), item_ffcp_ptr(nullptr) {
    // Путь к конфигу
    const char* home = getenv("HOME");
    config_path = home ? std::string(home) + "/.config/fusion-icon2/config" : "";

    // Загружаем настройки
    load_config();

    icon = Gtk::StatusIcon::create(Gdk::Pixbuf::create_from_file(ICON_DIR "/fusion-icon.png"));
    icon->set_visible(true);

    compiz_installed = (std::system("which compiz > /dev/null 2>&1") == 0);
    ccsm_installed = (std::system("which ccsm > /dev/null 2>&1") == 0);
    metacity_installed = (std::system("which metacity > /dev/null 2>&1") == 0);
    marco_installed = (std::system("which marco > /dev/null 2>&1") == 0);
    emerald_installed = (std::system("which emerald-theme-manager > /dev/null 2>&1") == 0);
    gtk_window_decorator_installed = (std::system("which gtk-window-decorator > /dev/null 2>&1") == 0);
    nvidia_installed = (std::system("lsmod 2>/dev/null | grep -q nvidia") == 0);

    std::string current_desktop = detect_desktop_name();
    std::string window_manager = detect_window_manager();

    if (current_desktop.empty())		{ g_error("DM not detected!!"); }
    if (current_desktop != "Unknown")	{ g_message("DM: %s", current_desktop.c_str()); }
	else								{ g_warning("Unknow DM!!"); }

	if (window_manager.empty())			{ g_error("WM not detected!!"); }
    if (window_manager != "Unknown")	{ g_message("WM: %s", window_manager.c_str()); }
	else								{ g_warning("No WM detected at all!"); start_compiz_setsid(); }

    // Применяем настройки из конфига если отличаются от текущего
    std::string cfg_wm = config["selected_wm"];
    if (!cfg_wm.empty() && cfg_wm != "Unknown" && cfg_wm != window_manager) {
        g_message("Config WM (%s) differs from current (%s), switching...", cfg_wm.c_str(), window_manager.c_str());
        change_window_manager(cfg_wm);
    }

    std::string cfg_dec = config["selected_decorator"];
    std::string cur_dec = detect_window_decorator();
    if (!cfg_dec.empty() && cfg_dec != "none" && cfg_dec != cur_dec) {
        g_message("Config decorator (%s) differs from current (%s), switching...", cfg_dec.c_str(), cur_dec.c_str());
        change_window_decorator(cfg_dec);
    }

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
        // Восстанавливаем из конфига
        if (config["indirect_rendering"] == "true") {
            indirect_rendering = true;
            item_indirect.set_active(true);
        }
        item_indirect.signal_toggled().connect([this]() {
            indirect_rendering = item_indirect.get_active();
            config["indirect_rendering"] = indirect_rendering ? "true" : "false";
            g_message("Indirect Rendering: %s", indirect_rendering ? "ON" : "OFF");
        });
        compiz_options_menu.append(item_indirect);

        item_loose.set_label("Loose Binding");
        if (config["loose_binding"] == "true") {
            loose_binding = true;
            item_loose.set_active(true);
        }
        item_loose.signal_toggled().connect([this]() {
            loose_binding = item_loose.get_active();
            config["loose_binding"] = loose_binding ? "true" : "false";
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

    // NVIDIA Options submenu
    if (nvidia_installed) {
        {
            auto* img = Gtk::manage(new Gtk::Image());
            Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_file(ICON_DIR "/nvidia.png");
            auto scaled = pixbuf->scale_simple(24, 24, Gdk::INTERP_BILINEAR);
            img->set(scaled);
            item_nvidia = Gtk::manage(new Gtk::ImageMenuItem(*img, "NVIDIA Options"));
        }
        item_nvidia->set_submenu(nvidia_menu);

        // Force Composition Pipeline
        auto* item_fcp = Gtk::manage(new Gtk::CheckMenuItem("Force Composition Pipeline"));
        item_fcp_ptr = item_fcp;
        item_fcp->signal_toggled().connect([this, item_fcp]() {
            config["force_composition_pipeline"] = item_fcp->get_active() ? "true" : "false";
            if (item_fcp->get_active()) {
                int r = std::system("nvidia-settings --assign 'CurrentMetaMode=DPY-1: nvidia-auto-select @1920x1080 +0+0 {ForceCompositionPipeline=On,ForceFullCompositionPipeline=Off}' > /dev/null 2>&1"); (void)r;
                FILE* p = popen("nvidia-settings -q CurrentMetaMode 2>/dev/null | grep -o 'ForceCompositionPipeline=[A-Za-z]*' | cut -d= -f2", "r");
                char buf[64] = {0};
                std::string state = "unknown";
                if (p && fgets(buf, sizeof(buf), p)) { buf[strcspn(buf, "\n")] = 0; state = buf; }
                if (p) pclose(p);
                g_message("Force Composition Pipeline... [%s]", state.c_str());
            } else {
                int r = std::system("nvidia-settings --assign 'CurrentMetaMode=DPY-1: nvidia-auto-select @1920x1080 +0+0 {ForceCompositionPipeline=Off,ForceFullCompositionPipeline=Off}' > /dev/null 2>&1"); (void)r;
                FILE* p = popen("nvidia-settings -q CurrentMetaMode 2>/dev/null | grep -o 'ForceCompositionPipeline=[A-Za-z]*' | cut -d= -f2", "r");
                char buf[64] = {0};
                std::string state = "unknown";
                if (p && fgets(buf, sizeof(buf), p)) { buf[strcspn(buf, "\n")] = 0; state = buf; }
                if (p) pclose(p);
                g_message("Force Composition Pipeline... [%s]", state.c_str());
            }
        });
        nvidia_menu.append(*item_fcp);

        // Force Full Composition Pipeline
        auto* item_ffcp = Gtk::manage(new Gtk::CheckMenuItem("Force Full Composition Pipeline"));
        item_ffcp_ptr = item_ffcp;
        item_ffcp->signal_toggled().connect([this, item_fcp, item_ffcp]() {
            config["force_full_composition_pipeline"] = item_ffcp->get_active() ? "true" : "false";
            if (item_ffcp->get_active()) {
                item_fcp->set_active(true);
                item_fcp->set_sensitive(false);
                int r = std::system("nvidia-settings --assign 'CurrentMetaMode=DPY-1: nvidia-auto-select @1920x1080 +0+0 {ForceCompositionPipeline=On,ForceFullCompositionPipeline=On}' > /dev/null 2>&1"); (void)r;
                FILE* p = popen("nvidia-settings -q CurrentMetaMode 2>/dev/null | grep -o 'ForceFullCompositionPipeline=[A-Za-z]*' | cut -d= -f2", "r");
                char buf[64] = {0};
                std::string state = "unknown";
                if (p && fgets(buf, sizeof(buf), p)) { buf[strcspn(buf, "\n")] = 0; state = buf; }
                if (p) pclose(p);
                g_message("Force Full Composition Pipeline... [%s]", state.c_str());
            } else {
                item_fcp->set_sensitive(true);
                int r = std::system("nvidia-settings --assign 'CurrentMetaMode=DPY-1: nvidia-auto-select @1920x1080 +0+0 {ForceCompositionPipeline=Off,ForceFullCompositionPipeline=Off}' > /dev/null 2>&1"); (void)r;
                FILE* p = popen("nvidia-settings -q CurrentMetaMode 2>/dev/null | grep -o 'ForceFullCompositionPipeline=[A-Za-z]*' | cut -d= -f2", "r");
                char buf[64] = {0};
                std::string state = "unknown";
                if (p && fgets(buf, sizeof(buf), p)) { buf[strcspn(buf, "\n")] = 0; state = buf; }
                if (p) pclose(p);
                g_message("Force Full Composition Pipeline... [%s]", state.c_str());
            }
        });
        nvidia_menu.append(*item_ffcp);

        // Детект текущего состояния пайплайнов
        {
            FILE* pipe = popen("nvidia-settings -q CurrentMetaMode 2>/dev/null", "r");
            if (pipe) {
                char buf[512] = {0};
                std::string output;
                while (fgets(buf, sizeof(buf), pipe))
                    output += buf;
                pclose(pipe);

                bool fcp_on = (output.find("ForceCompositionPipeline=On") != std::string::npos);
                bool ffcp_on = (output.find("ForceFullCompositionPipeline=On") != std::string::npos);

                if (ffcp_on) {
                    item_fcp->set_active(true);
                    item_fcp->set_sensitive(false);
                    item_ffcp->set_active(true);
                } else if (fcp_on) {
                    item_fcp->set_active(true);
                }
            }
        }

        menu.append(*item_nvidia);
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

    // Запускаем фоновый мониторинг — каждые 2 сек проверяем состояние
    last_known_wm = detect_window_manager();
    last_known_decorator = detect_window_decorator();
    Glib::signal_timeout().connect(sigc::mem_fun(*this, &FusionIcon::monitor_state), 2000);
    Glib::signal_timeout().connect(sigc::mem_fun(*this, &FusionIcon::reap_zombies), 1000);
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

    std::string cmd = "setsid " + wm + " --replace";
    if (wm == "compiz") {
        if (indirect_rendering) cmd += " --indirect-rendering";
        if (loose_binding) cmd += " --loose-binding";
    }
    cmd += " &";

    int ret = std::system(cmd.c_str());
    (void)ret;
    g_message("%s started", wm.c_str());
    active_monitoring = true;

    // Проверяем 4 раза по 500мс — если не запустился, повторяем
    std::string pgrep_cmd = "pgrep -x " + wm;
    std::string check_cmd = "for i in 1 2 3 4; do sleep 0.5; " + pgrep_cmd + " > /dev/null 2>&1 && exit 0; done; notify-send 'Fusion Icon 2' '" + wm + " failed to start, retrying...' && setsid " + wm + " --replace &";
    ret = std::system(("bash -c '" + check_cmd + "' &").c_str());
    (void)ret;
}

std::string FusionIcon::detect_desktop_name() {
    char* desktop_env = getenv("XDG_CURRENT_DESKTOP");
    return desktop_env ? std::string(desktop_env) : "Unknown";
}

std::string FusionIcon::detect_window_manager() {
    // Самый новый запущенный WM = активный
    char result[256] = {0};
    FILE* pipe = popen("ps -eo pid,lstart,comm 2>/dev/null | grep -E '[c]ompiz|[m]arco|[m]etacity' | sort -k2,6 | tail -1 | awk '{print $NF}'", "r");
    if (pipe) {
        if (fgets(result, sizeof(result), pipe)) {
            // Убираем перенос строки
            result[strcspn(result, "\n")] = 0;
            if (strlen(result) > 0) {
                pclose(pipe);
                return std::string(result);
            }
        }
        pclose(pipe);
    }
    // Fallback
    const char* wm_list[] = {"compiz", "marco", "metacity"};
    for (const char* wm : wm_list) {
        if (std::system(("pgrep -f " + std::string(wm) + " > /dev/null 2>&1").c_str()) == 0)
            return wm;
    }
    return "Unknown";
}

std::string FusionIcon::detect_window_decorator(const std::string& wm) {
    std::string current_wm = wm.empty() ? detect_window_manager() : wm;

    // Если WM не compiz — emerald не используется (он только для compiz)
    if (current_wm != "compiz") {
        if (std::system("pgrep -f gtk-window-decorator > /dev/null 2>&1") == 0)
            return "gtk-window-decorator";
        return "none";
    }

    // Для compiz проверяем кто запущен
    if (std::system("pgrep -x emerald > /dev/null 2>&1") == 0)
        return "emerald";
    if (std::system("pgrep -f gtk-window-decorator > /dev/null 2>&1") == 0)
        return "gtk-window-decorator";
    return "none";
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
        else if (current_wm == "marco") {
            Glib::RefPtr<Gdk::Pixbuf> pb = Gdk::Pixbuf::create_from_file(ICON_DIR "/marco.png");
            parent_img->set(pb->scale_simple(24, 24, Gdk::INTERP_BILINEAR));
        }
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
        if (wm == "marco") {
            Glib::RefPtr<Gdk::Pixbuf> pb = Gdk::Pixbuf::create_from_file(ICON_DIR "/marco.png");
            img->set(pb->scale_simple(24, 24, Gdk::INTERP_BILINEAR));
        } else
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
        if (current_decorator == "none" && dec == "gtk-window-decorator")
            label = "[ gtk-window-decorator ]";
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

bool FusionIcon::monitor_state() {
    std::string current_wm = detect_window_manager();
    std::string current_decorator = detect_window_decorator(current_wm);

    // Если что-то изменилось — обновляем подсветку
    if (current_wm != last_known_wm || current_decorator != last_known_decorator) {
        g_message("State changed: WM=%s Decorator=%s", current_wm.c_str(), current_decorator.c_str());
        last_known_wm = current_wm;
        last_known_decorator = current_decorator;
        active_monitoring = false; // Стабилизировались, обратно на медленный режим

        // Обновляем иконку родителя WM
        auto* parent_img = dynamic_cast<Gtk::Image*>(item_select_wm->get_image());
        if (parent_img) {
            if (current_wm == "compiz")
                parent_img->set_from_icon_name("compiz", Gtk::ICON_SIZE_MENU);
            else if (current_wm == "marco") {
                Glib::RefPtr<Gdk::Pixbuf> pb = Gdk::Pixbuf::create_from_file(ICON_DIR "/marco.png");
                parent_img->set(pb->scale_simple(24, 24, Gdk::INTERP_BILINEAR));
            }
            else if (current_wm == "metacity")
                parent_img->set_from_icon_name("metacity", Gtk::ICON_SIZE_MENU);
            else
                parent_img->set_from_icon_name("compiz", Gtk::ICON_SIZE_MENU);
        }

        // Пересоздаём пункты подменю
        rebuild_wm_menu();
        rebuild_decorator_menu();

        // Сохраняем только после подтверждения реального состояния
        config["selected_wm"] = current_wm;
        config["selected_decorator"] = current_decorator;
        save_config();
    }

    if (active_monitoring) {
        Glib::signal_timeout().connect_once([this]() { this->monitor_state(); }, 500);
        return false;
    }

    return true; // Продолжаем с текущим интервалом (2сек)
}

void FusionIcon::start_compiz_setsid() {
    if (std::system("pgrep -x compiz > /dev/null 2>&1") != 0) {
        g_message("Запускаем Compiz...");
        int result = std::system("setsid compiz --replace &");
        compiz_started_by_fusion_icon = true;
    }
}

bool FusionIcon::reap_zombies() {
    while (waitpid(-1, nullptr, WNOHANG) > 0) {}
    return true; // продолжаем таймер
}

void FusionIcon::signal_handler(int sig) {
    g_message("Caught signal: %d", sig);
    clean_exit();
}

void FusionIcon::clean_exit() {
    save_config();
    g_message("Exiting Fusion-icon-2...");
    Gtk::Main::quit();
}

void FusionIcon::save_config() {
    if (config_path.empty()) return;

    // Создаём директорию если нет
    std::string dir = config_path.substr(0, config_path.find_last_of('/'));
    int ret = std::system(("mkdir -p " + dir).c_str()); (void)ret;

    std::ofstream file(config_path);
    if (file.is_open()) {
        for (auto& pair : config) {
            file << pair.first << "=" << pair.second << "\n";
        }
        file.close();
        g_message("Config saved: %s", config_path.c_str());
    }
}

void FusionIcon::load_config() {
    if (config_path.empty()) return;

    std::ifstream file(config_path);
    if (file.is_open()) {
        std::string line;
        int line_num = 0;
        while (std::getline(file, line)) {
            line_num++;
            if (line.empty() || line[0] == '#') continue;
            size_t eq = line.find('=');
            if (eq == std::string::npos || eq == 0) {
                g_warning("Corrupted config at line %d, recreating...", line_num);
                file.close();
                std::remove(config_path.c_str());
                config.clear();
                return;
            }
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if (key.find_first_not_of("abcdefghijklmnopqrstuvwxyz_") != std::string::npos) {
                g_warning("Corrupted config at line %d, recreating...", line_num);
                file.close();
                std::remove(config_path.c_str());
                config.clear();
                return;
            }
            config[key] = val;
        }
        file.close();
        g_message("Config loaded: %s", config_path.c_str());
    }

    // Дефолты из текущего состояния если ключей нет
    if (config.find("selected_wm") == config.end())
        config["selected_wm"] = detect_window_manager();
    if (config.find("selected_decorator") == config.end())
        config["selected_decorator"] = detect_window_decorator();
    if (config.find("indirect_rendering") == config.end())
        config["indirect_rendering"] = "false";
    if (config.find("loose_binding") == config.end())
        config["loose_binding"] = "false";
    if (config.find("force_composition_pipeline") == config.end())
        config["force_composition_pipeline"] = "false";
    if (config.find("force_full_composition_pipeline") == config.end())
        config["force_full_composition_pipeline"] = "false";
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
    } else {
        // Выводим информацию о GPU в debug режиме
        std::string gpu_info;
        FILE* gpu_pipe = popen("lspci | grep -i 'vga\\|3d' | head -1", "r");
        if (gpu_pipe) {
            char buf[256] = {0};
            if (fgets(buf, sizeof(buf), gpu_pipe)) {
                buf[strcspn(buf, "\n")] = 0;
                // Извлекаем только модель: "NVIDIA Corporation GM204 [GeForce GTX 970]"
                char* bracket = strchr(buf, '[');
                char* rev = strstr(buf, " (rev");
                if (bracket) {
                    char* end = strchr(bracket, ']');
                    if (end) {
                        *end = 0;
                        gpu_info = std::string(bracket + 1);
                    }
                }
            }
            pclose(gpu_pipe);
        }
        FILE* drv_pipe = popen("nvidia-smi --query-gpu=driver_version --format=csv,noheader 2>/dev/null | head -1", "r");
        if (drv_pipe) {
            char drv[64] = {0};
            if (fgets(drv, sizeof(drv), drv_pipe)) {
                drv[strcspn(drv, "\n")] = 0;
                if (strlen(drv) > 0 && !gpu_info.empty())
                    g_message("%s, %s", gpu_info.c_str(), drv);
                else if (!gpu_info.empty())
                    g_message("%s", gpu_info.c_str());
            }
            pclose(drv_pipe);
        }
    }

    Gtk::Main kit(argc, argv);
    FusionIcon fusionIcon;
    Gtk::Main::run();
}
