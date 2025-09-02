#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

#include <glib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Structure to hold translations for a language
typedef struct {
    const char* language_code;
    const char* welcome_title;
    const char* welcome_subtitle;
    const char* theme_title;
    const char* theme_subtitle;
    const char* network_title;
    const char* network_subtitle;
    const char* keybinds_title;
    const char* keybinds_subtitle;
    const char* updater_title;
    const char* updater_subtitle;
    const char* settings_title;
    const char* settings_subtitle;
    const char* store_title;
    const char* store_subtitle;
    const char* complete_title;
    const char* support_button;
    const char* discord_button;
    const char* website_button;
    const char* close_button;
    const char* skip_button;
    const char* back_button;
    const char* next_button;
    const char* finish_button;
    const char* wifi_label;
    const char* refresh_button_tooltip;
    const char* enable_networking_button;
    const char* networking_disabled_message;
    const char* ethernet_connected_message;
    const char* wifi_hardware_disabled_message;
    const char* wifi_disabled_message;
    const char* no_wifi_device_message;
    const char* no_networks_found_message;
    const char* nm_not_available_message;
    const char* password_dialog_title;
    const char* password_dialog_prompt;
    const char* connect_button;
    const char* cancel_button;
    const char* connected_status;
    const char* saved_status;
    const char* secured_status;
    
    // Keybind translations
    const char* keybind_close_window;
    const char* keybind_app_manager;
    const char* keybind_terminal;
    const char* keybind_workspace_switcher;
    const char* keybind_change_language;
    const char* keybind_lock_screen;
    const char* keybind_powermenu;
    const char* keybind_switch_workspaces;
    const char* keybind_workspaces_viewer;
    const char* keybind_notification;
    const char* keybind_system_info;
    const char* keybind_wallpapers_menu;
    const char* keybind_exit_hyprland;
    const char* keybind_toggle_float;
    const char* keybind_launch_editor;
    const char* keybind_launch_file_manager;
    const char* keybind_launch_browser;
    const char* keybind_full_screenshot;
    const char* keybind_region_screenshot;
    const char* keybind_mute_volume;
    const char* keybind_lower_brightness;
    const char* keybind_higher_brightness;
    const char* keybind_lower_volume;
    const char* keybind_higher_volume;
    const char* keybind_mute_microphone;
} Translations;

// English translations
static const Translations en_translations = {
    "en",
    "Welcome",
    "Welcome to ElysiaOS",
    "Choose Your Style",
    "Pick a theme that matches your preference",
    "Connect to Internet",
    "Choose a network to get online",
    "ElysiaOS Keybinds",
    "Quick reference for system shortcuts",
    "Elysia Updater",
    "Update ElysiaOS updates smoothly",
    "Elysia Settings",
    "Smooth and Improved Settings app for the ElysiaOS environment",
    "App Store",
    "Easy download and install apps without hassle",
    "Setup is Finished!",
    "Support",
    "Discord",
    "Website",
    "Close",
    "Skip Setup",
    "Back",
    "Next",
    "Finish",
    "Wi-Fi",
    "Refresh Networks",
    "Enable Networking",
    "Networking is disabled. Enable networking to connect to Wi-Fi.",
    "You are already connected to the internet via Ethernet",
    "Wi-Fi hardware is disabled",
    "Wi-Fi is disabled. Enable Wi-Fi to see networks.",
    "No Wi-Fi device found",
    "No networks found. Try refresh.",
    "NetworkManager client not available",
    "Connect to Wi-Fi",
    "Enter password for \"%s\":",
    "Connect",
    "Cancel",
    "Connected",
    "Saved",
    "Secured",
    
    // Keybind translations
    "Close focused window",
    "Launch Application manager",
    "Terminal",
    "Brings up the workspaces switcher",
    "Change Language",
    "Lock your screen Hyprlock",
    "Powermenu",
    "Switch workspaces",
    "Workspaces viewer Hyprspace",
    "Opens Swaync Notification",
    "EWW Widget for system info",
    "Launches Wallpapers menu",
    "Exit Hyprland altogether",
    "Toggle float a window",
    "Launch text editor VSCODE",
    "Launch File manager Thunar",
    "Launch Floorp Browser",
    "Take a full screenshot",
    "Take a region screenshot",
    "MUTE Volume",
    "Lower Brightness",
    "Higher Brightness",
    "Lower Volume",
    "Higher Volume",
    "MUTE Microphone"
};

// French translations
static const Translations fr_translations = {
    "fr",
    "Bienvenue",
    "Bienvenue dans ElysiaOS",
    "Choisissez votre style",
    "Sélectionnez un thème qui correspond à vos préférences",
    "Se connecter à Internet",
    "Choisissez un réseau pour vous connecter en ligne",
    "Raccourcis ElysiaOS",
    "Référence rapide pour les raccourcis système",
    "Mise à jour Elysia",
    "Mettez à jour ElysiaOS en douceur",
    "Paramètres Elysia",
    "Application de paramètres fluide et améliorée pour l'environnement ElysiaOS",
    "Magasin d'applications",
    "Téléchargez et installez facilement des applications sans tracas",
    "La configuration est terminée !",
    "Support",
    "Discord",
    "Site web",
    "Fermer",
    "Ignorer la configuration",
    "Retour",
    "Suivant",
    "Terminer",
    "Wi-Fi",
    "Actualiser les réseaux",
    "Activer la mise en réseau",
    "La mise en réseau est désactivée. Activez la mise en réseau pour vous connecter au Wi-Fi.",
    "Vous êtes déjà connecté à Internet via Ethernet",
    "Le matériel Wi-Fi est désactivé",
    "Le Wi-Fi est désactivé. Activez le Wi-Fi pour voir les réseaux.",
    "Aucun périphérique Wi-Fi trouvé",
    "Aucun réseau trouvé. Essayez d'actualiser.",
    "Client NetworkManager non disponible",
    "Se connecter au Wi-Fi",
    "Entrez le mot de passe pour \"%s\" :",
    "Se connecter",
    "Annuler",
    "Connecté",
    "Enregistré",
    "Sécurisé",
    
    // Keybind translations
    "Fermer la fenêtre active",
    "Lancer le gestionnaire d'applications",
    "Terminal",
    "Ouvre le sélecteur d'espaces de travail",
    "Changer de langue",
    "Verrouiller votre écran Hyprlock",
    "Menu d'alimentation",
    "Changer d'espace de travail",
    "Visualiseur d'espaces de travail Hyprspace",
    "Ouvre les notifications Swaync",
    "Widget EWW pour les informations système",
    "Lance le menu des fonds d'écran",
    "Quitter Hyprland complètement",
    "Basculer le flottement d'une fenêtre",
    "Lancer l'éditeur de texte VSCODE",
    "Lancer le gestionnaire de fichiers Thunar",
    "Lancer le navigateur Floorp",
    "Prendre une capture d'écran complète",
    "Prendre une capture d'écran de région",
    "COUPER le volume",
    "Diminuer la luminosité",
    "Augmenter la luminosité",
    "Diminuer le volume",
    "Augmenter le volume",
    "COUPER le microphone"
};

// Spanish translations
static const Translations es_translations = {
    "es",
    "Bienvenido",
    "Bienvenido a ElysiaOS",
    "Elige tu estilo",
    "Selecciona un tema que coincida con tus preferencias",
    "Conectar a Internet",
    "Elige una red para conectarte en línea",
    "Atajos de ElysiaOS",
    "Referencia rápida para atajos del sistema",
    "Actualizador de Elysia",
    "Actualiza ElysiaOS sin problemas",
    "Configuración de Elysia",
    "Aplicación de configuración fluida y mejorada para el entorno ElysiaOS",
    "Tienda de aplicaciones",
    "Descarga e instala aplicaciones fácilmente sin complicaciones",
    "¡La configuración ha terminado!",
    "Soporte",
    "Discord",
    "Sitio web",
    "Cerrar",
    "Omitir configuración",
    "Atrás",
    "Siguiente",
    "Finalizar",
    "Wi-Fi",
    "Actualizar redes",
    "Habilitar red",
    "La red está deshabilitada. Habilita la red para conectarte al Wi-Fi.",
    "Ya estás conectado a Internet vía Ethernet",
    "El hardware Wi-Fi está deshabilitado",
    "El Wi-Fi está deshabilitado. Habilita el Wi-Fi para ver las redes.",
    "No se encontró dispositivo Wi-Fi",
    "No se encontraron redes. Intenta actualizar.",
    "Cliente NetworkManager no disponible",
    "Conectar al Wi-Fi",
    "Ingresa la contraseña para \"%s\":",
    "Conectar",
    "Cancelar",
    "Conectado",
    "Guardado",
    "Protegido",
    
    // Keybind translations
    "Cerrar ventana enfocada",
    "Lanzar gestor de aplicaciones",
    "Terminal",
    "Abre el selector de espacios de trabajo",
    "Cambiar idioma",
    "Bloquear tu pantalla Hyprlock",
    "Menú de energía",
    "Cambiar espacios de trabajo",
    "Visor de espacios de trabajo Hyprspace",
    "Abre notificaciones Swaync",
    "Widget EWW para información del sistema",
    "Lanza menú de fondos de pantalla",
    "Salir de Hyprland completamente",
    "Alternar flotación de ventana",
    "Lanzar editor de texto VSCODE",
    "Lanzar gestor de archivos Thunar",
    "Lanzar navegador Floorp",
    "Tomar captura de pantalla completa",
    "Tomar captura de región",
    "SILENCIAR volumen",
    "Disminuir brillo",
    "Aumentar brillo",
    "Disminuir volumen",
    "Aumentar volumen",
    "SILENCIAR micrófono"
};

// Russian translations
static const Translations ru_translations = {
    "ru",
    "Добро пожаловать",
    "Добро пожаловать в ElysiaOS",
    "Выберите свой стиль",
    "Выберите тему, которая соответствует вашим предпочтениям",
    "Подключение к Интернету",
    "Выберите сеть для подключения к Интернету",
    "Горячие клавиши ElysiaOS",
    "Краткий справочник по системным горячим клавишам",
    "Обновление Elysia",
    "Обновляйте ElysiaOS без проблем",
    "Настройки Elysia",
    "Плавное и улучшенное приложение настроек для среды ElysiaOS",
    "Магазин приложений",
    "Легко скачивайте и устанавливайте приложения без хлопот",
    "Настройка завершена!",
    "Поддержка",
    "Discord",
    "Веб-сайт",
    "Закрыть",
    "Пропустить настройку",
    "Назад",
    "Далее",
    "Завершить",
    "Wi-Fi",
    "Обновить сети",
    "Включить сеть",
    "Сеть отключена. Включите сеть для подключения к Wi-Fi.",
    "Вы уже подключены к Интернету через Ethernet",
    "Оборудование Wi-Fi отключено",
    "Wi-Fi отключен. Включите Wi-Fi, чтобы увидеть сети.",
    "Устройство Wi-Fi не найдено",
    "Сети не найдены. Попробуйте обновить.",
    "Клиент NetworkManager недоступен",
    "Подключиться к Wi-Fi",
    "Введите пароль для \"%s\":",
    "Подключить",
    "Отменить",
    "Подключено",
    "Сохранено",
    "Защищено",
    
    // Keybind translations
    "Закрыть активное окно",
    "Запустить менеджер приложений",
    "Терминал",
    "Открывает переключатель рабочих областей",
    "Изменить язык",
    "Заблокировать экран Hyprlock",
    "Меню питания",
    "Переключить рабочие области",
    "Просмотрщик рабочих областей Hyprspace",
    "Открывает уведомления Swaync",
    "Виджет EWW для системной информации",
    "Запускает меню обоев",
    "Полностью выйти из Hyprland",
    "Переключить плавающий режим окна",
    "Запустить текстовый редактор VSCODE",
    "Запустить файловый менеджер Thunar",
    "Запустить браузер Floorp",
    "Сделать полный скриншот",
    "Сделать скриншот области",
    "ВЫКЛЮЧИТЬ звук",
    "Уменьшить яркость",
    "Увеличить яркость",
    "Уменьшить громкость",
    "Увеличить громкость",
    "ВЫКЛЮЧИТЬ микрофон"
};

// Vietnamese translations
static const Translations vi_translations = {
    "vi",
    "Chào mừng",
    "Chào mừng bạn đến với ElysiaOS",
    "Chọn phong cách của bạn",
    "Chọn một chủ đề phù hợp với sở thích của bạn",
    "Kết nối Internet",
    "Chọn một mạng để kết nối trực tuyến",
    "Phím tắt ElysiaOS",
    "Tham khảo nhanh cho các phím tắt hệ thống",
    "Cập nhật Elysia",
    "Cập nhật ElysiaOS một cách mượt mà",
    "Cài đặt Elysia",
    "Ứng dụng cài đặt mượt mà và cải tiến cho môi trường ElysiaOS",
    "Cửa hàng ứng dụng",
    "Tải xuống và cài đặt ứng dụng dễ dàng không gặp rắc rối",
    "Thiết lập đã hoàn tất!",
    "Hỗ trợ",
    "Discord",
    "Trang web",
    "Đóng",
    "Bỏ qua thiết lập",
    "Quay lại",
    "Tiếp theo",
    "Hoàn thành",
    "Wi-Fi",
    "Làm mới mạng",
    "Bật mạng",
    "Mạng đã bị tắt. Bật mạng để kết nối Wi-Fi.",
    "Bạn đã kết nối Internet qua Ethernet",
    "Phần cứng Wi-Fi đã bị tắt",
    "Wi-Fi đã bị tắt. Bật Wi-Fi để xem các mạng.",
    "Không tìm thấy thiết bị Wi-Fi",
    "Không tìm thấy mạng nào. Hãy thử làm mới.",
    "Ứng dụng khách NetworkManager không khả dụng",
    "Kết nối Wi-Fi",
    "Nhập mật khẩu cho \"%s\":",
    "Kết nối",
    "Hủy",
    "Đã kết nối",
    "Đã lưu",
    "Đã bảo mật",
    
    // Keybind translations
    "Đóng cửa sổ đang tập trung",
    "Khởi chạy trình quản lý ứng dụng",
    "Terminal",
    "Hiển thị bộ chuyển đổi không gian làm việc",
    "Thay đổi ngôn ngữ",
    "Khóa màn hình Hyprlock",
    "Menu nguồn",
    "Chuyển đổi không gian làm việc",
    "Trình xem không gian làm việc Hyprspace",
    "Mở thông báo Swaync",
    "Widget EWW cho thông tin hệ thống",
    "Khởi chạy menu hình nền",
    "Thoát Hyprland hoàn toàn",
    "Chuyển đổi chế độ nổi của cửa sổ",
    "Khởi chạy trình soạn thảo văn bản VSCODE",
    "Khởi chạy trình quản lý tệp Thunar",
    "Khởi chạy trình duyệt Floorp",
    "Chụp ảnh màn hình toàn bộ",
    "Chụp ảnh màn hình vùng",
    "TẮT TIẾNG âm lượng",
    "Giảm độ sáng",
    "Tăng độ sáng",
    "Giảm âm lượng",
    "Tăng âm lượng",
    "TẮT TIẾNG microphone"
};

// Indonesian translations
static const Translations id_translations = {
    "id",
    "Selamat datang",
    "Selamat datang di ElysiaOS",
    "Pilih gaya Anda",
    "Pilih tema yang sesuai dengan preferensi Anda",
    "Hubungkan ke Internet",
    "Pilih jaringan untuk terhubung online",
    "Pintasan ElysiaOS",
    "Referensi cepat untuk pintasan sistem",
    "Pembaruan Elysia",
    "Perbarui ElysiaOS dengan lancar",
    "Pengaturan Elysia",
    "Aplikasi pengaturan yang lancar dan ditingkatkan untuk lingkungan ElysiaOS",
    "Toko Aplikasi",
    "Unduh dan instal aplikasi dengan mudah tanpa kerepotan",
    "Pengaturan selesai!",
    "Dukungan",
    "Discord",
    "Situs web",
    "Tutup",
    "Lewati pengaturan",
    "Kembali",
    "Selanjutnya",
    "Selesai",
    "Wi-Fi",
    "Segarkan jaringan",
    "Aktifkan jaringan",
    "Jaringan dinonaktifkan. Aktifkan jaringan untuk terhubung ke Wi-Fi.",
    "Anda sudah terhubung ke internet melalui Ethernet",
    "Perangkat keras Wi-Fi dinonaktifkan",
    "Wi-Fi dinonaktifkan. Aktifkan Wi-Fi untuk melihat jaringan.",
    "Perangkat Wi-Fi tidak ditemukan",
    "Tidak ada jaringan yang ditemukan. Coba segarkan.",
    "Klien NetworkManager tidak tersedia",
    "Hubungkan ke Wi-Fi",
    "Masukkan kata sandi untuk \"%s\":",
    "Hubungkan",
    "Batal",
    "Terhubung",
    "Tersimpan",
    "Diamankan",
    
    // Keybind translations
    "Tutup jendela yang difokuskan",
    "Luncurkan manajer aplikasi",
    "Terminal",
    "Memunculkan pengalih ruang kerja",
    "Ubah bahasa",
    "Kunci layar Hyprlock",
    "Menu daya",
    "Beralih ruang kerja",
    "Penampil ruang kerja Hyprspace",
    "Buka notifikasi Swaync",
    "Widget EWW untuk info sistem",
    "Luncurkan menu wallpaper",
    "Keluar dari Hyprland sepenuhnya",
    "Alihkan mengambang jendela",
    "Luncurkan editor teks VSCODE",
    "Luncurkan manajer file Thunar",
    "Luncurkan browser Floorp",
    "Ambil tangkapan layar penuh",
    "Ambil tangkapan layar wilayah",
    "BISU volume",
    "Turunkan kecerahan",
    "Naikkan kecerahan",
    "Turunkan volume",
    "Naikkan volume",
    "BISU mikrofon"
};

// Japanese translations
static const Translations ja_translations = {
    "ja",
    "ようこそ",
    "ElysiaOSへようこそ",
    "スタイルを選択",
    "好みに合ったテーマを選んでください",
    "インターネットに接続",
    "ネットワークを選択してオンラインに接続",
    "ElysiaOS キーバインド",
    "システムショートカットのクイックリファレンス",
    "Elysia アップデーター",
    "ElysiaOSをスムーズに更新",
    "Elysia 設定",
    "ElysiaOS環境のためのスムーズで改良された設定アプリ",
    "アプリストア",
    "面倒なく簡単にアプリをダウンロードしてインストール",
    "セットアップが完了しました！",
    "サポート",
    "Discord",
    "ウェブサイト",
    "閉じる",
    "セットアップをスキップ",
    "戻る",
    "次へ",
    "完了",
    "Wi-Fi",
    "ネットワークを更新",
    "ネットワークを有効化",
    "ネットワークが無効です。ネットワークを有効化してWi-Fiに接続してください。",
    "イーサネット経由で既にインターネットに接続しています",
    "Wi-Fiハードウェアが無効です",
    "Wi-Fiが無効です。Wi-Fiを有効化してネットワークを表示してください。",
    "Wi-Fiデバイスが見つかりません",
    "ネットワークが見つかりません。更新してみてください。",
    "NetworkManagerクライアントが利用できません",
    "Wi-Fiに接続",
    "\"%s\"のパスワードを入力:",
    "接続",
    "キャンセル",
    "接続済み",
    "保存済み",
    "保護済み",
    
    // Keybind translations
    "フォーカスされたウィンドウを閉じる",
    "アプリケーションマネージャーを起動",
    "ターミナル",
    "ワークスペーススイッチャーを表示",
    "言語を変更",
    "画面ロック (Hyprlock)",
    "パワーメニュー",
    "ワークスペースを切り替え",
    "ワークスペースビューア (Hyprspace)",
    "通知を開く (Swaync)",
    "システム情報ウィジェット (EWW)",
    "壁紙メニューを起動",
    "Hyprlandを終了",
    "ウィンドウのフロートを切り替え",
    "テキストエディタ (VSCODE) を起動",
    "ファイルマネージャー (Thunar) を起動",
    "ブラウザ (Floorp) を起動",
    "フルスクリーンショットを撮影",
    "範囲スクリーンショットを撮影",
    "音量をミュート",
    "明るさを下げる",
    "明るさを上げる",
    "音量を下げる",
    "音量を上げる",
    "マイクをミュート"
};

// Chinese translations
static const Translations zh_translations = {
    "zh",
    "欢迎",
    "欢迎使用 ElysiaOS",
    "选择您的风格",
    "选择符合您喜好的主题",
    "连接到互联网",
    "选择网络以连接到互联网",
    "ElysiaOS 快捷键",
    "系统快捷键快速参考",
    "Elysia 更新器",
    "平滑更新 ElysiaOS",
    "Elysia 设置",
    "为 ElysiaOS 环境提供流畅和改进的设置应用程序",
    "应用商店",
    "轻松下载和安装应用程序，无后顾之忧",
    "设置完成！",
    "支持",
    "Discord",
    "网站",
    "关闭",
    "跳过设置",
    "返回",
    "下一步",
    "完成",
    "Wi-Fi",
    "刷新网络",
    "启用网络",
    "网络已禁用。启用网络以连接到Wi-Fi。",
    "您已通过以太网连接到互联网",
    "Wi-Fi硬件已禁用",
    "Wi-Fi已禁用。启用Wi-Fi以查看网络。",
    "未找到Wi-Fi设备",
    "未找到网络。请尝试刷新。",
    "NetworkManager客户端不可用",
    "连接到Wi-Fi",
    "输入\"%s\"的密码：",
    "连接",
    "取消",
    "已连接",
    "已保存",
    "已保护",
    
    // Keybind translations
    "关闭焦点窗口",
    "启动应用程序管理器",
    "终端",
    "显示工作区切换器",
    "更改语言",
    "锁定屏幕 (Hyprlock)",
    "电源菜单",
    "切换工作区",
    "工作区查看器 (Hyprspace)",
    "打开通知 (Swaync)",
    "系统信息小部件 (EWW)",
    "启动壁纸菜单",
    "完全退出Hyprland",
    "切换窗口浮动",
    "启动文本编辑器 (VSCODE)",
    "启动文件管理器 (Thunar)",
    "启动浏览器 (Floorp)",
    "截取全屏截图",
    "截取区域截图",
    "静音音量",
    "降低亮度",
    "提高亮度",
    "降低音量",
    "提高音量",
    "静音麦克风"
};

// Function to get system language from LANG environment variable
static char* get_system_language() {
    // Get the LANG environment variable
    const char* lang_env = getenv("LANG");
    if (lang_env == NULL || strlen(lang_env) < 2) {
        // Default to English if LANG is not set or too short
        return strdup("en");
    }
    
    // Extract the first two characters as the language code
    char* lang_code = (char*)malloc(3);
    strncpy(lang_code, lang_env, 2);
    lang_code[2] = '\0';
    
    return lang_code;
}

// Function to get translations for current system language
static const Translations* get_translations() {
    char* system_lang = get_system_language();
    
    // Try to find matching translation
    if (strcmp(system_lang, "fr") == 0) {
        free(system_lang);
        return &fr_translations;
    } else if (strcmp(system_lang, "es") == 0) {
        free(system_lang);
        return &es_translations;
    } else if (strcmp(system_lang, "ru") == 0) {
        free(system_lang);
        return &ru_translations;
    } else if (strcmp(system_lang, "vi") == 0) {
        free(system_lang);
        return &vi_translations;
    } else if (strcmp(system_lang, "id") == 0) {
        free(system_lang);
        return &id_translations;
    } else if (strcmp(system_lang, "ja") == 0) {
        free(system_lang);
        return &ja_translations;
    } else if (strcmp(system_lang, "zh") == 0) {
        free(system_lang);
        return &zh_translations;
    } else {
        // Default to English for any other language
        free(system_lang);
        return &en_translations;
    }
}

#endif // TRANSLATIONS_H