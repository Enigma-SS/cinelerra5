/* cakewalk.C (uncommented). Part of the Cakewalk theme. */
#include "bcsignals.h"
#include "clip.h"
#include "cwindowgui.h"
#include "cakewalk.h"
#include "edl.h"
#include "edlsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mbuttons.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "patchbay.h"
#include "preferencesthread.h"
#include "recordgui.h"
#include "recordmonitor.h"
#include "setformat.h"
#include "statusbar.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "vframe.h"
#include "vwindowgui.h"
#define ALARM 0xff0000
#define CwGray 0x31363b
#define CwDarkGray 0x232629
#define TextFg 0xeff0f1
#define TextFgBright 0xfffdee
#define TextBorderOut 0x7f8c8d
#define CwCyan 0x3daee9
#define CwCyanDark 0x2980b9
#define ComicYellow 0xffcc00
#define CwOrange 0xf67400
#define LockedRed 0xda4453
#define MeterGreen 0xa7fd6f
#define MeterYellow 0xfce16f
PluginClient* new_plugin(PluginServer *server)
{
 return new CAKEWALKTHEMEMain(server);
}
CAKEWALKTHEMEMain::CAKEWALKTHEMEMain(PluginServer *server)
 : PluginTClient(server)
{
}
CAKEWALKTHEMEMain::~CAKEWALKTHEMEMain()
{
}
const char* CAKEWALKTHEMEMain::plugin_title() { return N_("Cakewalk"); }
Theme* CAKEWALKTHEMEMain::new_theme()
{
 theme = new CAKEWALKTHEME;
 extern unsigned char _binary_theme_cakewalk_data_start[];
 theme->set_data(_binary_theme_cakewalk_data_start);
 return theme;
}
CAKEWALKTHEME::CAKEWALKTHEME()
 : Theme()
{
}
CAKEWALKTHEME::~CAKEWALKTHEME()
{
 delete camerakeyframe_data;
 delete channel_position_data;
 delete keyframe_data;
 delete maskkeyframe_data;
 delete modekeyframe_data;
 delete hardedge_data;
 delete pankeyframe_data;
 delete projectorkeyframe_data;
}
void CAKEWALKTHEME::initialize()
{
 BC_Resources *resources = BC_WindowBase::get_resources();
 delete about_bg;
 about_bg = new VFramePng(get_image_data("about_bg.png"));
 new_image_set_images("mwindow_icon", 1, new VFramePng(get_image_data("cin_icon_mwin.png")));
 new_image_set_images("vwindow_icon", 1, new VFramePng(get_image_data("cin_icon_vwin.png")));
 new_image_set_images("cwindow_icon", 1, new VFramePng(get_image_data("cin_icon_cwin.png")));
 new_image_set_images("awindow_icon", 1, new VFramePng(get_image_data("cin_icon_awin.png")));
 new_image_set_images("record_icon", 1, new VFramePng(get_image_data("cin_icon_rec.png")));
 resources->text_default = TextFg;
 resources->text_background = CwDarkGray;
 resources->text_background_disarmed = LockedRed;
 resources->text_border1 = TextBorderOut;
 resources->text_border2 = CwDarkGray;
 resources->text_border3 = CwDarkGray;
 resources->text_border4 = TextBorderOut;
 resources->text_border2_hi = CwOrange;
 resources->text_border3_hi = CwOrange;
 resources->text_highlight = CwCyan;
 resources->text_inactive_highlight = CwCyanDark;
   resources->bg_color = CwGray;
 resources->border_light2 = 0xaaff00;
 resources->border_shadow2 = 0xaaaaff;
 resources->default_text_color = TextFg;
 resources->menu_title_text = WHITE;
 resources->popup_title_text = WHITE;
 resources->menu_item_text = WHITE;
 resources->menu_highlighted_fontcolor = BLACK;
 resources->generic_button_margin = xS(20);
 resources->pot_needle_color = TextFg;
 resources->pot_offset = 1;
 resources->progress_text = resources->text_default;
 resources->meter_font_color = resources->default_text_color;
 resources->menu_light = CwGray;
 resources->menu_down = ALARM;
 resources->menu_up = ALARM;
 resources->menu_shadow = ALARM;
 resources->menu_highlighted = CwCyan;
 resources->popupmenu_margin = xS(15);
 resources->popupmenu_triangle_margin = xS(15);
 resources->listbox_title_color = TextFgBright;
 resources->listbox_title_margin = xS(15);
 resources->listbox_title_hotspot = xS(15);
 resources->listbox_border1 = TextBorderOut;
 resources->listbox_border2 = CwDarkGray;
 resources->listbox_border3 = CwDarkGray;
 resources->listbox_border4 = TextBorderOut;
 resources->listbox_border2_hi = CwCyan;
 resources->listbox_border3_hi = CwCyan;
 resources->listbox_highlighted = CwCyan;
 resources->listbox_inactive = CwDarkGray;
 resources->listbox_bg = 0;
 resources->listbox_text = TextFg;
 resources->listbox_selected = CwCyan;
 resources->filebox_margin = yS(130);
 resources->file_color = WHITE;
 resources->directory_color = ComicYellow;
 title_font = MEDIUMFONT;
 title_color = BLACK;
 recordgui_fixed_color = MeterGreen;
 recordgui_variable_color = MeterYellow;
 channel_position_color = MeterYellow;
 resources->meter_title_w = xS(28);
 edit_font_color = ComicYellow;
 assetedit_color = TextFgBright;
 timebar_cursor_color = WHITE;
 resources->tooltip_bg_color = TextFgBright;
 resources->tooltip_delay = 1500;
 audio_color = BLACK;
 resources->audiovideo_color = TextFg;
 resources->blink_rate = 750;
 clock_fg_color = MeterGreen;
 inout_highlight_color=CwCyan;
 new_toggle(
  "loadmode_new.png",
  "loadmode_up.png",
  "loadmode_hi.png",
  "loadmode_checked.png",
  "loadmode_dn.png",
  "loadmode_checkedhi.png",
  "loadmode_new");
 new_toggle(
  "loadmode_none.png",
  "loadmode_up.png",
  "loadmode_hi.png",
  "loadmode_checked.png",
  "loadmode_dn.png",
  "loadmode_checkedhi.png",
  "loadmode_none");
 new_toggle(
  "loadmode_newcat.png",
  "loadmode_up.png",
  "loadmode_hi.png",
  "loadmode_checked.png",
  "loadmode_dn.png",
  "loadmode_checkedhi.png",
  "loadmode_newcat");
 new_toggle(
  "loadmode_cat.png",
  "loadmode_up.png",
  "loadmode_hi.png",
  "loadmode_checked.png",
  "loadmode_dn.png",
  "loadmode_checkedhi.png",
  "loadmode_cat");
 new_toggle(
  "loadmode_newtracks.png",
  "loadmode_up.png",
  "loadmode_hi.png",
  "loadmode_checked.png",
  "loadmode_dn.png",
  "loadmode_checkedhi.png",
  "loadmode_newtracks");
 new_toggle(
  "loadmode_paste.png",
  "loadmode_up.png",
  "loadmode_hi.png",
  "loadmode_checked.png",
  "loadmode_dn.png",
  "loadmode_checkedhi.png",
  "loadmode_paste");
 new_toggle(
  "loadmode_resource.png",
  "loadmode_up.png",
  "loadmode_hi.png",
  "loadmode_checked.png",
  "loadmode_dn.png",
  "loadmode_checkedhi.png",
  "loadmode_resource");

 new_toggle("loadmode_edl_clip.png",
  "loadmode_up.png",
  "loadmode_hi.png",
  "loadmode_checked.png",
  "loadmode_dn.png",
  "loadmode_checkedhi.png",
  "loadmode_edl_clip");
 new_toggle("loadmode_edl_nested.png",
  "loadmode_up.png",
  "loadmode_hi.png",
  "loadmode_checked.png",
  "loadmode_dn.png",
  "loadmode_checkedhi.png",
  "loadmode_edl_nested");
 new_toggle("loadmode_edl_fileref.png",
  "loadmode_up.png",
  "loadmode_hi.png",
  "loadmode_checked.png",
  "loadmode_dn.png",
  "loadmode_checkedhi.png",
  "loadmode_edl_fileref");

 resources->filebox_icons_images = new_button(
  "icons.png",
  "fileboxbutton_up.png",
  "fileboxbutton_hi.png",
  "fileboxbutton_dn.png",
  "filebox_icons");
 resources->filebox_text_images = new_button(
  "text.png",
  "fileboxbutton_up.png",
  "fileboxbutton_hi.png",
  "fileboxbutton_dn.png",
  "filebox_text");
 resources->filebox_newfolder_images = new_button(
  "folder.png",
  "fileboxbutton_up.png",
  "fileboxbutton_hi.png",
  "fileboxbutton_dn.png",
  "filebox_newfolder");
 resources->filebox_rename_images = new_button(
  "rename.png",
  "fileboxbutton_up.png",
  "fileboxbutton_hi.png",
  "fileboxbutton_dn.png",
  "filebox_rename");
 resources->filebox_updir_images = new_button(
  "updir.png",
  "fileboxbutton_up.png",
  "fileboxbutton_hi.png",
  "fileboxbutton_dn.png",
  "filebox_updir");
 resources->filebox_delete_images = new_button(
  "delete.png",
  "fileboxbutton_up.png",
  "fileboxbutton_hi.png",
  "fileboxbutton_dn.png",
  "filebox_delete");
 resources->filebox_reload_images = new_button(
  "reload.png",
  "fileboxbutton_up.png",
  "fileboxbutton_hi.png",
  "fileboxbutton_dn.png",
  "filebox_reload");
 resources->filebox_descend_images = new_button(
  "openfolder.png",
  "filebox_bigbutton_up.png",
  "filebox_bigbutton_hi.png",
  "filebox_bigbutton_dn.png",
  "filebox_descend");
 resources->usethis_button_images = resources->ok_images = new_button(
  "ok.png",
  "filebox_bigbutton_up.png",
  "filebox_bigbutton_hi.png",
  "filebox_bigbutton_dn.png",
  "ok_button");
 new_button(
  "ok.png",
  "new_bigbutton_up.png",
  "new_bigbutton_hi.png",
  "new_bigbutton_dn.png",
  "new_ok_images");
 new_button(
  "reset.png",
  "reset_up.png",
  "reset_hi.png",
  "reset_dn.png",
  "reset_button");
 new_button(
  "unclear.png",
  "unclear_up.png",
  "unclear_hi.png",
  "unclear_dn.png",
  "unclear_button");
 new_button(
  "keyframe.png",
  "editpanel_up.png",
  "editpanel_hi.png",
  "editpanel_dn.png",
  "keyframe_button");
 resources->cancel_images = new_button(
  "cancel.png",
  "filebox_bigbutton_up.png",
  "filebox_bigbutton_hi.png",
  "filebox_bigbutton_dn.png",
  "cancel_button");
 new_button(
  "cancel.png",
  "new_bigbutton_up.png",
  "new_bigbutton_hi.png",
  "new_bigbutton_dn.png",
  "new_cancel_images");
 new_button("mask_pnt_linear.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_pnt_linear_images");
 new_button("mask_crv_linear.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_crv_linear_images");
 new_button("mask_all_linear.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_all_linear_images");
 new_button("mask_pnt_smooth.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_pnt_smooth_images");
 new_button("mask_crv_smooth.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_crv_smooth_images");
 new_button("mask_all_smooth.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_all_smooth_images");
 new_button("mask_prst_sqr.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_prst_sqr_images");
 new_button("mask_prst_crc.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_prst_crc_images");
 new_button("mask_prst_tri.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_prst_tri_images");
 new_button("mask_prst_ovl.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_prst_ovl_images");
 new_button("mask_prst_load.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_prst_load_images");
 new_button("mask_prst_save.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_prst_save_images");
 new_button("mask_prst_trsh.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_prst_trsh_images");
 new_button("mask_pstn_cen.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_pstn_cen_images");
 new_button("mask_pstn_nrm.png",
      "mask_button_up.png",
      "mask_button_hi.png",
      "mask_button_dn.png",
      "mask_pstn_nrm_images");
 resources->bar_data = new_image("bar", "bar.png");
 resources->check = new_image("check", "check.png");
 resources->min_menu_w = xS(96);
 resources->menu_popup_bg = new_image("menu_popup_bg.png");
 resources->menu_item_bg = new_image_set(
  3,
  "menuitem_up.png",
  "menuitem_hi.png",
  "menuitem_dn.png");
 resources->menu_bar_bg = new_image("menubar_bg.png");
 resources->menu_title_bg = new_image_set
  (3,
   "menubar_up.png",
   "menubar_hi.png",
   "menubar_dn.png");
 resources->popupmenu_images = 0;
 resources->toggle_highlight_bg = new_image(
  "toggle_highlight_bg",
  "text_highlight.png");
 resources->generic_button_images = new_image_set(
  3,
  "generic_up.png",
  "generic_hi.png",
  "generic_dn.png");
 resources->horizontal_slider_data = new_image_set(
  6,
  "hslider_fg_up.png",
  "hslider_fg_hi.png",
  "hslider_fg_dn.png",
  "hslider_bg_up.png",
  "hslider_bg_hi.png",
  "hslider_bg_dn.png");
 resources->vertical_slider_data = new_image_set(
  6,
  "hslider_fg_up.png",
  "hslider_fg_hi.png",
  "hslider_fg_dn.png",
  "hslider_bg_up.png",
  "hslider_bg_hi.png",
  "hslider_bg_dn.png");
 for( int i=0; i<6; ++i )
  resources->vertical_slider_data[i]->rotate90();
 resources->progress_images = new_image_set(
  2,
  "progress_bg.png",
  "progress_hi.png");
 resources->tumble_data = new_image_set(
  4,
  "tumble_up.png",
  "tumble_hi.png",
  "tumble_bottom.png",
  "tumble_top.png");
 new_image_set("tumblepatch_data", 4,
  "tumblepatch_up.png",
  "tumblepatch_hi.png",
  "tumblepatch_bottom.png",
  "tumblepatch_top.png");
 resources->listbox_button = new_button4(
  "listbox_button.png",
  "editpanel_up.png",
  "editpanel_hi.png",
  "editpanel_dn.png",
  "editpanel_hi.png",
  "listbox_button");
 resources->filebox_szfmt_images = new_image_set(
  12,
  "file_size_zero_up.png",
  "file_size_zero_hi.png",
  "file_size_zero_dn.png",
  "file_size_lwrb_up.png",
  "file_size_lwrb_hi.png",
  "file_size_lwrb_dn.png",
  "file_size_capb_up.png",
  "file_size_capb_hi.png",
  "file_size_capb_dn.png",
  "file_size_semi_up.png",
  "file_size_semi_hi.png",
  "file_size_semi_dn.png");
 resources->listbox_column = new_image_set(
  3,
  "column_up.png",
  "column_hi.png",
  "column_dn.png");
 resources->listbox_up = new_image("listbox_up.png");
 resources->listbox_dn = new_image("listbox_dn.png");
 resources->pan_data = new_image_set(
  7,
  "pan_up.png",
  "pan_hi.png",
  "pan_popup.png",
  "pan_channel.png",
  "pan_stick.png",
  "pan_channel_small.png",
  "pan_stick_small.png");
 resources->pan_text_color = TextFgBright;
 resources->pot_images = new_image_set(
  3,
  "pot_up.png",
  "pot_hi.png",
  "pot_dn.png");
 resources->checkbox_images = new_image_set(
  5,
  "checkbox_up.png",
  "checkbox_hi.png",
  "checkbox_checked.png",
  "checkbox_dn.png",
  "checkbox_checkedhi.png");
 resources->radial_images = new_image_set(
  5,
  "radial_up.png",
  "radial_hi.png",
  "radial_checked.png",
  "radial_dn.png",
  "radial_checkedhi.png");
 resources->xmeter_images = new_image_set(
  7,
  "xmeter_normal.png",
  "xmeter_green.png",
  "xmeter_red.png",
  "xmeter_yellow.png",
  "xmeter_white.png",
  "xmeter_over.png",
  "downmix51_2.png");
 resources->ymeter_images = new_image_set(
  7,
  "ymeter_normal.png",
  "ymeter_green.png",
  "ymeter_red.png",
  "ymeter_yellow.png",
  "ymeter_white.png",
  "ymeter_over.png",
  "downmix51_2.png");
 resources->hscroll_data = new_image_set(
  10,
  "hscroll_handle_up.png",
  "hscroll_handle_hi.png",
  "hscroll_handle_dn.png",
  "hscroll_handle_bg.png",
  "hscroll_left_up.png",
  "hscroll_left_hi.png",
  "hscroll_left_dn.png",
  "hscroll_right_up.png",
  "hscroll_right_hi.png",
  "hscroll_right_dn.png");
 resources->vscroll_data = new_image_set(
  10,
  "vscroll_handle_up.png",
  "vscroll_handle_hi.png",
  "vscroll_handle_dn.png",
  "vscroll_handle_bg.png",
  "vscroll_left_up.png",
  "vscroll_left_hi.png",
  "vscroll_left_dn.png",
  "vscroll_right_up.png",
  "vscroll_right_hi.png",
  "vscroll_right_dn.png");
 resources->scroll_minhandle = xS(20);
 new_button(
  "prevtip.png", "tipbutton_up.png",
  "tipbutton_hi.png", "tipbutton_dn.png", "prev_tip");
 new_button(
  "nexttip.png", "tipbutton_up.png",
  "tipbutton_hi.png", "tipbutton_dn.png", "next_tip");
 new_button(
  "closetip.png", "tipbutton_up.png",
  "tipbutton_hi.png", "tipbutton_dn.png", "close_tip");
 new_button(
  "swap_extents.png", "editpanel_up.png",
  "editpanel_hi.png", "editpanel_dn.png", "swap_extents");
 preferences_category_overlap = 0;
 preferencescategory_x = 0;
 preferencescategory_y = yS(5);
 preferencestitle_x = xS(5);
 preferencestitle_y = yS(10);
 preferencesoptions_x = xS(5);
 preferencesoptions_y = 0;
 message_normal = resources->text_default;
 mtransport_margin = xS(10);
 toggle_margin = xS(10);
 new_button("pane.png", "pane_up.png", "pane_hi.png", "pane_dn.png",
      "pane");
 new_image_set("xpane", 3,
      "xpane_up.png",
      "xpane_hi.png",
      "xpane_dn.png");
 new_image_set("ypane", 3,
      "ypane_up.png",
      "ypane_hi.png",
      "ypane_dn.png");
 new_image("mbutton_bg", "mbutton_bg.png");
 new_image("timebar_bg", "timebar_bg_flat.png");
 new_image("timebar_brender", "timebar_brender.png");
 new_image("clock_bg", "mclock_flat.png");
 new_image("patchbay_bg", "patchbay_bg.png");
 new_image("statusbar", "statusbar.png");
 new_image_set("zoombar_menu", 3,
      "zoompopup_up.png",
      "zoompopup_hi.png",
      "zoompopup_dn.png");
 new_image_set("zoombar_tumbler", 4,
      "zoomtumble_up.png",
      "zoomtumble_hi.png",
      "zoomtumble_bottom.png",
      "zoomtumble_top.png");
new_image_set("auto_range", 4,
      "autorange_up.png",
      "autorange_hi.png",
      "autorange_bottom.png",
      "autorange_top.png");
 new_image_set("mode_popup", 3,
      "mode_up.png", "mode_hi.png", "mode_dn.png");
 new_image("mode_add", "mode_add.png");
 new_image("mode_divide", "mode_divide.png");
 new_image("mode_multiply", "mode_multiply.png");
 new_image("mode_normal", "mode_normal.png");
 new_image("mode_replace", "mode_replace.png");
 new_image("mode_subtract", "mode_subtract.png");
 new_image("mode_max", "mode_max.png");
 new_image_set("plugin_on", 5,
      "plugin_on.png",
      "plugin_onhi.png",
      "plugin_onselect.png",
      "plugin_ondn.png",
      "plugin_onselecthi.png");
 new_image_set("plugin_show", 5,
      "plugin_show.png",
      "plugin_showhi.png",
      "plugin_showselect.png",
      "plugin_showdn.png",
      "plugin_showselecthi.png");
 new_image_set("mixpatch_data", 5,
      "mixpatch_up.png",
      "mixpatch_hi.png",
      "mixpatch_checked.png",
      "mixpatch_dn.png",
      "mixpatch_checkedhi.png");
 new_image("cpanel_bg", "cpanel_bg.png");
 new_image("cbuttons_left", "cbuttons_left.png");
 new_image("cbuttons_right", "cbuttons_right.png");
 new_image("cmeter_bg", "cmeter_bg.png");
 new_image("cwindow_focus", "cwindow_focus.png");
 new_image("vbuttons_left", "vbuttons_left.png");
 new_image("vbuttons_right", "vbuttons_right.png");
 new_image("vclock", "vclock.png");
 new_image("preferences_bg", "preferences_bg.png");
 new_image("new_bg", "new_bg.png");
 new_image("setformat_bg", "setformat_bg.png");
 timebar_view_data = new_image("timebar_view.png");
 setformat_w = get_image("setformat_bg")->get_w();
 setformat_h = get_image("setformat_bg")->get_h();
 setformat_x1 = xS(15);
 setformat_x2 = xS(110);
 setformat_x3 = xS(315);
 setformat_x4 = xS(425);
 setformat_y1 = yS(20);
 setformat_y2 = yS(85);
 setformat_y3 = yS(125);
 setformat_margin = yS(30);
 setformat_channels_x = xS(25);
 setformat_channels_y = yS(242);
 setformat_channels_w = xS(250);
 setformat_channels_h = yS(250);
 loadfile_pad = get_image_set("loadmode_new")[0]->get_h() + yS(10);
 browse_pad = yS(20);
 new_toggle("playpatch.png",
      "playpatch_up.png",
      "playpatch_hi.png",
      "playpatch_checked.png",
      "playpatch_dn.png",
      "playpatch_checkedhi.png",
      "playpatch_data");
 new_toggle("recordpatch.png",
      "recordpatch_up.png",
      "recordpatch_hi.png",
      "recordpatch_checked.png",
      "recordpatch_dn.png",
      "recordpatch_checkedhi.png",
      "recordpatch_data");
 new_toggle("gangpatch.png",
      "patch_up.png",
      "patch_hi.png",
      "patch_checked.png",
      "patch_dn.png",
      "patch_checkedhi.png",
      "gangpatch_data");
 new_toggle("drawpatch.png",
      "patch_up.png",
      "patch_hi.png",
      "patch_checked.png",
      "patch_dn.png",
      "patch_checkedhi.png",
      "drawpatch_data");
 new_toggle("masterpatch.png",
      "patch_up.png",
      "patch_hi.png",
      "patch_checked.png",
      "patch_dn.png",
      "patch_checkedhi.png",
      "masterpatch_data");

 new_image_set("mutepatch_data",
      5,
      "mutepatch_up.png",
      "mutepatch_hi.png",
      "mutepatch_checked.png",
      "mutepatch_dn.png",
      "mutepatch_checkedhi.png");
 new_image_set("expandpatch_data",
      5,
      "expandpatch_up.png",
      "expandpatch_hi.png",
      "expandpatch_checked.png",
      "expandpatch_dn.png",
      "expandpatch_checkedhi.png");
 build_bg_data();
 build_overlays();
 out_point = new_image_set(
  5,
  "out_up.png",
  "out_hi.png",
  "out_checked.png",
  "out_dn.png",
  "out_checkedhi.png");
 in_point = new_image_set(
  5,
  "in_up.png",
  "in_hi.png",
  "in_checked.png",
  "in_dn.png",
  "in_checkedhi.png");
 label_toggle = new_image_set(
  5,
  "labeltoggle_up.png",
  "labeltoggle_uphi.png",
  "label_checked.png",
  "labeltoggle_dn.png",
  "label_checkedhi.png");
 ffmpeg_toggle = new_image_set(
  5,
  "ff_up.png",
  "ff_hi.png",
  "ff_checked.png",
  "ff_down.png",
  "ff_checkedhi.png");
 proxy_p_toggle = new_image_set(
  5,
  "proxy_p_up.png",
  "proxy_p_hi.png",
  "proxy_p_chkd.png",
  "proxy_p_down.png",
  "proxy_p_chkdhi.png");
 proxy_s_toggle = new_image_set(
  5,
  "proxy_s_up.png",
  "proxy_s_hi.png",
  "proxy_s_chkd.png",
  "proxy_s_down.png",
  "proxy_s_chkdhi.png");
 mask_mode_toggle = new_image_set(5,
          "mask_mode_up.png",
          "mask_mode_hi.png",
          "mask_mode_chkd.png",
          "mask_mode_down.png",
          "mask_mode_chkdhi.png");
 shbtn_data = new_image_set(
  3,
  "shbtn_up.png",
  "shbtn_hi.png",
  "shbtn_dn.png");
 new_image_set(
  "preset_edit",
  3,
  "preset_edit0.png",
  "preset_edit1.png",
  "preset_edit2.png");
 new_image_set(
  "histogram_carrot",
  5,
  "histogram_carrot_up.png",
  "histogram_carrot_hi.png",
  "histogram_carrot_checked.png",
  "histogram_carrot_dn.png",
  "histogram_carrot_checkedhi.png");
 statusbar_cancel_data = new_image_set(
  3,
  "statusbar_cancel_up.png",
  "statusbar_cancel_hi.png",
  "statusbar_cancel_dn.png");
 VFrame *editpanel_up = new_image("editpanel_up.png");
 VFrame *editpanel_hi = new_image("editpanel_hi.png");
 VFrame *editpanel_dn = new_image("editpanel_dn.png");
 VFrame *editpanel_checked = new_image("editpanel_checked.png");
 VFrame *editpanel_checkedhi = new_image("editpanel_checkedhi.png");
 new_image("panel_divider", "panel_divider.png");
 new_button("bottom_justify.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "bottom_justify");
 new_button("center_justify.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "center_justify");
 new_button("channel.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "channel");
 new_button("lok.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "lok");
 new_toggle("histogram_toggle.png",
      editpanel_up,
      editpanel_hi,
      editpanel_checked,
      editpanel_dn,
      editpanel_checkedhi,
      "histogram_toggle");
 new_toggle("histogram_rgb.png",
      editpanel_up,
      editpanel_hi,
      editpanel_checked,
      editpanel_dn,
      editpanel_checkedhi,
      "histogram_rgb_toggle");
 new_toggle("waveform.png",
      editpanel_up,
      editpanel_hi,
      editpanel_checked,
      editpanel_dn,
      editpanel_checkedhi,
      "waveform_toggle");
 new_toggle("waveform_rgb.png",
      editpanel_up,
      editpanel_hi,
      editpanel_checked,
      editpanel_dn,
      editpanel_checkedhi,
      "waveform_rgb_toggle");
 new_toggle("scope.png",
      editpanel_up,
      editpanel_hi,
      editpanel_checked,
      editpanel_dn,
      editpanel_checkedhi,
      "scope_toggle");
 new_button("picture.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "picture");
 new_button("histogram_img.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "histogram_img");
 new_button("copy.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "copy");
 new_button("commercial.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "commercial");
 new_button("cut.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "cut");
 new_button("fit.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "fit");
 new_button("fitautos.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "fitautos");
 new_button("inpoint.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "inbutton");
 new_button("label.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "labelbutton");
 new_button("left_justify.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "left_justify");
 new_button("magnify.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "magnify_button");
 new_button("middle_justify.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "middle_justify");
 new_button("nextlabel.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "nextlabel");
 new_button("prevlabel.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "prevlabel");
 new_button("nextedit.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "nextedit");
 new_button("prevedit.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "prevedit");
 new_button("outpoint.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "outbutton");
 over_button = new_button("over.png",
        editpanel_up, editpanel_hi, editpanel_dn,
        "overbutton");
 overwrite_data = new_button("overwrite.png",
        editpanel_up, editpanel_hi, editpanel_dn,
        "overwritebutton");
 new_button("paste.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "paste");
 new_button("redo.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "redo");
 new_button("right_justify.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "right_justify");
 splice_data = new_button("splice.png",
        editpanel_up, editpanel_hi, editpanel_dn,
        "splicebutton");
 new_button("toclip.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "toclip");
 new_button("goto.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "goto");
 new_button("clapper.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "clapperbutton");
 new_button("top_justify.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "top_justify");
 new_button("undo.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "undo");
 new_button("wrench.png",
      editpanel_up, editpanel_hi, editpanel_dn,
      "wrench");

 VFrame **edge_on = new_toggle("edge_on.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi, "edge_on");
 VFrame **edge_off = new_toggle("edge_off.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi, "edge_off");
 new_image_set_images("bump_edge", 5,
      new VFrame(*edge_off[0]), new VFrame(*edge_off[1]),
      new VFrame(*edge_on[0]),  new VFrame(*edge_off[3]),
      new VFrame(*edge_on[4]));
 VFrame **span_on = new_toggle("span_on.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi, "span_on");
 VFrame **span_off = new_toggle("span_off.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi, "span_off");
 new_image_set_images("bump_span", 5,
      new VFrame(*span_off[0]), new VFrame(*span_off[1]),
      new VFrame(*span_on[0]),  new VFrame(*span_off[3]),
      new VFrame(*span_on[4]));

 VFrame *transport_up = new_image("transportup.png");
 VFrame *transport_hi = new_image("transporthi.png");
 VFrame *transport_dn = new_image("transportdn.png");
 new_button("end.png",
      transport_up, transport_hi, transport_dn,
      "end");
 new_button("fastfwd.png",
      transport_up, transport_hi, transport_dn,
      "fastfwd");
 new_button("fastrev.png",
      transport_up, transport_hi, transport_dn,
      "fastrev");
 new_button("play.png",
      transport_up, transport_hi, transport_dn,
      "play");
 new_button("framefwd.png",
      transport_up, transport_hi, transport_dn,
      "framefwd");
 new_button("framerev.png",
      transport_up, transport_hi, transport_dn,
      "framerev");
 new_button("pause.png",
      transport_up, transport_hi, transport_dn,
      "pause");
 new_button("record.png",
      transport_up, transport_hi, transport_dn,
      "record");
 new_button("singleframe.png",
      transport_up, transport_hi, transport_dn,
      "recframe");
 new_button("reverse.png",
      transport_up, transport_hi, transport_dn,
      "reverse");
 new_button("rewind.png",
      transport_up, transport_hi, transport_dn,
      "rewind");
 new_button("stop.png",
      transport_up, transport_hi, transport_dn,
      "stop");
 new_button("stop.png",
      transport_up, transport_hi, transport_dn,
      "stoprec");
 new_image("cwindow_inactive", "cwindow_inactive.png");
 new_image("cwindow_active", "cwindow_active.png");
 new_image_set("category_button",
      3,
      "preferencesbutton_dn.png",
      "preferencesbutton_dnhi.png",
      "preferencesbutton_dnlo.png");
 new_image_set("category_button_checked",
      3,
      "preferencesbutton_up.png",
      "preferencesbutton_uphi.png",
      "preferencesbutton_dnlo.png");
 new_image_set("color3way_point",
      3,
      "color3way_up.png",
      "color3way_hi.png",
      "color3way_dn.png");
 new_toggle("arrow.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "arrow");
 new_toggle("autokeyframe.png",
      transport_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "autokeyframe");
 new_toggle("spankeyframe.png",
      transport_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "spankeyframe");
 new_toggle("ibeam.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "ibeam");
 new_toggle("show_meters.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "meters");
 new_toggle("blank30x30.png",
      new_image("locklabels_locked.png"),
      new_image("locklabels_lockedhi.png"),
      new_image("locklabels_unlocked.png"),
      new_image("locklabels_dn.png"),
      new_image("locklabels_unlockedhi.png"),
      "locklabels");
 new_toggle("gang0.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "gang0");
 new_toggle("gang1.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "gang1");
 new_toggle("gang2.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "gang2");
 VFrame *cpanel_up = new_image("cpanel_up.png");
 VFrame *cpanel_hi = new_image("cpanel_hi.png");
 VFrame *cpanel_dn = new_image("cpanel_dn.png");
 VFrame *cpanel_checked = new_image("cpanel_checked.png");
 VFrame *cpanel_checkedhi = new_image("cpanel_checkedhi.png");
 new_toggle("camera.png",
      cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi,
      "camera");
 new_toggle("crop.png",
      cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi,
      "crop");
 new_toggle("eyedrop.png",
      cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi,
      "eyedrop");
 new_toggle("magnify.png",
      cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi,
      "magnify");
 new_toggle("mask.png",
      cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi,
      "mask");
 new_toggle("ruler.png",
      cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi,
      "ruler");
 new_toggle("projector.png",
      cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi,
      "projector");
 new_toggle("protect.png",
      cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi,
      "protect");
 new_toggle("titlesafe.png",
      cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi,
      "titlesafe");
 new_toggle("toolwindow.png",
      cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi,
      "tool");
 new_toggle("tan_smooth.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "tan_smooth");
 new_toggle("tan_linear.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "tan_linear");
 new_toggle("tan_tangent.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "tan_tangent");
 new_toggle("tan_free.png",
       editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "tan_free");
 new_toggle("tan_bump.png",
      editpanel_up, editpanel_hi, editpanel_checked,
      editpanel_dn, editpanel_checkedhi,
      "tan_bump");
 new_toggle("mask_scale_x.png", "mask_scale_up.png", "mask_scale_uphi.png",
      "mask_scale_chkd.png", "mask_scale_xdown.png", "mask_scale_chkdhi.png",
      "mask_scale_x");
 new_toggle("mask_scale_y.png", "mask_scale_up.png", "mask_scale_uphi.png",
      "mask_scale_chkd.png", "mask_scale_ydown.png", "mask_scale_chkdhi.png",
      "mask_scale_y");
 new_toggle("mask_scale_xy.png", "mask_scale_up.png", "mask_scale_uphi.png",
      "mask_scale_chkd.png", "mask_scale_xydown.png", "mask_scale_chkdhi.png",
      "mask_scale_xy");
 flush_images();
}
void CAKEWALKTHEME::get_vwindow_sizes(VWindowGUI *gui)
{
 int edit_w = EditPanel::calculate_w(mwindow, 0, 12);
 int transport_w = PlayTransport::get_transport_width(mwindow) + toggle_margin;
 vtimebar_h = yS(16);
 int division_w = xS(30);
 vtime_w = xS(140);
 int vtime_border = xS(15);
 vmeter_y = widget_border;
 vmeter_h = mwindow->session->vwindow_h - cmeter_y - widget_border;
 int buttons_h;
 if (mwindow->edl->session->vwindow_meter) {
  vmeter_x = mwindow->session->vwindow_w -
   MeterPanel::get_meters_width(this,
           mwindow->edl->session->audio_channels,
           mwindow->edl->session->vwindow_meter);
 } else {
  vmeter_x = mwindow->session->vwindow_w + widget_border;
 }
 vcanvas_x = 0;
 vcanvas_y = 0;
 vcanvas_w = vmeter_x - vcanvas_x - widget_border;
 if (edit_w +
  widget_border * 2 +
  transport_w + widget_border +
  vtime_w + division_w +
  vtime_border > vmeter_x) {
  buttons_h = get_image("vbuttons_left")->get_h();
  vedit_x = widget_border;
  vedit_y = mwindow->session->vwindow_h -
   buttons_h +
   vtimebar_h +
   widget_border;
  vtransport_x = widget_border;
  vtransport_y = mwindow->session->vwindow_h -
   get_image_set("autokeyframe")[0]->get_h() -
   widget_border;
  vdivision_x = xS(280);
  vtime_x = vedit_x + xS(85);
  vtime_y = vedit_y + yS(28);
 } else {
  buttons_h = vtimebar_h +
   widget_border +
   EditPanel::calculate_h(mwindow) +
   widget_border;
  vtransport_x = widget_border;
  vtransport_y = mwindow->session->vwindow_h -
   buttons_h +
   vtimebar_h +
   widget_border;
  vedit_x = vtransport_x + transport_w + widget_border;
  vedit_y = vtransport_y;
  vdivision_x = vedit_x + edit_w + division_w;
  vtime_x = vdivision_x + vtime_border;
  vtime_y = vedit_y + widget_border;
 }
 vcanvas_h = mwindow->session->vwindow_h - buttons_h;
 vtimebar_x = 0;
 vtimebar_y = vcanvas_y + vcanvas_h;
 vtimebar_w = vmeter_x - widget_border;
}
void CAKEWALKTHEME::build_bg_data()
{
 channel_position_data = new VFramePng(get_image_data("channel_position.png"));
 new_image1("resource1024", "resource1024.png");
 new_image1("resource512", "resource512.png");
 new_image1("resource256", "resource256.png");
 new_image1("resource128", "resource128.png");
 new_image1("resource64", "resource64.png");
 new_image1("resource32", "resource32.png");
 new_image("plugin_bg_data", "plugin_bg.png");
 new_image("title_bg_data", "title_bg.png");
 new_image("vtimebar_bg_data", "vwindow_timebar.png");
}
void CAKEWALKTHEME::build_overlays()
{
 keyframe_data = new VFramePng(get_image_data("keyframe3.png"));
 camerakeyframe_data = new VFramePng(get_image_data("camerakeyframe.png"));
 maskkeyframe_data = new VFramePng(get_image_data("maskkeyframe.png"));
 modekeyframe_data = new VFramePng(get_image_data("modekeyframe.png"));
 hardedge_data = new VFramePng(get_image_data("hardedge.png"));
 pankeyframe_data = new VFramePng(get_image_data("pankeyframe.png"));
 projectorkeyframe_data = new VFramePng(get_image_data("projectorkeyframe.png"));
}
void CAKEWALKTHEME::draw_rwindow_bg(RecordGUI *gui)
{
}
void CAKEWALKTHEME::draw_rmonitor_bg(RecordMonitorGUI *gui)
{
}
void CAKEWALKTHEME::draw_mwindow_bg(MWindowGUI *gui)
{
 gui->draw_3segmenth(mbuttons_x, mbuttons_y - 1,
      gui->menu_w(), get_image("mbutton_bg"));
 int pdw = get_image("panel_divider")->get_w();
 int x = mbuttons_x;
 x += 9 * get_image("play")->get_w();
 x += mtransport_margin;
 int xs2 = xS(2);
 gui->draw_vframe(get_image("panel_divider"),
      x - toggle_margin / 2 - pdw / 2 + xs2,
      mbuttons_y - 1);
 x += 2 * get_image("arrow")->get_w() + toggle_margin;
 gui->draw_vframe(get_image("panel_divider"),
      x - toggle_margin / 2 - pdw / 2 + xs2,
      mbuttons_y - 1);
 x += 2 * get_image("autokeyframe")->get_w() + toggle_margin;
 gui->draw_vframe(get_image("panel_divider"),
      x - toggle_margin / 2 - pdw / 2 + xs2,
      mbuttons_y - 1);
 gui->draw_3segmenth(0,
      mbuttons_y - 1 + get_image("mbutton_bg")->get_h(),
      get_image("patchbay_bg")->get_w(),
      get_image("clock_bg"));
 gui->draw_3segmentv(patchbay_x,
      patchbay_y,
      patchbay_h,
      get_image("patchbay_bg"));
 gui->set_color(BLACK);
 gui->draw_box(mcanvas_x + get_image("patchbay_bg")->get_w(),
      mcanvas_y + mtimebar_h,
      mcanvas_w - BC_ScrollBar::get_span(SCROLL_VERT),
      mcanvas_h - BC_ScrollBar::get_span(SCROLL_HORIZ) - mtimebar_h);
 gui->draw_3segmenth(mtimebar_x,
      mtimebar_y,
      mtimebar_w,
      get_image("timebar_bg"));
 gui->set_color(CwDarkGray);
 gui->draw_box(mzoom_x, mzoom_y,
      mwindow->session->mwindow_w, yS(25));
 gui->draw_3segmenth(mzoom_x,
      mzoom_y,
      mzoom_w,
      get_image("statusbar"));
}
void CAKEWALKTHEME::draw_cwindow_bg(CWindowGUI *gui)
{
 gui->draw_3segmentv(0, 0, ccomposite_h, get_image("cpanel_bg"));
 gui->draw_3segmenth(0, ccomposite_h, cstatus_x, get_image("cbuttons_left"));
 if(mwindow->edl->session->cwindow_meter)
 {
  gui->draw_3segmenth(cstatus_x,
       ccomposite_h,
       cmeter_x - widget_border - cstatus_x,
       get_image("cbuttons_right"));
  gui->draw_9segment(cmeter_x - widget_border,
         0,
         mwindow->session->cwindow_w - cmeter_x + widget_border,
         mwindow->session->cwindow_h,
         get_image("cmeter_bg"));
 } else {
  gui->draw_3segmenth(cstatus_x,
       ccomposite_h,
       cmeter_x - widget_border - cstatus_x + xS(100),
       get_image("cbuttons_right"));
 }
}
void CAKEWALKTHEME::draw_vwindow_bg(VWindowGUI *gui)
{
 gui->draw_3segmenth(0,
      vcanvas_h,
      vdivision_x,
      get_image("vbuttons_left"));
 if(mwindow->edl->session->vwindow_meter)
 {
  gui->draw_3segmenth(vdivision_x,
       vcanvas_h,
       vmeter_x - widget_border - vdivision_x,
       get_image("vbuttons_right"));
  gui->draw_9segment(vmeter_x - widget_border,
         0,
         mwindow->session->vwindow_w - vmeter_x + widget_border,
         mwindow->session->vwindow_h,
         get_image("cmeter_bg"));
 } else {
  gui->draw_3segmenth(vdivision_x,
       vcanvas_h,
       vmeter_x - widget_border - vdivision_x + xS(100),
       get_image("vbuttons_right"));
 }
 gui->draw_3segmenth(vtime_x - xS(5), vtime_y + 0,
  vtime_w + xS(10), get_image("vclock"));
}
void CAKEWALKTHEME::draw_preferences_bg(PreferencesWindow *gui)
{
 gui->draw_vframe(get_image("preferences_bg"), 0, 0);
}
void CAKEWALKTHEME::draw_new_bg(NewWindow *gui)
{
 gui->draw_vframe(get_image("new_bg"), 0, 0);
}
void CAKEWALKTHEME::draw_setformat_bg(SetFormatWindow *gui)
{
 gui->draw_vframe(get_image("setformat_bg"), 0, 0);
}
