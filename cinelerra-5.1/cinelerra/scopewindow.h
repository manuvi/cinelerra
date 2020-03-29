/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef SCOPEWINDOW_H
#define SCOPEWINDOW_H


#include "guicast.h"
#include "loadbalance.h"
#include "mwindow.h"
#include "pluginclient.h"
#include "recordmonitor.inc"
#include "scopewindow.inc"
#include "theme.inc"

enum {
	SCOPE_HISTOGRAM, SCOPE_HISTOGRAM_RGB,
	SCOPE_WAVEFORM, SCOPE_WAVEFORM_RGB, SCOPE_WAVEFORM_PLY,
	SCOPE_VECTORSCOPE,
};

// Number of divisions in histogram.
// 65536 + min and max range to speed up the tabulation
#define TOTAL_BINS 0x13333
#define HIST_SECTIONS 4
#define FLOAT_RANGE 1.2
// Minimum value in percentage
#define HISTOGRAM_MIN -10
#define FLOAT_MIN -0.1
// Maximum value in percentage
#define HISTOGRAM_MAX 110
#define FLOAT_MAX 1.1

#define MIN_SCOPE_W xS(640)
#define MIN_SCOPE_H yS(320)


#define WAVEFORM_DIVISIONS 12
#define VECTORSCOPE_DIVISIONS 11

class ScopePackage : public LoadPackage
{
public:
	ScopePackage();
	int row1, row2;
};


class ScopeUnit : public LoadClient
{
public:
	ScopeUnit(ScopeGUI *gui, ScopeEngine *server);
	void process_package(LoadPackage *package);
	int bins[HIST_SECTIONS][TOTAL_BINS];
	ScopeGUI *gui;
};

class ScopeEngine : public LoadServer
{
public:
	ScopeEngine(ScopeGUI *gui, int cpus);
	virtual ~ScopeEngine();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	void process();
	ScopeGUI *gui;
};

class ScopePanel : public BC_SubWindow
{
public:
	ScopePanel(ScopeGUI *gui, int x, int y, int w, int h);
	void create_objects();
	virtual void update_point(int x, int y);
	virtual void draw_point();
	virtual void clear_point();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	int is_dragging;
	ScopeGUI *gui;
};

class ScopeWaveform : public ScopePanel
{
public:
	ScopeWaveform(ScopeGUI *gui, int x, int y, int w, int h);
	virtual void update_point(int x, int y);
	virtual void draw_point();
	virtual void clear_point();
	int drag_x;
	int drag_y;
};


class ScopeVectorscope : public ScopePanel
{
public:
	ScopeVectorscope(ScopeGUI *gui, int x, int y, int w, int h);
	virtual void update_point(int x, int y);
	virtual void draw_point();
	virtual void clear_point();
	int drag_radius;
	float drag_angle;
};

class ScopeHistogram : public ScopePanel
{
public:
	ScopeHistogram(ScopeGUI *gui, int x, int y, int w, int h);
	void clear_point();
	void update_point(int x, int y);
	void draw_point();
	void draw(int flash, int flush);
	void draw_mode(int mode, int color, int y, int h);
	int drag_x;
};


class ScopeScopesOn : public BC_MenuItem
{
public:
	ScopeScopesOn(ScopeMenu *scope_menu, const char *text, int id);
	int handle_event();

	ScopeMenu *scope_menu;
	int id;
};

class ScopeMenu : public BC_PopupMenu
{
public:
	ScopeMenu(ScopeGUI *gui, int x, int y);
	void create_objects();
	void update_toggles();

	ScopeGUI *gui;
	ScopeScopesOn *hist_on;
	ScopeScopesOn *hist_rgb_on;
	ScopeScopesOn *wave_on;
	ScopeScopesOn *wave_rgb_on;
	ScopeScopesOn *wave_ply_on;
	ScopeScopesOn *vect_on;
};

class ScopeWaveSlider : public BC_ISlider
{
public:
	ScopeWaveSlider(ScopeGUI *gui, int x, int y, int w);
	int handle_event();
	ScopeGUI *gui;
};

class ScopeVectSlider : public BC_ISlider
{
public:
	ScopeVectSlider(ScopeGUI *gui, int x, int y, int w);
	int handle_event();
	ScopeGUI *gui;
};

class ScopeSmooth : public BC_CheckBox
{
public:
	ScopeSmooth(ScopeGUI *gui, int x, int y);
	int handle_event();
	ScopeGUI *gui;
};


class ScopeGUI : public PluginClientWindow
{
public:
	ScopeGUI(Theme *theme, int x, int y, int w, int h, int cpus);
	ScopeGUI(PluginClient *plugin, int w, int h);
	virtual ~ScopeGUI();

	void reset();
	virtual void create_objects();
	void create_panels();
	virtual int resize_event(int w, int h);
	virtual int translation_event();
	virtual void update_scope() {}

// Called for user storage when toggles change
	virtual void toggle_event();

// update toggles
	void update_toggles();
	void calculate_sizes(int w, int h);
	void allocate_vframes();
	void draw_overlays(int overlays, int borders, int flush);
	void process(VFrame *output_frame);
	void draw(int flash, int flush);
	void clear_points(int flash);

	Theme *theme;
	VFrame *output_frame;
	VFrame *data_frame, *temp_frame;
	ScopeEngine *engine;
	BoxBlur *box_blur;
	VFrame *waveform_vframe;
	VFrame *vector_vframe;
	ScopeHistogram *histogram;
	ScopeWaveform *waveform;
	ScopeVectorscope *vectorscope;
	ScopeMenu *scope_menu;
	ScopeWaveSlider *wave_slider;
	ScopeVectSlider *vect_slider;
	ScopeSmooth *smooth;
	BC_Title *value_text;

	int x, y, w, h;
	int vector_x, vector_y, vector_w, vector_h;
	int wave_x, wave_y, wave_w, wave_h;
	int hist_x, hist_y, hist_w, hist_h;

	int cpus;
	int use_hist, use_wave, use_vector;
	int use_hist_parade, use_wave_parade;

	int bins[HIST_SECTIONS][TOTAL_BINS];
	int frame_w, use_smooth;
	int use_wave_gain, use_vect_gain;
};


class BoxBlurPackage : public LoadPackage
{
public:
	BoxBlurPackage();
	int u1, u2;
};

class BoxBlurUnit : public LoadClient
{
public:
	BoxBlurUnit(BoxBlur*server);
	template<class dst_t, class src_t>
		void blurt_package(LoadPackage *package);
	void process_package(LoadPackage *package);
};

class BoxBlur : public LoadServer
{
public:
	BoxBlur(int cpus);
	virtual ~BoxBlur();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	void process(VFrame *dst, VFrame *src, int uv,
		int radius, int power, int comp);
	void hblur(VFrame *dst, VFrame *src,
		int radius, int power, int comp=-1);
	void vblur(VFrame *dst, VFrame *src,
		int radius, int power, int comp=-1);
	void blur(VFrame *dst, VFrame *src,
		int radius, int power, int comp=-1);
	const uint8_t *src_data;
	uint8_t *dst_data;
	int src_ustep, dst_ustep;
	int src_vstep, dst_vstep;
	int radius, power;
	int ulen, vlen, c0, c1;
};

#endif
