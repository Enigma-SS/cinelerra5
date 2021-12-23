#include "bcwindowbase.h"
#include "bcdisplayinfo.h"
#include "bcdialog.h"
#include "awindow.h"
#include "awindowgui.h"
#include "cstrdup.h"
#include "indexable.h"
#include "language.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mwindow.h"
#include "shbtnprefs.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "theme.h"

#include <sys/wait.h>

ShBtnRun::ShBtnRun(const char *nm, const char *cmds, int warn)
 : Thread(0, 0, 1)
{
	argv.set_array_delete();
	strncpy(name, nm, sizeof(name)-1);
	name[sizeof(name)-1] = 0;
	strncpy(commands, cmds, sizeof(commands)-1);
	commands[sizeof(commands)-1] = 0;
	this->warn = warn;
}
ShBtnRun::~ShBtnRun()
{
	argv.remove_all_objects();
}

void ShBtnRun::add_arg(const char *v)
{
	argv.append(cstrdup(v));
}

void ShBtnRun::run()
{
	pid_t pid = vfork();
	if( pid < 0 ) {
		perror("fork");
		return;
	}
	char msg[BCTEXTLEN];
	if( !pid ) {
		argv.append(0);
		execvp(argv[0], &argv[0]);
		return;
	}
	// warn <0:always, =0:never, >0:on err
	if( !warn ) return;
	int stat;  waitpid(pid, &stat, 0);
	if( !stat ) {
		if( warn > 0 ) return;
		sprintf(msg, "%s: completed", name);
	}
	else
		sprintf(msg, "%s: error exit status %d", name, stat);
	MainError::show_error(msg);
}

ShBtnPref::ShBtnPref(const char *nm, const char *cmds, int warn, int run_script)
{
	strncpy(name, nm, sizeof(name));
	strncpy(commands, cmds, sizeof(commands));
	this->warn = warn;
	this->run_script = run_script;
}

ShBtnPref::~ShBtnPref()
{
}

void ShBtnPref::execute(ArrayList<Indexable*> &args)
{
// thread async+autodelete, no explicit delete
	ShBtnRun *job = new ShBtnRun(name, commands, warn);
	job->add_arg("/bin/bash");
	job->add_arg(commands);
	int n = args.size();
	for( int i=0; i<n; ++i ) {
		Indexable *idxbl = args[i];
		if( !idxbl->is_asset ) continue;
		job->add_arg(idxbl->path);
	}
	job->start();
}

void ShBtnPref::execute()
{
	ShBtnRun *job = new ShBtnRun(name, commands, warn);
	job->add_arg("/bin/bash");
	job->add_arg("-c");
	job->add_arg(commands);
	job->start();
}

ShBtnEditDialog::ShBtnEditDialog(PreferencesWindow *pwindow)
 : BC_DialogThread()
{
	this->pwindow = pwindow;
}

ShBtnEditDialog::~ShBtnEditDialog()
{
	close_window();
}

BC_Window* ShBtnEditDialog::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x();
	int y = display_info.get_abs_cursor_y();

	sb_window = new ShBtnEditWindow(this, x, y);
	sb_window->create_objects();
	return sb_window;
}

void ShBtnEditDialog::handle_close_event(int result)
{
	sb_window = 0;
}


ShBtnEditWindow::ShBtnEditWindow(ShBtnEditDialog *shbtn_edit, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Shell"), x, y,
		xS(300), yS(200), xS(300), yS(200), 0, 0, 1)
{
	this->shbtn_edit = shbtn_edit;
	sb_dialog = 0;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Menu Bar Shell Commands");
}

ShBtnEditWindow::~ShBtnEditWindow()
{
	delete sb_dialog;
}

int ShBtnEditWindow::list_update()
{
	shbtn_items.remove_all_objects();
	Preferences *preferences = shbtn_edit->pwindow->thread->preferences;
	for( int i=0; i<preferences->shbtn_prefs.size(); ++i ) {
		shbtn_items.append(new ShBtnPrefItem(preferences->shbtn_prefs[i]));
	}
	return op_list->update(&shbtn_items, 0, 0, 1);
}

ShBtnAddButton::ShBtnAddButton(ShBtnEditWindow *sb_window, int x, int y)
 : BC_GenericButton(x, y, _("Add"))
{
	this->sb_window = sb_window;
}

ShBtnAddButton::~ShBtnAddButton()
{
}

int ShBtnAddButton::handle_event()
{

	Preferences *preferences = sb_window->shbtn_edit->pwindow->thread->preferences;
	ShBtnPref *pref = new ShBtnPref(_("new"), "", 0, 0);
	preferences->shbtn_prefs.append(pref);
	sb_window->list_update();
	return sb_window->start_edit(pref);
}

ShBtnDelButton::ShBtnDelButton(ShBtnEditWindow *sb_window, int x, int y)
 : BC_GenericButton(x, y, _("Del"))
{
	this->sb_window = sb_window;
}

ShBtnDelButton::~ShBtnDelButton()
{
}

int ShBtnDelButton::handle_event()
{
	ShBtnPrefItem *sp = (ShBtnPrefItem *)sb_window->op_list->get_selection(0,0);
	if( !sp ) return 0;
	Preferences *preferences = sb_window->shbtn_edit->pwindow->thread->preferences;
	preferences->shbtn_prefs.remove(sp->pref);
	sb_window->list_update();
	return 1;
}

ShBtnEditButton::ShBtnEditButton(ShBtnEditWindow *sb_window, int x, int y)
 : BC_GenericButton(x, y, _("Edit"))
{
	this->sb_window = sb_window;
}

ShBtnEditButton::~ShBtnEditButton()
{
}

int ShBtnEditButton::handle_event()
{
	ShBtnPrefItem *sp = (ShBtnPrefItem *)sb_window->op_list->get_selection(0,0);
	if( !sp ) return 0;
	return sb_window->start_edit(sp->pref);
}

ShBtnTextDialog::ShBtnTextDialog(ShBtnEditWindow *sb_window)
 : BC_DialogThread()
{
	this->sb_window = sb_window;
	this->pref = 0;
}

ShBtnTextDialog::~ShBtnTextDialog()
{
	close_window();
}

ShBtnTextWindow::ShBtnTextWindow(ShBtnEditWindow *sb_window, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Commands"), x, y,
		xS(640), yS(160), xS(640), yS(150), 0, 0, 1)
{
        this->sb_window = sb_window;
	warn = sb_window->sb_dialog->pref->warn;
	run_script = sb_window->sb_dialog->pref->run_script;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Menu Bar Shell Commands");
}

ShBtnTextWindow::~ShBtnTextWindow()
{
}


ShBtnErrWarnItem::ShBtnErrWarnItem(ShBtnErrWarn *popup,
                const char *text, int warn)
 : BC_MenuItem(text)
{
        this->popup = popup;
        this->warn = warn;
}

int ShBtnErrWarnItem::handle_event()
{
        popup->set_text(get_text());
	popup->st_window->warn = warn;
        return 1;
}

ShBtnErrWarn::ShBtnErrWarn(ShBtnTextWindow *st_window, int x, int y)
 : BC_PopupMenu(x, y, xS(120), st_window->warn < 0 ? _("Always"):
	!st_window->warn ? _("Never") : _("On Error"))
{
        this->st_window = st_window;
}
ShBtnErrWarn::~ShBtnErrWarn()
{
}
int ShBtnErrWarn::handle_event()
{
        return 0;
}

void ShBtnErrWarn::create_objects()
{
	add_item(new ShBtnErrWarnItem(this,_("Always"), -1));
	add_item(new ShBtnErrWarnItem(this,_("Never"), 0));
	add_item(new ShBtnErrWarnItem(this,_("On Error"), 1));
}


ShBtnRunScript::ShBtnRunScript(ShBtnTextWindow *st_window, int x, int y)
 : BC_CheckBox(x, y, &st_window->run_script, _("run /path/script.sh + argvs"))
{
        this->st_window = st_window;
}

ShBtnRunScript::~ShBtnRunScript()
{
}

void ShBtnTextWindow::create_objects()
{
	lock_window("ShBtnTextWindow::create_objects");
        int x = xS(10), y = yS(10);
	int x1 = xS(160);
	BC_Title *title = new BC_Title(x, y, _("Label:"));
	add_subwindow(title);
	title = new BC_Title(x1, y, _("Commands:"));
	add_subwindow(title);
	y += title->get_h() + yS(8);
	ShBtnPref *pref = sb_window->sb_dialog->pref;
        cmd_name = new BC_TextBox(x, y, xS(140), 1, pref->name);
        add_subwindow(cmd_name);
        cmd_text = new BC_ScrollTextBox(this, x1, y, get_w()-x1-xS(20), 4, pref->commands);
	cmd_text->create_objects();
	y += cmd_text->get_h() + yS(16);
        add_subwindow(title = new BC_Title(x1,y, _("OnExit Notify:")));
	x1 += title->get_w() + xS(10);
        add_subwindow(st_err_warn = new ShBtnErrWarn(this, x1, y));
	st_err_warn->create_objects();
	x1 += st_err_warn->get_w() + xS(20);
        add_subwindow(st_run_script = new ShBtnRunScript(this, x1, y));
        y = get_h() - ShBtnTextOK::calculate_h() - yS(10);
        add_subwindow(new ShBtnTextOK(this, x, y));
        show_window();
	unlock_window();
}

ShBtnTextOK::ShBtnTextOK(ShBtnTextWindow *st_window, int x, int y)
 : BC_OKButton(x, y)
{
	this->st_window = st_window;
}

ShBtnTextOK::~ShBtnTextOK()
{
}

int ShBtnTextOK::handle_event()
{
	ShBtnPref *pref = st_window->sb_window->sb_dialog->pref;
	strcpy(pref->name, st_window->cmd_name->get_text());
	strcpy(pref->commands, st_window->cmd_text->get_text());
	pref->warn = st_window->warn;
	pref->run_script = st_window->run_script;
	return BC_OKButton::handle_event();
}


BC_Window *ShBtnTextDialog::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x();
	int y = display_info.get_abs_cursor_y();

	st_window = new ShBtnTextWindow(sb_window, x, y);
	st_window->create_objects();
	return st_window;
}

void ShBtnTextDialog::handle_close_event(int result)
{
	if( !result ) {
		sb_window->lock_window("ShBtnTextDialog::handle_close_event");
		sb_window->list_update();
		sb_window->unlock_window();
	}
	st_window = 0;
}

int ShBtnTextDialog::start_edit(ShBtnPref *pref)
{
	this->pref = pref;
	start();
	return 1;
}

void ShBtnEditWindow::create_objects()
{
	lock_window("ShBtnEditWindow::create_objects");
	Preferences *preferences = shbtn_edit->pwindow->thread->preferences;
	for( int i=0; i<preferences->shbtn_prefs.size(); ++i ) {
		shbtn_items.append(new ShBtnPrefItem(preferences->shbtn_prefs[i]));
	}
	int x = xS(10), y = yS(10);
	add_subwindow(op_list = new ShBtnPrefList(this, x, y));
	x = xS(190);
	add_subwindow(add_button = new ShBtnAddButton(this, x, y));
	y += add_button->get_h() + yS(8);
	add_subwindow(del_button = new ShBtnDelButton(this, x, y));
	y += del_button->get_h() + yS(8);
	add_subwindow(edit_button = new ShBtnEditButton(this, x, y));
	add_subwindow(new BC_OKButton(this));
	show_window();
	unlock_window();
}

int ShBtnEditWindow::start_edit(ShBtnPref *pref)
{
	if( !sb_dialog )
		sb_dialog = new ShBtnTextDialog(this);
	return sb_dialog->start_edit(pref);
}


ShBtnPrefItem::ShBtnPrefItem(ShBtnPref *pref)
 : BC_ListBoxItem(pref->name)
{
	this->pref = pref;
}

ShBtnPrefItem::~ShBtnPrefItem()
{
}

ShBtnPrefList::ShBtnPrefList(ShBtnEditWindow *sb_window, int x, int y)
 : BC_ListBox(x, y, xS(140), yS(100), LISTBOX_TEXT, &sb_window->shbtn_items, 0, 0)
{
	this->sb_window = sb_window;
}

ShBtnPrefList::~ShBtnPrefList()
{
}

int ShBtnPrefList::handle_event()
{
	return 1;
}

MainShBtnItem::MainShBtnItem(MainShBtns *shbtns, ShBtnPref *pref)
 : BC_MenuItem(pref->name)
{
	this->shbtns = shbtns;
	this->pref = pref;
}

int MainShBtnItem::handle_event()
{
	MWindow *mwindow = shbtns->mwindow;
	if( pref->run_script ) {
		AWindowGUI *agui = mwindow->awindow->gui;
		agui->lock_window("MainShBtnItem::handle_event");
		mwindow->awindow->gui->collect_assets();
		pref->execute(*mwindow->session->drag_assets);
		agui->unlock_window();
	}
	else
		pref->execute();
	return 1;
}

MainShBtns::MainShBtns(MWindow *mwindow, int x, int y)
 : BC_PopupMenu(x, y, 0, "", -1, mwindow->theme->shbtn_data)
{
	this->mwindow = mwindow;
	set_tooltip(_("shell cmds"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Menu Bar Shell Commands");
}

int MainShBtns::load(Preferences *preferences)
{
	while( total_items() ) del_item(0);
	for( int i=0; i<preferences->shbtn_prefs.size(); ++i )
		add_item(new MainShBtnItem(this, preferences->shbtn_prefs[i]));
	return 0;
}

int MainShBtns::handle_event()
{
	return 1;
}

