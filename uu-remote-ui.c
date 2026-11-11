/*
 * uu-remote-ui — UU 远程 Linux 统一启动器
 *
 * 一个窗口里同时提供：
 *   · 我的设备   —— 本机设备 ID / 账号 / 是否允许他人远程协助
 *   · 远程协助   —— 打开被控端（主机）窗口，看设备 ID 与验证码
 *   · 开始协助   —— 打开控制端，连接别人的设备
 *   · 扫码登录   —— 登录 / 退出 UU 账号
 *
 * 界面仿照官方 UU 远程客户端：左侧导航 + 右侧卡片，蓝色主色。
 * 只用 GTK3，不依赖 zenity / kdialog。
 */
#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREFIX      "/opt/uu-remote-linux"
#define CONFIG_PATH "$HOME/.config/uuremote/config.json"

/* ---------------------------------------------------------------- 工具 */

static char *
read_home(const char *path)
{
	return g_build_filename(g_get_home_dir(), path, NULL);
}

/* 从 config.json 里取一个字符串字段，取不到返回 NULL */
static gchar *
config_string(const char *key)
{
	gchar *path = read_home(".config/uuremote/config.json");
	gchar *content = NULL;
	gchar *value = NULL;

	if (!g_file_get_contents(path, &content, NULL, NULL)) {
		g_free(path);
		return NULL;
	}
	g_free(path);

	gchar *needle = g_strdup_printf("\"%s\"", key);
	gchar *pos = strstr(content, needle);
	if (pos) {
		pos += strlen(needle);
		while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r')
			pos++;
		if (*pos == ':') {
			pos++;
			while (*pos == ' ' || *pos == '\t')
				pos++;
			if (*pos == '"') {
				pos++;
				const char *end = strchr(pos, '"');
				if (end)
					value = g_strndup(pos, end - pos);
			}
		}
	}
	g_free(needle);
	g_free(content);
	return value;
}

static gboolean
config_bool(const char *key, gboolean fallback)
{
	gchar *path = read_home(".config/uuremote/config.json");
	gchar *content = NULL;
	gboolean result = fallback;

	if (!g_file_get_contents(path, &content, NULL, NULL)) {
		g_free(path);
		return fallback;
	}
	g_free(path);

	gchar *needle = g_strdup_printf("\"%s\"", key);
	gchar *pos = strstr(content, needle);
	if (pos) {
		pos += strlen(needle);
		while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r')
			pos++;
		if (*pos == ':') {
			pos++;
			while (*pos == ' ' || *pos == '\t')
				pos++;
			if (g_str_has_prefix(pos, "true"))
				result = TRUE;
			else if (g_str_has_prefix(pos, "false"))
				result = FALSE;
		}
	}
	g_free(needle);
	g_free(content);
	return result;
}

static void
spawn_program(const char *const *argv)
{
	GError *error = NULL;
	if (!g_spawn_async(NULL, (gchar **)argv, NULL,
			   G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
			   NULL, NULL, NULL, &error)) {
		g_printerr("uu-remote-ui: 无法启动 %s: %s\n", argv[0], error->message);
		g_error_free(error);
	}
}

static void
spawn_host(void)     { const char *a[] = { PREFIX "/bin/uuremote-server", "show", NULL }; spawn_program(a); }
static void
spawn_viewer(void)   { const char *a[] = { PREFIX "/bin/uuremote-viewer", "ui", NULL }; spawn_program(a); }
static void
spawn_login(void)    { const char *a[] = { PREFIX "/bin/uuremote-server", "login", NULL }; spawn_program(a); }

/* ---------------------------------------------------------------- 样式 */

static const char *APP_CSS =
"window { background: #ffffff; }\n"
"#sidebar { background: #f5f6f7; }\n"
"#header { background: #ffffff; border-bottom: 1px solid #ebedf0; }\n"
"#logo { background: #1a73e8; color: #ffffff; font-size: 15px; font-weight: 700; }\n"
"#appname { font-size: 16px; font-weight: 700; color: #1f2329; }\n"
"#appsub { font-size: 11px; color: #8a9099; }\n"
"navitem { color: #5a6069; font-size: 13px; padding: 9px 12px; border-radius: 6px; }\n"
"navitem:hover { background: #e9ebee; color: #1f2329; }\n"
"navitem.active { background: #e6efff; color: #1a73e8; font-weight: 600; }\n"
"#pagetitle { font-size: 17px; font-weight: 700; color: #1f2329; }\n"
"#card { background: #ffffff; border: 1px solid #ebedf0; border-radius: 10px; }\n"
"#cardtitle { font-size: 13px; font-weight: 600; color: #1f2329; }\n"
"#label { font-size: 12px; color: #8a9099; }\n"
"#value { font-size: 13px; color: #1f2329; font-family: monospace; }\n"
"#valueplain { font-size: 13px; color: #1f2329; }\n"
"#hint { font-size: 12px; color: #8a9099; }\n"
"button.primary { background: #1a73e8; color: #ffffff; border: none; border-radius: 6px; padding: 8px 18px; font-size: 13px; font-weight: 600; }\n"
"button.primary:hover { background: #1765cc; }\n"
"button.secondary { background: #ffffff; color: #1a73e8; border: 1px solid #c6d9fb; border-radius: 6px; padding: 8px 18px; font-size: 13px; font-weight: 600; }\n"
"button.secondary:hover { background: #f2f7ff; }\n"
"button.flat { background: transparent; color: #1a73e8; border: none; font-size: 12px; padding: 4px 8px; }\n"
"button.flat:hover { background: #e6efff; border-radius: 5px; }\n"
"#stateon { color: #1a9c4e; font-size: 12px; font-weight: 600; }\n"
"#stateoff { color: #c0392b; font-size: 12px; font-weight: 600; }\n";

/* ---------------------------------------------------------------- 页面 */

typedef struct {
	GtkWidget *stack;
	GtkWidget *nav[4];
	GtkWidget *device_value;
	GtkWidget *account_value;
	GtkWidget *assist_value;
} App;

static void
nav_select(App *app, int index)
{
	for (int i = 0; i < 4; i++) {
		if (i == index)
			gtk_style_context_add_class(gtk_widget_get_style_context(app->nav[i]), "active");
		else
			gtk_style_context_remove_class(gtk_widget_get_style_context(app->nav[i]), "active");
	}
	gtk_stack_set_visible_child_name(GTK_STACK(app->stack), index == 0 ? "devices" :
					 index == 1 ? "assist" : index == 2 ? "start" : "login");
}

static GtkWidget *
nav_button(App *app, const char *label, int index)
{
	GtkWidget *btn = gtk_button_new_with_label(label);
	gtk_widget_set_name(btn, "navitem");
	gtk_style_context_add_class(gtk_widget_get_style_context(btn), "navitem");
	gtk_widget_set_size_request(btn, -1, 36);
	gtk_widget_set_halign(btn, GTK_ALIGN_FILL);
	g_object_set_data(G_OBJECT(btn), "index", GINT_TO_POINTER(index + 1));
	app->nav[index] = btn;
	return btn;
}

/* nav_button 的 clicked 回调需要 index，单独包一层 */
static void
nav_clicked(GtkButton *btn, gpointer data)
{
	App *app = data;
	gint index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "index")) - 1;
	nav_select(app, index);
}

static gboolean
reset_copy_label(gpointer data)
{
	gtk_button_set_label(GTK_BUTTON(data), "复制");
	return G_SOURCE_REMOVE;
}

static void
copy_device_id(GtkButton *btn, gpointer data)
{
	const gchar *id = (const gchar *)data;
	GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(clip, id, -1);
	gtk_button_set_label(btn, "已复制");
	g_timeout_add(1200, reset_copy_label, btn);
}

/* ---------------------------------------------------------------- 构建 */

static GtkWidget *
card_new(void)
{
	GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_name(card, "card");
	gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");
	gtk_widget_set_margin_start(card, 16);
	gtk_widget_set_margin_end(card, 16);
	gtk_widget_set_margin_top(card, 12);
	gtk_widget_set_margin_bottom(card, 14);
	return card;
}

static GtkWidget *
row_new(const char *label, GtkWidget *value_widget)
{
	GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	GtkWidget *lab = gtk_label_new(label);
	gtk_widget_set_name(lab, "label");
	gtk_widget_set_size_request(lab, 76, -1);
	gtk_widget_set_halign(lab, GTK_ALIGN_START);
	gtk_widget_set_valign(value_widget, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(row), lab, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(row), value_widget, TRUE, TRUE, 0);
	return row;
}

static GtkWidget *
make_button(const char *label, const char *style, GCallback cb, gpointer data)
{
	GtkWidget *btn = gtk_button_new_with_label(label);
	gtk_style_context_add_class(gtk_widget_get_style_context(btn), style);
	if (cb)
		g_signal_connect(btn, "clicked", cb, data);
	return btn;
}

static GtkWidget *
build_page_devices(App *app)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
	gtk_widget_set_margin_start(page, 22);
	gtk_widget_set_margin_end(page, 22);
	gtk_widget_set_margin_top(page, 20);

	GtkWidget *title = gtk_label_new("我的设备");
	gtk_widget_set_name(title, "pagetitle");
	gtk_widget_set_halign(title, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(page), title, FALSE, FALSE, 0);

	/* 设备 ID 卡片 */
	GtkWidget *card = card_new();
	GtkWidget *ct = gtk_label_new("本设备");
	gtk_widget_set_name(ct, "cardtitle");
	gtk_widget_set_halign(ct, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(card), ct, FALSE, FALSE, 0);

	gchar *device_id = config_string("device_id");
	if (!device_id)
		device_id = g_strdup("未注册");
	GtkWidget *id_value = gtk_label_new(device_id);
	gtk_widget_set_name(id_value, "value");
	gtk_widget_set_halign(id_value, GTK_ALIGN_START);
	gtk_label_set_selectable(GTK_LABEL(id_value), TRUE);

	GtkWidget *id_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_set_valign(id_value, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(id_box), id_value, FALSE, FALSE, 0);
	GtkWidget *copy = make_button("复制", "flat", G_CALLBACK(copy_device_id), device_id);
	gtk_box_pack_start(GTK_BOX(id_box), copy, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(card), row_new("设备 ID", id_box), FALSE, FALSE, 0);
	g_free(device_id);

	gchar *account = config_string("account_name");
	GtkWidget *acc_value = gtk_label_new(account ? account : "未登录");
	gtk_widget_set_name(acc_value, "valueplain");
	gtk_widget_set_halign(acc_value, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(card), row_new("账号", acc_value), FALSE, FALSE, 0);
	g_free(account);

	gboolean allow = config_bool("assist_allow", FALSE);
	GtkWidget *assist = gtk_label_new(allow ? "已开启" : "已关闭");
	gtk_widget_set_name(assist, allow ? "stateon" : "stateoff");
	gtk_widget_set_halign(assist, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(card), row_new("远程协助", assist), FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(page), card, FALSE, FALSE, 0);

	/* 操作按钮 */
	GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_halign(actions, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(actions),
			   make_button("打开主机窗口", "primary", G_CALLBACK(spawn_host), NULL), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actions),
			   make_button("打开控制端", "secondary", G_CALLBACK(spawn_viewer), NULL), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), actions, FALSE, FALSE, 0);

	GtkWidget *hint = gtk_label_new("设备 ID 和验证码在主机窗口里显示；协助开关也在那里。");
	gtk_widget_set_name(hint, "hint");
	gtk_widget_set_halign(hint, GTK_ALIGN_START);
	gtk_label_set_line_wrap(GTK_LABEL(hint), TRUE);
	gtk_box_pack_start(GTK_BOX(page), hint, FALSE, FALSE, 0);

	return page;
}

static GtkWidget *
build_page_assist(App *app)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
	gtk_widget_set_margin_start(page, 22);
	gtk_widget_set_margin_end(page, 22);
	gtk_widget_set_margin_top(page, 20);

	GtkWidget *title = gtk_label_new("远程协助");
	gtk_widget_set_name(title, "pagetitle");
	gtk_widget_set_halign(title, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(page), title, FALSE, FALSE, 0);

	GtkWidget *desc = gtk_label_new(
		"让别人远程协助这台机器。主机窗口会显示设备 ID 和验证码，\n"
		"把这两个发给对方，对方在 UU 远程「开始协助」里输入即可连接。");
	gtk_widget_set_name(desc, "hint");
	gtk_widget_set_halign(desc, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(page), desc, FALSE, FALSE, 0);

	GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_halign(actions, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(actions),
			   make_button("打开主机窗口", "primary", G_CALLBACK(spawn_host), NULL), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), actions, FALSE, FALSE, 0);
	return page;
}

static GtkWidget *
build_page_start(App *app)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
	gtk_widget_set_margin_start(page, 22);
	gtk_widget_set_margin_end(page, 22);
	gtk_widget_set_margin_top(page, 20);

	GtkWidget *title = gtk_label_new("开始协助");
	gtk_widget_set_name(title, "pagetitle");
	gtk_widget_set_halign(title, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(page), title, FALSE, FALSE, 0);

	GtkWidget *desc = gtk_label_new(
		"控制别人的设备。打开控制端后，可以在「远程协助」里选设备，\n"
		"或者在「开始协助」里输入对方的设备 ID 和验证码。");
	gtk_widget_set_name(desc, "hint");
	gtk_widget_set_halign(desc, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(page), desc, FALSE, FALSE, 0);

	GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_halign(actions, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(actions),
			   make_button("打开控制端", "primary", G_CALLBACK(spawn_viewer), NULL), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), actions, FALSE, FALSE, 0);
	return page;
}

static GtkWidget *
build_page_login(App *app)
{
	GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
	gtk_widget_set_margin_start(page, 22);
	gtk_widget_set_margin_end(page, 22);
	gtk_widget_set_margin_top(page, 20);

	GtkWidget *title = gtk_label_new("扫码登录");
	gtk_widget_set_name(title, "pagetitle");
	gtk_widget_set_halign(title, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(page), title, FALSE, FALSE, 0);

	GtkWidget *desc = gtk_label_new(
		"用手机上的网易 UU 远程 App 扫码登录。\n"
		"登录后这台机器会出现在你账号的设备列表里。");
	gtk_widget_set_name(desc, "hint");
	gtk_widget_set_halign(desc, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(page), desc, FALSE, FALSE, 0);

	GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_halign(actions, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(actions),
			   make_button("打开扫码窗口", "primary", G_CALLBACK(spawn_login), NULL), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(page), actions, FALSE, FALSE, 0);
	return page;
}

int
main(int argc, char **argv)
{
	gtk_init(&argc, &argv);

	GtkCssProvider *css = gtk_css_provider_new();
	gtk_css_provider_load_from_data(css, APP_CSS, -1, NULL);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
						 GTK_STYLE_PROVIDER(css),
						 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	App app = { 0 };

	GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win), "UU 远程 Linux");
	gtk_window_set_default_size(GTK_WINDOW(win), 520, 460);
	gtk_window_set_icon_name(GTK_WINDOW(win), "uu-remote-linux");
	gtk_window_set_resizable(GTK_WINDOW(win), FALSE);

	GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(win), root);

	/* 顶栏 */
	GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_name(header, "header");
	gtk_widget_set_margin_start(header, 16);
	gtk_widget_set_margin_end(header, 16);
	gtk_widget_set_margin_top(header, 14);
	gtk_widget_set_margin_bottom(header, 14);

	GtkWidget *logo = gtk_label_new("UU");
	gtk_widget_set_name(logo, "logo");
	gtk_widget_set_size_request(logo, 34, 34);
	gtk_style_context_add_class(gtk_widget_get_style_context(logo), "logo");
	gtk_box_pack_start(GTK_BOX(header), logo, FALSE, FALSE, 0);

	GtkWidget *titles = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_valign(titles, GTK_ALIGN_CENTER);
	GtkWidget *name = gtk_label_new("UU 远程 Linux");
	gtk_widget_set_name(name, "appname");
	gtk_widget_set_halign(name, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(titles), name, FALSE, FALSE, 0);
	GtkWidget *sub = gtk_label_new("非官方客户端 · 被控端与控制端");
	gtk_widget_set_name(sub, "appsub");
	gtk_widget_set_halign(sub, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(titles), sub, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(header), titles, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(root), header, FALSE, FALSE, 0);

	/* 主体 */
	GtkWidget *body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(root), body, TRUE, TRUE, 0);

	GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_widget_set_name(sidebar, "sidebar");
	gtk_widget_set_size_request(sidebar, 150, -1);
	gtk_widget_set_margin_top(sidebar, 16);
	gtk_widget_set_margin_bottom(sidebar, 16);
	gtk_widget_set_margin_start(sidebar, 12);
	gtk_widget_set_margin_end(sidebar, 12);
	gtk_box_pack_start(GTK_BOX(body), sidebar, FALSE, FALSE, 0);

	const char *nav_labels[4] = { "我的设备", "远程协助", "开始协助", "扫码登录" };
	for (int i = 0; i < 4; i++) {
		GtkWidget *btn = nav_button(&app, nav_labels[i], i);
		g_signal_connect(btn, "clicked", G_CALLBACK(nav_clicked), &app);
		gtk_box_pack_start(GTK_BOX(sidebar), btn, FALSE, FALSE, 0);
	}

	GtkWidget *stack = gtk_stack_new();
	gtk_widget_set_name(stack, "content");
	app.stack = stack;
	gtk_stack_add_titled(GTK_STACK(stack), build_page_devices(&app), "devices", "我的设备");
	gtk_stack_add_titled(GTK_STACK(stack), build_page_assist(&app), "assist", "远程协助");
	gtk_stack_add_titled(GTK_STACK(stack), build_page_start(&app), "start", "开始协助");
	gtk_stack_add_titled(GTK_STACK(stack), build_page_login(&app), "login", "扫码登录");
	gtk_box_pack_start(GTK_BOX(body), stack, TRUE, TRUE, 0);

	nav_select(&app, 0);

	g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	gtk_widget_show_all(win);
	gtk_main();
	return 0;
}
