
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "clip.h"
#include "language.h"
#include "loadmode.h"
#include "mwindow.h"
#include "theme.h"

// Must match macros
static const char *mode_images[] =
{
	"loadmode_none",
	"loadmode_new",
	"loadmode_newcat",
	"loadmode_newtracks",
	"loadmode_cat",
	"loadmode_paste",
	"loadmode_resource",
	"loadmode_nested"
};

static const char *mode_text[] =
{
	N_("Insert nothing"),
	N_("Replace current project"),
	N_("Replace current project and concatenate tracks"),
	N_("Append in new tracks"),
	N_("Concatenate to existing tracks"),
	N_("Paste over selection/at insertion point"),
	N_("Create new resources only"),
	N_("Nest sequence")
};


LoadModeItem::LoadModeItem(const char *text, int value)
 : BC_ListBoxItem(text)
{
	this->value = value;
}


LoadModeToggle::LoadModeToggle(int x, int y, LoadMode *window,
		int value, const char *images, const char *tooltip)
 : BC_Toggle(x, y, window->mwindow->theme->get_image_set(images),
		*window->output == value)
{
	this->window = window;
	this->value = value;
	set_tooltip(tooltip);
}

int LoadModeToggle::handle_event()
{
	*window->output = value;
	window->update();
	return 1;
}



LoadMode::LoadMode(MWindow *mwindow,
		BC_WindowBase *window, int x, int y, int *output,
		int use_nothing, int use_nested, int line_wrap)
{
	this->mwindow = mwindow;
	this->window = window;
	this->x = x;
	this->y = y;
	this->output = output;
	this->use_nothing = use_nothing;
	this->use_nested = use_nested;
	this->line_wrap = line_wrap;
	for( int i=0; i<TOTAL_LOADMODES; ++i ) mode[i] = 0;
}

LoadMode::~LoadMode()
{
	delete title;
	delete textbox;
	delete listbox;
	load_modes.remove_all_objects();
	for( int i=0; i<TOTAL_LOADMODES; ++i ) delete mode[i];
}

const char *LoadMode::mode_to_text()
{
	for( int i=0; i<load_modes.total; ++i ) {
		if( load_modes[i]->value == *output )
			return load_modes[i]->get_text();
	}
	return _("Unknown");
}

void LoadMode::load_mode_geometry(BC_WindowBase *gui, Theme *theme,
		int use_nothing, int use_nested, int line_wrap,
		int *pw, int *ph)
{
	int pad = 5;
	const char *title_text = _("Insertion strategy:");
	int mw = BC_Title::calculate_w(gui, title_text);
	int mh = BC_Title::calculate_h(gui, title_text);
	int ix = mw + 2*pad, iy = 0, x1 = ix;
	int ww = theme->loadmode_w + 24;
	if( mw < ww ) mw = ww;

	for( int i=0; i<TOTAL_LOADMODES; ++i ) {
		if( i == LOADMODE_NOTHING && !use_nothing) continue;
		if( i == LOADMODE_NESTED && !use_nested) continue;
		int text_line, w, h, toggle_x, toggle_y;
		int text_x, text_y, text_w, text_h;
		BC_Toggle::calculate_extents(gui,
			theme->get_image_set(mode_images[i]), 0,
			&text_line, &w, &h, &toggle_x, &toggle_y,
			&text_x, &text_y, &text_w, &text_h, 0, MEDIUMFONT);
		if( line_wrap && ix+w > ww ) { ix = x1;  iy += h+pad; }
		if( (ix+=w) > mw ) mw = ix;
		if( (h+=iy) > mh ) mh = h;
		ix += pad;
	}

	ix = 0;  iy = mh+pad;
	mh = iy + BC_TextBox::calculate_h(gui, MEDIUMFONT, 1, 1);
	if( pw ) *pw = mw;
	if( ph ) *ph = mh;
}

int LoadMode::calculate_w(BC_WindowBase *gui, Theme *theme,
		int use_nothing, int use_nested, int line_wrap)
{
	int result = 0;
	load_mode_geometry(gui, theme, use_nothing, use_nested, line_wrap,
			&result, 0);
	return result;
}

int LoadMode::calculate_h(BC_WindowBase *gui, Theme *theme,
		int use_nothing, int use_nested, int line_wrap)
{
	int result = 0;
	load_mode_geometry(gui, theme, use_nothing, use_nested, line_wrap,
			0, &result);
	return result;
}

void LoadMode::create_objects()
{
	int pad = 5;
	const char *title_text = _("Insertion strategy:");
	window->add_subwindow(title = new BC_Title(x, y, title_text));
	int mw = title->get_w(), mh = title->get_h();
	int ix = mw + 2*pad, iy = 0, x1 = ix;
	int ww = mwindow->theme->loadmode_w + 24;
	if( mw < ww ) mw = ww;

	for( int i=0; i<TOTAL_LOADMODES; ++i ) {
		if( i == LOADMODE_NOTHING && !use_nothing) continue;
		if( i == LOADMODE_NESTED && !use_nested) continue;
		load_modes.append(new LoadModeItem(_(mode_text[i]), i));
		int text_line, w, h, toggle_x, toggle_y;
		int text_x, text_y, text_w, text_h;
		BC_Toggle::calculate_extents(window,
			mwindow->theme->get_image_set(mode_images[i]), 0,
			&text_line, &w, &h, &toggle_x, &toggle_y,
			&text_x, &text_y, &text_w, &text_h, 0, MEDIUMFONT);
		if( line_wrap && ix+w > ww ) { ix = x1;  iy += h+pad; }
		mode[i] = new LoadModeToggle(x+ix, y+iy, this,
			i, mode_images[i], _(mode_text[i]));
		window->add_subwindow(mode[i]);
		if( (ix+=w) > mw ) mw = ix;
		if( (h+=iy) > mh ) mh = h;
		ix += pad;
	}

	ix = 0;  iy = mh+pad;
	const char *mode_text = mode_to_text();
	textbox = new BC_TextBox(x+ix, y+iy,
		mwindow->theme->loadmode_w, 1, mode_text);
	window->add_subwindow(textbox);
	ix += textbox->get_w();
	listbox = new LoadModeListBox(window, this, x+ix, y+iy);
	window->add_subwindow(listbox);
	mh = iy + textbox->get_h();
}

int LoadMode::reposition_window(int x, int y)
{
	this->x = x;  this->y = y;
	title->reposition_window(x, y);
	int mw = title->get_w(), mh = title->get_h();
	int pad = 5;
	int ix = mw + 2*pad, iy = 0, x1 = ix;
	int ww = mwindow->theme->loadmode_w + 24;
	if( mw < ww ) mw = ww;

	for( int i=0; i<TOTAL_LOADMODES; ++i ) {
		if( i == LOADMODE_NOTHING && !use_nothing) continue;
		if( i == LOADMODE_NESTED && !use_nested) continue;
		int text_line, w, h, toggle_x, toggle_y;
		int text_x, text_y, text_w, text_h;
		BC_Toggle::calculate_extents(window,
			mwindow->theme->get_image_set(mode_images[i]), 0,
			&text_line, &w, &h, &toggle_x, &toggle_y,
			&text_x, &text_y, &text_w, &text_h, 0, MEDIUMFONT);
		if( line_wrap && ix+w > ww ) { ix = x1;  iy += h+pad; }
		mode[i]->reposition_window(x+ix, y+iy);
		if( (ix+=w) > mw ) mw = ix;
		if( (h+=iy) > mh ) mh = h;
		ix += pad;
	}

	ix = 0;  iy = mh+pad;
	textbox->reposition_window(x+ix, y+iy);
	ix += textbox->get_w();
	listbox->reposition_window(x+ix, y+iy);
	return 0;
}

int LoadMode::get_h()
{
	int result = 0;
	load_mode_geometry(window, mwindow->theme,
			use_nothing, use_nested, line_wrap, 0, &result);
	return result;
}

int LoadMode::get_x()
{
	return x;
}

int LoadMode::get_y()
{
	return y;
}

void LoadMode::update()
{
	for( int i=0; i<TOTAL_LOADMODES; ++i ) {
		if( !mode[i] ) continue;
		mode[i]->set_value(*output == i);
	}
	textbox->update(mode_to_text());
}

int LoadMode::set_line_wrap(int v)
{
	int ret = line_wrap;
	line_wrap = v;
	return ret;
}

LoadModeListBox::LoadModeListBox(BC_WindowBase *window, LoadMode *loadmode,
		int x, int y)
 : BC_ListBox(x, y, loadmode->mwindow->theme->loadmode_w, 150, LISTBOX_TEXT,
	(ArrayList<BC_ListBoxItem *>*)&loadmode->load_modes, 0, 0, 1, 0, 1)
{
	this->window = window;
	this->loadmode = loadmode;
}

LoadModeListBox::~LoadModeListBox()
{
}

int LoadModeListBox::handle_event()
{
	LoadModeItem *item = (LoadModeItem *)get_selection(0, 0);
	if( item ) {
		*(loadmode->output) = item->value;
		loadmode->update();
	}
	return 1;
}

