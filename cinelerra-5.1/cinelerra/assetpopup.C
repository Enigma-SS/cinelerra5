
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "asset.h"
#include "assetedit.h"
#include "assetpopup.h"
#include "assetremove.h"
#include "assets.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bccapture.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "cache.h"
#include "clipedit.h"
#include "cstrdup.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "language.h"
#include "loadfile.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "renderengine.h"
#include "tracks.h"
#include "transportque.h"
#include "vframe.h"
#include "vrender.h"
#include "vwindow.h"
#include "vwindowgui.h"
#include "zwindow.h"


AssetPopup::AssetPopup(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

AssetPopup::~AssetPopup()
{
}

void AssetPopup::create_objects()
{
	BC_MenuItem *menu_item;
	BC_SubMenu *submenu;
	add_item(info = new AssetPopupInfo(mwindow, this));
	add_item(format = new AWindowListFormat(mwindow, gui));
	add_item(open_edl = new AssetPopupOpenEDL(mwindow, this));
	add_item(close_edl = new AssetPopupCloseEDL(mwindow, gui));
	add_item(to_clip = new AssetPopupToClip(mwindow, this));
	add_item(sort = new AssetPopupSort(mwindow, this));
	add_item(index = new AssetPopupBuildIndex(mwindow, this));
	add_item(view = new AssetPopupView(mwindow, this));
	add_item(view_window = new AssetPopupViewWindow(mwindow, this));
	add_item(open_mixer = new AssetPopupOpenMixer(mwindow, this));
	add_item(insert_mixer = new AssetPopupInsertMixer(mwindow, this));
	add_item(paste = new AssetPopupPaste(mwindow, this));
	add_item(menu_item = new BC_MenuItem(_("Match...")));
	menu_item->add_submenu(submenu = new BC_SubMenu());
	submenu->add_submenuitem(new AssetMatchSize(mwindow, this));
	submenu->add_submenuitem(new AssetMatchRate(mwindow, this));
	submenu->add_submenuitem(new AssetMatchAll(mwindow, this));
	add_item(menu_item = new BC_MenuItem(_("Remove...")));
	menu_item->add_submenu(submenu = new BC_SubMenu());
	submenu->add_submenuitem(new AssetPopupProjectRemove(mwindow, this));
	submenu->add_submenuitem(new AssetPopupDiskRemove(mwindow, this));
}

void AssetPopup::paste_assets()
{
	int proxy = mwindow->edl->session->awindow_folder == AW_PROXY_FOLDER ? 1 : 0;
	gui->collect_assets(proxy);
// Collect items into the drag vectors for temporary storage
	gui->unlock_window();
	mwindow->gui->lock_window("AssetPopup::paste_assets");
	mwindow->paste_assets(mwindow->edl->local_session->get_selectionstart(1),
		mwindow->edl->tracks->first, 0);   // do not overwrite
	mwindow->gui->unlock_window();
	gui->lock_window("AssetPopup::paste_assets");
}

void AssetPopup::match_size()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->gui->lock_window("AssetPopup::match_size");
	mwindow->asset_to_size();
	mwindow->gui->unlock_window();
}

void AssetPopup::match_rate()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->gui->lock_window("AssetPopup::match_rate");
	mwindow->asset_to_rate();
	mwindow->gui->unlock_window();
}

void AssetPopup::match_all()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->gui->lock_window("AssetPopup::match_rate");
	mwindow->asset_to_all();
	mwindow->gui->unlock_window();
}

int AssetPopup::update()
{
	format->update();
	int proxy = mwindow->edl->session->awindow_folder == AW_PROXY_FOLDER ? 1 : 0;
	gui->collect_assets(proxy);
	int enable_open = 0;
	int assets_total = mwindow->session->drag_assets->size();
	Indexable *idxbl = !assets_total ? 0 :
		mwindow->session->drag_assets->get(0);
	if( idxbl ) {
		if( idxbl->is_asset ) {
			Asset *asset = (Asset *)idxbl;
			if( asset->format == FILE_REF )
				enable_open = 1;
		}
		else
			enable_open = 1;
	}
	open_edl->set_enabled(enable_open);
	int enable_close = mwindow->stack.size() > 0 ? 1 : 0;
	close_edl->set_enabled(enable_close);
	int enable_clip = 0;
	for( int i=0; !enable_clip && i<assets_total; ++i ) {
		Indexable *idxbl = mwindow->session->drag_assets->get(i);
		if( !idxbl->is_asset ) enable_clip = 1;
	}
	to_clip->set_enabled(enable_clip);
	return 0;
}


AssetPopupInfo::AssetPopupInfo(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Info..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupInfo::~AssetPopupInfo()
{
}

int AssetPopupInfo::handle_event()
{
	int cur_x, cur_y;
	popup->gui->get_abs_cursor(cur_x, cur_y);
	int n = mwindow->session->drag_assets->size();
	if( n > 0 ) {
		int xs30 = xS(30), ys30 = yS(30);
		for( int i=0; i<n; ++i ) {
			AssetEdit *asset_edit = mwindow->awindow->get_asset_editor();
			asset_edit->edit_asset(
				mwindow->session->drag_assets->values[i], cur_x-xs30*i, cur_y-ys30*i);
		}
	}
	else if( mwindow->session->drag_clips->size() ) {
		popup->gui->awindow->clip_edit->edit_clip(
			mwindow->session->drag_clips->values[0], cur_x, cur_y);
	}
	return 1;
}


AssetPopupOpenEDL::AssetPopupOpenEDL(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Open EDL"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
AssetPopupOpenEDL::~AssetPopupOpenEDL()
{
}

int AssetPopupOpenEDL::handle_event()
{
	int assets_total = mwindow->session->drag_assets->size();
	if( !assets_total ) return 1;
	popup->unlock_window();
	EDL *edl = 0;
	Indexable *idxbl = mwindow->session->drag_assets->get(0);
	if( idxbl->is_asset ) {
		Asset *asset = (Asset *)idxbl;
		if( asset->format == FILE_REF ) {
			FileXML xml_file;
			const char *filename = asset->path;
			if( xml_file.read_from_file(filename, 1) ) {
				eprintf(_("Error: unable to open:\n  %s"), filename);
				return 1;
			}
			edl = new EDL;
			edl->create_objects();
			if( edl->load_xml(&xml_file, LOAD_ALL) ) {
				eprintf(_("Error: unable to load:\n  %s"), filename);
				edl->remove_user();
				return 1;
			}
		}
	}
	else {
		edl = (EDL *)idxbl;
	}
	if( edl )
		mwindow->stack_push(edl, idxbl);
	else
		eprintf(_("media is not EDL:\n%s"), idxbl->path);
	popup->lock_window("AssetPopupOpenEDL::handle_event");
	return 1;
}

AssetPopupCloseEDL::AssetPopupCloseEDL(MWindow *mwindow, AWindowGUI *gui)
 : BC_MenuItem(_("Close EDL"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}
AssetPopupCloseEDL::~AssetPopupCloseEDL()
{
}

int AssetPopupCloseEDL::handle_event()
{
	gui->unlock_window();
	mwindow->gui->lock_window("AssetPopupCloseEDL::handle_event");
	mwindow->stack_pop();
	mwindow->gui->unlock_window();
	gui->lock_window("AssetPopupCloseEDL::handle_event");
	return 1;
}


AssetPopupToClip::AssetPopupToClip(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("EDL to clip"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupToClip::~AssetPopupToClip()
{
}

int AssetPopupToClip::handle_event()
{
	mwindow->media_to_clip();
	return 1;
}


AssetPopupBuildIndex::AssetPopupBuildIndex(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Rebuild index"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupBuildIndex::~AssetPopupBuildIndex()
{
}

int AssetPopupBuildIndex::handle_event()
{
	popup->unlock_window();
//printf("AssetPopupBuildIndex::handle_event 1\n");
	mwindow->rebuild_indices();
	popup->lock_window("AssetPopupBuildIndex::handle_event");
	return 1;
}


AssetPopupSort::AssetPopupSort(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Sort"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupSort::~AssetPopupSort()
{
}

int AssetPopupSort::handle_event()
{
	mwindow->awindow->gui->sort_assets();
	return 1;
}


AssetPopupView::AssetPopupView(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("View"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupView::~AssetPopupView()
{
}

int AssetPopupView::handle_event()
{
	VWindow *vwindow = mwindow->get_viewer(1, DEFAULT_VWINDOW);

	if( mwindow->session->drag_assets->total )
		vwindow->change_source(
			mwindow->session->drag_assets->values[0]);
	else
	if( mwindow->session->drag_clips->total )
		vwindow->change_source(
			mwindow->session->drag_clips->values[0]);

	return 1;
}


AssetPopupViewWindow::AssetPopupViewWindow(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("View in new window"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupViewWindow::~AssetPopupViewWindow()
{
}

int AssetPopupViewWindow::handle_event()
{
	for( int i=0; i<mwindow->session->drag_assets->size(); ++i ) {
		VWindow *vwindow = mwindow->get_viewer(1);
		vwindow->gui->lock_window("AssetPopupView::handle_event 1");
		vwindow->change_source(mwindow->session->drag_assets->get(i));
		vwindow->gui->unlock_window();
	}
	for( int i=0; i<mwindow->session->drag_clips->size(); ++i ) {
		VWindow *vwindow = mwindow->get_viewer(1);
		vwindow->gui->lock_window("AssetPopupView::handle_event 2");
		vwindow->change_source(mwindow->session->drag_clips->get(i));
		vwindow->gui->unlock_window();
	}
	return 1;
}

AssetPopupOpenMixer::AssetPopupOpenMixer(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Open Mixers"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupOpenMixer::~AssetPopupOpenMixer()
{
}

int AssetPopupOpenMixer::handle_event()
{
	popup->unlock_window();
	mwindow->gui->lock_window("AssetPopupOpenMixer::handle_event");
	mwindow->create_mixers();
	mwindow->gui->unlock_window();
	popup->lock_window("AssetPopupOpenMixer::handle_event");
	return 1;
}

AssetPopupInsertMixer::AssetPopupInsertMixer(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Insert Mixers"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupInsertMixer::~AssetPopupInsertMixer()
{
}

int AssetPopupInsertMixer::handle_event()
{
	popup->unlock_window();
	mwindow->gui->lock_window("AssetPopupInsertMixer::handle_event");
	mwindow->create_mixers(-1);
	mwindow->gui->unlock_window();
	popup->lock_window("AssetPopupInsertMixer::handle_event");
	return 1;
}

AssetPopupPaste::AssetPopupPaste(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(C_("Paste"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupPaste::~AssetPopupPaste()
{
}

int AssetPopupPaste::handle_event()
{
	popup->paste_assets();
	return 1;
}


AssetMatchSize::AssetMatchSize(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Match project size"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int AssetMatchSize::handle_event()
{
	popup->match_size();
	return 1;
}

AssetMatchRate::AssetMatchRate(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Match frame rate"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int AssetMatchRate::handle_event()
{
	popup->match_rate();
	return 1;
}

AssetMatchAll::AssetMatchAll(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Match all"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int AssetMatchAll::handle_event()
{
	popup->match_all();
	return 1;
}


AssetPopupProjectRemove::AssetPopupProjectRemove(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Remove from project"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupProjectRemove::~AssetPopupProjectRemove()
{
}

int AssetPopupProjectRemove::handle_event()
{
	popup->gui->unlock_window();
	mwindow->remove_assets_from_project(1, 1, 1,
		mwindow->session->drag_assets, 0);
	popup->gui->lock_window("AssetPopupProjectRemove::handle_event");
	return 1;
}


AssetPopupDiskRemove::AssetPopupDiskRemove(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Remove from disk"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}


AssetPopupDiskRemove::~AssetPopupDiskRemove()
{
}

int AssetPopupDiskRemove::handle_event()
{
	mwindow->awindow->asset_remove->start();
	return 1;
}


AssetListMenu::AssetListMenu(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

AssetListMenu::~AssetListMenu()
{
}

void AssetListMenu::create_objects()
{
	add_item(load_file = new AssetPopupLoadFile(mwindow, gui));
	add_item(format = new AWindowListFormat(mwindow, gui));
	add_item(close_edl = new AssetPopupCloseEDL(mwindow, gui));
	add_item(select_used = new AssetSelectUsed(mwindow, gui));
	BC_SubMenu *submenu;
	select_used->add_submenu(submenu = new BC_SubMenu());
	submenu->add_submenuitem(new AssetSelectUsedItem(select_used, _("All"), SELECT_ALL));
	submenu->add_submenuitem(new AssetSelectUsedItem(select_used, _("Used"), SELECT_USED));
	submenu->add_submenuitem(new AssetSelectUsedItem(select_used, _("Unused"), SELECT_UNUSED));
	submenu->add_submenuitem(new AssetSelectUsedItem(select_used, _("None"), SELECT_NONE));
	add_item(new AWindowListSort(mwindow, gui));
	add_item(new AssetListCopy(mwindow, gui));
	add_item(new AssetListPaste(mwindow, gui));
	SnapshotSubMenu *snapshot_submenu;
	add_item(asset_snapshot = new AssetSnapshot(mwindow, this));
	asset_snapshot->add_submenu(snapshot_submenu = new SnapshotSubMenu(asset_snapshot));
	snapshot_submenu->add_submenuitem(new SnapshotMenuItem(snapshot_submenu, _("png"),  SNAPSHOT_PNG));
	snapshot_submenu->add_submenuitem(new SnapshotMenuItem(snapshot_submenu, _("jpeg"), SNAPSHOT_JPEG));
	snapshot_submenu->add_submenuitem(new SnapshotMenuItem(snapshot_submenu, _("tiff"), SNAPSHOT_TIFF));
	snapshot_submenu->add_submenuitem(new SnapshotMenuItem(snapshot_submenu, _("ppm"),  SNAPSHOT_PPM));
	GrabshotSubMenu *grabshot_submenu;
	add_item(asset_grabshot = new AssetGrabshot(mwindow, this));
	asset_grabshot->add_submenu(grabshot_submenu = new GrabshotSubMenu(asset_grabshot));
	grabshot_submenu->add_submenuitem(new GrabshotMenuItem(grabshot_submenu, _("png"),  GRABSHOT_PNG));
	grabshot_submenu->add_submenuitem(new GrabshotMenuItem(grabshot_submenu, _("jpeg"), GRABSHOT_JPEG));
	grabshot_submenu->add_submenuitem(new GrabshotMenuItem(grabshot_submenu, _("tiff"), GRABSHOT_TIFF));
	grabshot_submenu->add_submenuitem(new GrabshotMenuItem(grabshot_submenu, _("ppm"),  GRABSHOT_PPM));
	update_titles(1);
}

AssetPopupLoadFile::AssetPopupLoadFile(MWindow *mwindow, AWindowGUI *gui)
 : BC_MenuItem(_("Load files..."), "o", 'o')
{
	this->mwindow = mwindow;
	this->gui = gui;
}

AssetPopupLoadFile::~AssetPopupLoadFile()
{
}

int AssetPopupLoadFile::handle_event()
{
	mwindow->gui->mainmenu->load_file->thread->start();
	return 1;
}

void AssetListMenu::update_titles(int shots)
{
	format->update();
	int enable_close = mwindow->stack.size() > 0 ? 1 : 0;
	close_edl->set_enabled(enable_close);
	asset_snapshot->set_enabled(shots);
	asset_grabshot->set_enabled(shots);
}

AssetListCopy::AssetListCopy(MWindow *mwindow, AWindowGUI *gui)
 : BC_MenuItem(_("Copy file list"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	copy_dialog = 0;
}
AssetListCopy::~AssetListCopy()
{
	delete copy_dialog;
}

int AssetListCopy::handle_event()
{
	int len = 0;
	MWindowGUI *gui = mwindow->gui;
	gui->lock_window("AssetListCopy::handle_event");
	mwindow->awindow->gui->collect_assets();
	int n = mwindow->session->drag_assets->total;
	for( int i=0; i<n; ++i ) {
		Indexable *indexable = mwindow->session->drag_assets->values[i];
		const char *path = indexable->path;
		if( !*path ) continue;
		len += strlen(path) + 1;
	}
	char *text = new char[len+1], *cp = text;
	for( int i=0; i<n; ++i ) {
		Indexable *indexable = mwindow->session->drag_assets->values[i];
		const char *path = indexable->path;
		if( !*path ) continue;
		cp += sprintf(cp, "%s\n", path);
	}
	*cp = 0;
	int cur_x, cur_y;
	gui->get_abs_cursor(cur_x, cur_y, 0);
	gui->unlock_window(); 

	if( n ) {
		if( !copy_dialog )
			copy_dialog = new AssetCopyDialog(this);
		copy_dialog->start(text, cur_x, cur_y);
	}
	else {
		eprintf(_("Nothing selected"));
		delete [] text;
	}
	return 1;
}

AssetCopyDialog::AssetCopyDialog(AssetListCopy *copy)
 : BC_DialogThread()
{
	this->copy = copy;
	copy_window = 0;
}

void AssetCopyDialog::start(char *text, int x, int y)
{
	close_window();
	this->text = text;
	this->x = x;  this->y = y;
	BC_DialogThread::start();
}

AssetCopyDialog::~AssetCopyDialog()
{
	close_window();
}

BC_Window* AssetCopyDialog::new_gui()
{
	BC_DisplayInfo display_info;

	copy_window = new AssetCopyWindow(this);
	copy_window->create_objects();
	return copy_window;
}

void AssetCopyDialog::handle_done_event(int result)
{
	delete [] text;  text = 0;
}

void AssetCopyDialog::handle_close_event(int result)
{
	copy_window = 0;
}

#define ACW_W xS(500)
#define ACW_H yS(200)

AssetCopyWindow::AssetCopyWindow(AssetCopyDialog *copy_dialog)
 : BC_Window(_(PROGRAM_NAME ": Copy File List"),
	copy_dialog->x - ACW_W/2, copy_dialog->y - ACW_H/2,
	ACW_W, ACW_H, ACW_W, ACW_H, 1, 0, 1)
{
	this->copy_dialog = copy_dialog;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Moving clips");
}

AssetCopyWindow::~AssetCopyWindow()
{
}

void AssetCopyWindow::create_objects()
{
	lock_window("AssetCopyWindow::create_objects");
	BC_Title *title;
	int xs10 = xS(10);
	int ys5 = yS(5), ys10 = yS(10);
	int x = xs10, y = ys10;
	add_subwindow(title = new BC_Title(x, y, _("List of asset paths:")));
	y += title->get_h() + ys5;
	int text_w = get_w() - x - 10;
	int text_h = get_h() - y - BC_OKButton::calculate_h() - ys5;
	int text_rows = BC_TextBox::pixels_to_rows(this, MEDIUMFONT, text_h);
	char *text = copy_dialog->text;
	int len = strlen(text) + BCTEXTLEN;
	file_list = new BC_ScrollTextBox(this, x, y, text_w, text_rows, text, len);
	file_list->create_objects();

	add_subwindow(new BC_OKButton(this));
	show_window();
	unlock_window();
}

int AssetCopyWindow::resize_event(int w, int h)
{
	int fx = file_list->get_x(), fy = file_list->get_y();
	int text_w = w - fx - xS(10);
	int text_h = h - fy - BC_OKButton::calculate_h() - yS(5);
	int text_rows = BC_TextBox::pixels_to_rows(this, MEDIUMFONT, text_h);
	file_list->reposition_window(fx, fy, text_w, text_rows);
	return 0;
}

AssetListPaste::AssetListPaste(MWindow *mwindow, AWindowGUI *gui)
 : BC_MenuItem(_("Paste file list"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	paste_dialog = 0;
}
AssetListPaste::~AssetListPaste()
{
	delete paste_dialog;
}

int AssetListPaste::handle_event()
{
	if( !paste_dialog )
		paste_dialog = new AssetPasteDialog(this);
	else
		paste_dialog->close_window();
	int cur_x, cur_y;
	gui->get_abs_cursor(cur_x, cur_y, 0);
	paste_dialog->start(cur_x, cur_y);
	return 1;
}

AssetPasteDialog::AssetPasteDialog(AssetListPaste *paste)
 : BC_DialogThread()
{
	this->paste = paste;
	paste_window = 0;
}

AssetPasteDialog::~AssetPasteDialog()
{
	close_window();
}

BC_Window* AssetPasteDialog::new_gui()
{
	paste_window = new AssetPasteWindow(this);
	paste_window->create_objects();
	return paste_window;
}

void AssetPasteDialog::handle_done_event(int result)
{
	if( result ) return;
	const char *bp = paste_window->file_list->get_text(), *ep = bp+strlen(bp);
	ArrayList<char*> path_list;
	path_list.set_array_delete();

	for( const char *cp=bp; cp<ep && *cp; ) {
		const char *dp = strchr(cp, '\n');
		if( !dp ) dp = ep;
		char path[BCTEXTLEN], *pp = path;
		int len = sizeof(path)-1;
		while( --len>0 && cp<dp ) *pp++ = *cp++;
		if( *cp ) ++cp;
		*pp = 0;
		if( !strlen(path) ) continue;
		path_list.append(cstrdup(path));
	}
	if( !path_list.size() ) return;

	MWindow *mwindow = paste->mwindow;
	mwindow->interrupt_indexes();
	mwindow->gui->lock_window("AssetPasteDialog::handle_done_event");
	result = mwindow->load_filenames(&path_list,
		LOADMODE_RESOURCESONLY, LOADMODE_EDL_CLIP, 0);
	mwindow->gui->unlock_window();
	path_list.remove_all_objects();
	mwindow->save_backup();
	mwindow->restart_brender();
	mwindow->session->changes_made = 1;
}

void AssetPasteDialog::handle_close_event(int result)
{
	paste_window = 0;
}

void AssetPasteDialog::start(int x, int y)
{
	this->x = x;  this->y = y;
	BC_DialogThread::start();
}

#define APW_W xS(500)
#define APW_H yS(200)

AssetPasteWindow::AssetPasteWindow(AssetPasteDialog *paste_dialog)
 : BC_Window(_(PROGRAM_NAME ": Paste File List"),
	paste_dialog->x - APW_W/2, paste_dialog->y - APW_H/2,
	APW_W, APW_H, APW_W, APW_H, 1, 0, 1)
{
	this->paste_dialog = paste_dialog;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Moving clips");
}

AssetPasteWindow::~AssetPasteWindow()
{
}

void AssetPasteWindow::create_objects()
{
	lock_window("AssetPasteWindow::create_objects()");
	BC_Title *title;
	int xs10 = xS(10);
	int ys5 = yS(5), ys10 = yS(10);
	int x = xs10, y = ys10;
	add_subwindow(title = new BC_Title(x, y, _("Enter list of asset paths:")));
	y += title->get_h() + ys5;
	int text_w = get_w() - x - xs10;
	int text_h = get_h() - y - BC_OKButton::calculate_h() - ys5;
	int text_rows = BC_TextBox::pixels_to_rows(this, MEDIUMFONT, text_h);
	file_list = new BC_ScrollTextBox(this, x, y, text_w, text_rows, (char*)0, 65536);
	file_list->create_objects();
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	unlock_window();
}

int AssetPasteWindow::resize_event(int w, int h)
{
	int fx = file_list->get_x(), fy = file_list->get_y();
	int text_w = w - fx - xS(10);
	int text_h = h - fy - BC_OKButton::calculate_h() - yS(5);
	int text_rows = BC_TextBox::pixels_to_rows(this, MEDIUMFONT, text_h);
	file_list->reposition_window(fx, fy, text_w, text_rows);
	return 0;
}



AssetSnapshot::AssetSnapshot(MWindow *mwindow, AssetListMenu *asset_list_menu)
 : BC_MenuItem(_("Snapshot..."))
{
	this->mwindow = mwindow;
	this->asset_list_menu = asset_list_menu;
}

AssetSnapshot::~AssetSnapshot()
{
}

SnapshotSubMenu::SnapshotSubMenu(AssetSnapshot *asset_snapshot)
{
	this->asset_snapshot = asset_snapshot;
}

SnapshotSubMenu::~SnapshotSubMenu()
{
}

SnapshotMenuItem::SnapshotMenuItem(SnapshotSubMenu *submenu, const char *text, int mode)
 : BC_MenuItem(text)
{
	this->submenu = submenu;
	this->mode = mode;
}

SnapshotMenuItem::~SnapshotMenuItem()
{
}

int SnapshotMenuItem::handle_event()
{
	MWindow *mwindow = submenu->asset_snapshot->mwindow;
	EDL *edl = mwindow->edl;
	if( !edl->have_video() ) return 1;

	Preferences *preferences = mwindow->preferences;
	char filename[BCTEXTLEN], snapshot_path[BCTEXTLEN];
	static const char *exts[] = { "png", "jpg", "tif", "ppm" };
	time_t tt;     time(&tt);
	struct tm tm;  localtime_r(&tt,&tm);
	File::getenv_path(snapshot_path, preferences->snapshot_path);
	snprintf(filename,sizeof(filename),"%s/%s_%04d%02d%02d-%02d%02d%02d.%s",
		snapshot_path, _("snap"),
		1900+tm.tm_year,1+tm.tm_mon,tm.tm_mday,
		tm.tm_hour,tm.tm_min,tm.tm_sec, exts[mode]);
	char *asset_path = FileSystem::basepath(filename);
	Asset *asset = new Asset(asset_path);
	delete [] asset_path;

	int fw = edl->get_w(), fh = edl->get_h();
	int fcolor_model = edl->session->color_model;

	switch( mode ) {
	case SNAPSHOT_PNG:
		asset->format = FILE_PNG;
		asset->png_use_alpha = 1;
		break;
	case SNAPSHOT_JPEG:
		asset->format = FILE_JPEG;
		asset->jpeg_quality = 90;
		break;
	case SNAPSHOT_TIFF:
		asset->format = FILE_TIFF;
		asset->tiff_cmodel = 0;
		asset->tiff_compression = 0;
		break;
	case SNAPSHOT_PPM:
		asset->format = FILE_PPM;
		break;
	}
	asset->width = fw;
	asset->height = fh;
	asset->audio_data = 0;
	asset->video_data = 1;
	asset->video_length = 1;
	asset->layers = 1;

	File file;
	int processors = preferences->project_smp + 1;
	if( processors > 8 ) processors = 8;
	file.set_processors(processors);
	int ret = file.open_file(preferences, asset, 0, 1);
	if( !ret ) {
		file.start_video_thread(1, fcolor_model,
			processors > 1 ? 2 : 1, 0);
		VFrame ***frames = file.get_video_buffer();
		VFrame *frame = frames[0][0];
		TransportCommand command(preferences);
		//command.command = audio_tracks ? NORMAL_FWD : CURRENT_FRAME;
		command.command = CURRENT_FRAME;
		command.get_edl()->copy_all(edl);
		command.change_type = CHANGE_ALL;
		command.realtime = 0;

		RenderEngine render_engine(0, preferences, 0, 0);
		CICache *video_cache = new CICache(preferences);
		render_engine.set_vcache(video_cache);
		render_engine.arm_command(&command);

		double position = edl->local_session->get_selectionstart(1);
		int64_t source_position = (int64_t)(position * edl->get_frame_rate());
		ret = !render_engine.vrender ? 1 :
			render_engine.vrender->process_buffer(frame, source_position, 0);
		if( !ret )
			ret = file.write_video_buffer(1);
		file.close_file();
		video_cache->remove_user();
	}
	if( !ret ) {
		asset->folder_no = AW_MEDIA_FOLDER;
		mwindow->edl->assets->append(asset);
		mwindow->awindow->gui->async_update_assets();
	}
	else {
		eprintf(_("snapshot render failed"));
		asset->remove_user();
	}
	return 1;
}


AssetGrabshot::AssetGrabshot(MWindow *mwindow, AssetListMenu *asset_list_menu)
 : BC_MenuItem(_("Grabshot..."))
{
	this->mwindow = mwindow;
	this->asset_list_menu = asset_list_menu;
}

AssetGrabshot::~AssetGrabshot()
{
}

GrabshotSubMenu::GrabshotSubMenu(AssetGrabshot *asset_grabshot)
{
	this->asset_grabshot = asset_grabshot;
}

GrabshotSubMenu::~GrabshotSubMenu()
{
}

GrabshotMenuItem::GrabshotMenuItem(GrabshotSubMenu *submenu, const char *text, int mode)
 : BC_MenuItem(text)
{
	this->submenu = submenu;
	this->mode = mode;
	grab_thread = 0;
}

GrabshotMenuItem::~GrabshotMenuItem()
{
	delete grab_thread;
}

int GrabshotMenuItem::handle_event()
{
	if( !grab_thread )
		grab_thread = new GrabshotThread(submenu->asset_grabshot->mwindow);
	if( !grab_thread->running() )
		grab_thread->start(this);
	return 1;
}

GrabshotThread::GrabshotThread(MWindow *mwindow)
 : BC_DragBox(mwindow->gui)
{
	this->mwindow = mwindow;
}

GrabshotThread::~GrabshotThread()
{
}

void GrabshotThread::start(GrabshotMenuItem *menu_item)
{
	mode = menu_item->mode;
	start_drag();
}

int GrabshotThread::handle_done_event(int x0, int y0, int x1, int y1)
{
	int cx = x0,    cy = y0;
	int cw = x1-x0, ch = y1-y0;

	Preferences *preferences = mwindow->preferences;
	char filename[BCTEXTLEN], snapshot_path[BCTEXTLEN];
	static const char *exts[] = { "png", "jpg", "tif", "ppm" };
	time_t tt;     time(&tt);
	struct tm tm;  localtime_r(&tt,&tm);
	File::getenv_path(snapshot_path, preferences->snapshot_path);
	snprintf(filename,sizeof(filename),"%s/%s_%04d%02d%02d-%02d%02d%02d.%s",
		snapshot_path, _("grab"),
		1900+tm.tm_year,1+tm.tm_mon,tm.tm_mday,
		tm.tm_hour,tm.tm_min,tm.tm_sec, exts[mode]);
	char *asset_path = FileSystem::basepath(filename);
	Asset *asset = new Asset(asset_path);
	delete [] asset_path;
	switch( mode ) {
	case GRABSHOT_PNG:
		asset->format = FILE_PNG;
		asset->png_use_alpha = 1;
		break;
	case GRABSHOT_JPEG:
		asset->format = FILE_JPEG;
		asset->jpeg_quality = 90;
		break;
	case GRABSHOT_TIFF:
		asset->format = FILE_TIFF;
		asset->tiff_cmodel = 0;
		asset->tiff_compression = 0;
		break;
	case GRABSHOT_PPM:
		asset->format = FILE_PPM;
		break;
	}

// no odd dimensions
	int rw = mwindow->gui->get_root_w(0);
	int rh = mwindow->gui->get_root_h(0);
	if( cx < 0 ) { cw += cx;  cx = 0; }
	if( cy < 0 ) { ch += cy;  cy = 0; }
	if( cx+cw > rw ) cw = rw-cx;
	if( cy+ch > rh ) ch = rh-cy;
	if( !cw || !ch ) return 0;

	VFrame vframe(cw,ch, BC_RGB888);
	if( cx+cw < rw ) ++cw;
	if( cy+ch < rh ) ++ch;
	BC_Capture capture_bitmap(cw,ch, 0);
	capture_bitmap.capture_frame(&vframe, cx,cy);

	asset->width = vframe.get_w();
	asset->height = vframe.get_h();
	asset->audio_data = 0;
	asset->video_data = 1;
	asset->video_length = 1;
	asset->layers = 1;

	File file;
	int fcolor_model = mwindow->edl->session->color_model;
	int processors = preferences->project_smp + 1;
	if( processors > 8 ) processors = 8;
	file.set_processors(processors);
	int ret = file.open_file(preferences, asset, 0, 1);
	if( !ret ) {
		file.start_video_thread(1, fcolor_model,
			processors > 1 ? 2 : 1, 0);
		VFrame ***frames = file.get_video_buffer();
		VFrame *frame = frames[0][0];
		frame->transfer_from(&vframe);
		ret = file.write_video_buffer(1);
		file.close_file();
	}
	if( !ret ) {
		asset->folder_no = AW_MEDIA_FOLDER;
		mwindow->edl->assets->append(asset);
		mwindow->awindow->gui->async_update_assets();
	}
	else {
		eprintf(_("grabshot render failed"));
		asset->remove_user();
	}

	return 1;
}

