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

#include "bcsignals.h"
#include "bccolors.h"
#include "clip.h"
#include "cursors.h"
#include "language.h"
#include "scopewindow.h"
#include "theme.h"

#include <string.h>

ScopePackage::ScopePackage()
 : LoadPackage()
{
}

ScopeUnit::ScopeUnit(ScopeGUI *gui,
	ScopeEngine *server)
 : LoadClient(server)
{
	this->gui = gui;
}

#define BPP 3
#define incr_point(rows,h, n, comp) \
if(iy >= 0 && iy < h) { int v; \
  uint8_t *vp = rows[iy] + ix*BPP + (comp); \
  v = *vp+(n);  *vp = v>0xff ? 0xff : v; \
}
#define incr_points(rows,h, rv, gv, bv) \
if(iy >= 0 && iy < h) { int v; \
  uint8_t *vp = rows[iy] + ix*BPP; \
  v = *vp+(rv);  *vp++ = v>0xff ? 0xff : v; \
  v = *vp+(gv);  *vp++ = v>0xff ? 0xff : v; \
  v = *vp+(bv);  *vp   = v>0xff ? 0xff : v; \
}
#define clip_points(rows,h, rv, gv, bv) \
if(iy >= 0 && iy < h) { int v; \
  uint8_t *vp = rows[iy] + ix*BPP; \
  v = *vp+(rv);  *vp++ = v>0xff ? 0xff : v<0 ? 0 : v; \
  v = *vp+(gv);  *vp++ = v>0xff ? 0xff : v<0 ? 0 : v; \
  v = *vp+(bv);  *vp   = v>0xff ? 0xff : v<0 ? 0 : v; \
}

#define PROCESS_PIXEL(column) { \
/* Calculate histogram */ \
	if(use_hist) { \
		int v_i = (intensity - FLOAT_MIN) * (TOTAL_BINS / (FLOAT_MAX - FLOAT_MIN)); \
		CLAMP(v_i, 0, TOTAL_BINS - 1); \
		bins[3][v_i]++; \
	} \
	if(use_hist_parade) { \
		int r_i = (r - FLOAT_MIN) * (TOTAL_BINS / (FLOAT_MAX - FLOAT_MIN)); \
		int g_i = (g - FLOAT_MIN) * (TOTAL_BINS / (FLOAT_MAX - FLOAT_MIN)); \
		int b_i = (b - FLOAT_MIN) * (TOTAL_BINS / (FLOAT_MAX - FLOAT_MIN)); \
		CLAMP(r_i, 0, TOTAL_BINS - 1); \
		CLAMP(g_i, 0, TOTAL_BINS - 1); \
		CLAMP(b_i, 0, TOTAL_BINS - 1); \
		bins[0][r_i]++; \
		bins[1][g_i]++; \
		bins[2][b_i]++; \
	} \
/* Calculate waveform */ \
	if(use_wave || use_wave_parade) { \
		int ix = (column) * wave_w / out_w; \
		if(ix >= 0 && ix < wave_w) { \
			if(use_wave_parade > 0) { \
				ix /= 3; \
				int iy = wave_h - (int)((r - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				incr_point(waveform_rows,wave_h, winc, 0); \
				ix += wave_w/3; \
				iy = wave_h - (int)((g - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				incr_point(waveform_rows,wave_h, winc, 1); \
				ix += wave_w/3; \
				iy = wave_h - (int)((b - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				incr_point(waveform_rows,wave_h, winc, 2); \
			} \
			else if(use_wave_parade < 0) { \
				int iy = wave_h - (int)((r - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				clip_points(waveform_rows,wave_h, winc, -winc, -winc); \
				iy = wave_h - (int)((g - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				clip_points(waveform_rows,wave_h, -winc, winc, -winc); \
				iy = wave_h - (int)((b - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				clip_points(waveform_rows,wave_h, -winc, -winc, winc); \
			} \
			else { int yinc = 3*winc; \
				int rinc = yinc*(r-FLOAT_MIN) / (FLOAT_MAX-FLOAT_MIN) + 3; \
				int ginc = yinc*(g-FLOAT_MIN) / (FLOAT_MAX-FLOAT_MIN) + 3; \
				int binc = yinc*(b-FLOAT_MIN) / (FLOAT_MAX-FLOAT_MIN) + 3; \
				int iy = wave_h - ((intensity - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				incr_points(waveform_rows,wave_h, rinc, ginc, binc); \
			} \
		} \
	} \
/* Calculate vectorscope */ \
	if(use_vector) { \
		float th = 90 - h; \
		if( th < 0 ) th += 360; \
		double t = TO_RAD(th); \
		float adjacent = -cos(t), opposite = sin(t); \
		int ix = vector_w / 2 + adjacent * (s) / (FLOAT_MAX) * radius; \
		int iy = vector_h / 2 - opposite * (s) / (FLOAT_MAX) * radius; \
		CLAMP(ix, 0, vector_w - 1); \
	/* Get color with full saturation & value */ \
		float r_f, g_f, b_f; \
		if( (h=h-90) < 0 ) h += 360; \
		HSV::hsv_to_rgb(r_f, g_f, b_f, h, s, 1); \
		int rv = CLIP(r_f, 0, 1) * vinc + 3; \
		int gv = CLIP(g_f, 0, 1) * vinc + 3; \
		int bv = CLIP(b_f, 0, 1) * vinc + 3; \
		incr_points(vector_rows,vector_h, rv, gv, bv); \
	} \
}

#define PROCESS_RGB_PIXEL(column, max) { \
	r = (float)*row++ / max; \
	g = (float)*row++ / max; \
	b = (float)*row++ / max; \
	HSV::rgb_to_hsv(r, g, b, h, s, v); \
	intensity = v; \
	PROCESS_PIXEL(column) \
}

#define PROCESS_BGR_PIXEL(column, max) { \
	b = (float)*row++ / max; \
	g = (float)*row++ / max; \
	r = (float)*row++ / max; \
	HSV::rgb_to_hsv(r, g, b, h, s, v); \
	intensity = v; \
	PROCESS_PIXEL(column) \
}

#define PROCESS_YUV_PIXEL(column, y_in, u_in, v_in) { \
	YUV::yuv.yuv_to_rgb_f(r, g, b, y_in, u_in, v_in); \
	HSV::rgb_to_hsv(r, g, b, h, s, v); \
	intensity = v; \
	PROCESS_PIXEL(column) \
}

void ScopeUnit::process_package(LoadPackage *package)
{
	ScopePackage *pkg = (ScopePackage*)package;

	float r, g, b;
	float h, s, v;
	float intensity;
	int use_hist = gui->use_hist;
	int use_hist_parade = gui->use_hist_parade;
	int use_vector = gui->use_vector;
	int use_wave = gui->use_wave;
	int use_wave_parade = gui->use_wave_parade;
	VFrame *waveform_vframe = gui->waveform_vframe;
	VFrame *vector_vframe = gui->vector_vframe;
	int wave_h = waveform_vframe->get_h();
	int wave_w = waveform_vframe->get_w();
	int vector_h = vector_vframe->get_h();
	int vector_w = vector_vframe->get_w();
	int out_w = gui->output_frame->get_w();
	int out_h = gui->output_frame->get_h();
	int winc = (wave_w * wave_h) / (out_w * out_h);
	if( use_wave_parade ) winc *= 3;
	winc += 2;  winc *= gui->wdial;
	int vinc = 3*(vector_w * vector_h) / (out_w * out_h) + 2;
	vinc *= gui->vdial;
	float radius = MIN(gui->vector_w / 2, gui->vector_h / 2);
	unsigned char **waveform_rows = waveform_vframe->get_rows();
	unsigned char **vector_rows = vector_vframe->get_rows();
	unsigned char **rows = gui->output_frame->get_rows();

	switch( gui->output_frame->get_color_model() ) {
	case BC_RGB888:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			unsigned char *row = rows[y];
			for( int x=0; x<out_w; ++x ) {
				PROCESS_RGB_PIXEL(x, 255)
			}
		}
		break;
	case BC_RGBA8888:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			unsigned char *row = rows[y];
			for( int x=0; x<out_w; ++x ) {
				PROCESS_RGB_PIXEL(x, 255)
				++row;
			}
		}
		break;
	case BC_BGR888:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			unsigned char *row = rows[y];
			for( int x=0; x<out_w; ++x ) {
				PROCESS_BGR_PIXEL(x, 255)
			}
		}
		break;
	case BC_BGR8888:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			unsigned char *row = rows[y];
			for( int x=0; x<out_w; ++x ) {
				PROCESS_BGR_PIXEL(x, 255)
				++row;
			}
		}
		break;
	case BC_RGB_FLOAT:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			float *row = (float*)rows[y];
			for( int x=0; x<out_w; ++x ) {
				PROCESS_RGB_PIXEL(x, 1.0)
			}
		}
		break;
	case BC_RGBA_FLOAT:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			float *row = (float*)rows[y];
			for( int x=0; x<out_w; ++x ) {
				PROCESS_RGB_PIXEL(x, 1.0)
				++row;
			}
		}
		break;
	case BC_YUV888:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			unsigned char *row = rows[y];
			for( int x=0; x<out_w; ++x ) {
				PROCESS_YUV_PIXEL(x, row[0], row[1], row[2])
				row += 3;
			}
		}
		break;

	case BC_YUVA8888:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			unsigned char *row = rows[y];
			for( int x=0; x<out_w; ++x ) {
				PROCESS_YUV_PIXEL(x, row[0], row[1], row[2])
				row += 4;
			}
		}
		break;
	case BC_YUV420P: {
		unsigned char *yp = gui->output_frame->get_y();
		unsigned char *up = gui->output_frame->get_u();
		unsigned char *vp = gui->output_frame->get_v();
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			unsigned char *y_row = yp + y * out_w;
			unsigned char *u_row = up + (y / 2) * (out_w / 2);
			unsigned char *v_row = vp + (y / 2) * (out_w / 2);
			for( int x=0; x<out_w; x+=2 ) {
				PROCESS_YUV_PIXEL(x, *y_row, *u_row, *v_row);
				++y_row;
				PROCESS_YUV_PIXEL(x + 1, *y_row, *u_row, *v_row);
				++y_row;
				++u_row;  ++v_row;
			}
		}
		break; }
	case BC_YUV422:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			unsigned char *row = rows[y];
			for( int x=0; x<out_w; x+=2 ) {
				PROCESS_YUV_PIXEL(x, row[0], row[1], row[3]);
				PROCESS_YUV_PIXEL(x + 1, row[2], row[1], row[3]);
				row += 4;
			}
		}
		break;

	default:
		printf("ScopeUnit::process_package %d: color_model=%d unrecognized\n",
			__LINE__, gui->output_frame->get_color_model());
		break;
	}
}


ScopeEngine::ScopeEngine(ScopeGUI *gui, int cpus)
 : LoadServer(cpus, cpus)
{
//printf("ScopeEngine::ScopeEngine %d cpus=%d\n", __LINE__, cpus);
	this->gui = gui;
}

ScopeEngine::~ScopeEngine()
{
}

void ScopeEngine::init_packages()
{
	int y = 0, h = gui->output_frame->get_h();
	for( int i=0,n=LoadServer::get_total_packages(); i<n; ) {
		ScopePackage *pkg = (ScopePackage*)get_package(i);
		pkg->row1 = y;
		pkg->row2 = y = (++i * h) / n;
	}

	for( int i=0,n=LoadServer::get_total_packages(); i<n; ++i ) {
		ScopeUnit *unit = (ScopeUnit*)get_client(i);
		for( int j=0; j<HIST_SECTIONS; ++j )
			bzero(unit->bins[j], sizeof(int) * TOTAL_BINS);
	}
}


LoadClient* ScopeEngine::new_client()
{
	return new ScopeUnit(gui, this);
}

LoadPackage* ScopeEngine::new_package()
{
	return new ScopePackage;
}

void ScopeEngine::process()
{
	process_packages();

	for(int i = 0; i < HIST_SECTIONS; i++)
		bzero(gui->bins[i], sizeof(int) * TOTAL_BINS);

	for(int i=0,n=get_total_clients(); i<n; ++i ) {
		ScopeUnit *unit = (ScopeUnit*)get_client(i);
		for( int j=0; j<HIST_SECTIONS; ++j ) {
			int *bp = gui->bins[j], *up = unit->bins[j];
			for( int k=TOTAL_BINS; --k>=0; ++bp,++up ) *bp += *up;
		}
	}
}


ScopeGUI::ScopeGUI(Theme *theme,
	int x, int y, int w, int h, int cpus)
 : PluginClientWindow(_(PROGRAM_NAME ": Scopes"),
	x, y, w, h, MIN_SCOPE_W, MIN_SCOPE_H, 1)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->theme = theme;
	this->cpus = cpus;
	reset();
}

ScopeGUI::ScopeGUI(PluginClient *plugin, int w, int h)
 : PluginClientWindow(plugin, w, h, MIN_SCOPE_W, MIN_SCOPE_H, 1)
{
	this->x = get_x();
	this->y = get_y();
	this->w = w;
	this->h = h;
	this->theme = plugin->get_theme();
	this->cpus = plugin->PluginClient::smp + 1;
	reset();
}

ScopeGUI::~ScopeGUI()
{
	delete waveform_vframe;
	delete vector_vframe;
	delete engine;
}

void ScopeGUI::reset()
{
	frame_w = 1;
	waveform_vframe = 0;
	vector_vframe = 0;
	engine = 0;
	use_hist = 0;
	use_wave = 1;
	use_vector = 1;
	use_hist_parade = 0;
	use_wave_parade = 0;
	waveform = 0;
	vectorscope = 0;
	histogram = 0;
	wave_w = wave_h = vector_w = vector_h = 0;
	wdial = vdial = 5.f;
}


void ScopeGUI::create_objects()
{
	if( use_hist && use_hist_parade )
		use_hist = 0;
	if( use_wave && use_wave_parade )
		use_wave = 0;
	if( !engine ) engine = new ScopeEngine(this, cpus);

	lock_window("ScopeGUI::create_objects");
	int x = theme->widget_border;
	int y = theme->widget_border +
		ScopeWaveDial::calculate_h() - ScopeMenu::calculate_h();
	add_subwindow(scope_menu = new ScopeMenu(this, x, y));
	scope_menu->create_objects();
	x += scope_menu->get_w() + theme->widget_border;
	add_subwindow(value_text = new BC_Title(x, y, ""));

	create_panels();
	update_toggles();
	show_window();
	update_scope();
	unlock_window();
}

void ScopeGUI::create_panels()
{
	calculate_sizes(get_w(), get_h());
	if( (use_wave || use_wave_parade) ) {
		int px = wave_x + wave_w - ScopeWaveDial::calculate_w() - xS(5);
		int py = wave_y - ScopeWaveDial::calculate_h() - yS(5);
		if( !waveform ) {
			add_subwindow(waveform = new ScopeWaveform(this,
				wave_x, wave_y, wave_w, wave_h));
			waveform->create_objects();
			add_subwindow(wave_dial = new ScopeWaveDial(this, px, py));
		}
		else {
			waveform->reposition_window(
				wave_x, wave_y, wave_w, wave_h);
			waveform->clear_box(0, 0, wave_w, wave_h);
			wave_dial->reposition_window(px, py);
		}
	}
	else if( !(use_wave || use_wave_parade) && waveform ) {
		delete waveform;   waveform = 0;
		delete wave_dial;  wave_dial = 0;
	}

	if( use_vector ) {
		int vx = vector_x + vector_w - ScopeVectDial::calculate_w() - xS(5);
		int vy = vector_y - ScopeVectDial::calculate_h() - yS(5);
		if( !vectorscope ) {
			add_subwindow(vectorscope = new ScopeVectorscope(this,
				vector_x, vector_y, vector_w, vector_h));
			vectorscope->create_objects();
			add_subwindow(vect_dial = new ScopeVectDial(this, vx, vy));
		}
		else {
			vectorscope->reposition_window(
				vector_x, vector_y, vector_w, vector_h);
			vectorscope->clear_box(0, 0, vector_w, vector_h);
			vect_dial->reposition_window(vx, vy);
		}
	}
	else if( !use_vector && vectorscope ) {
		delete vectorscope;  vectorscope = 0;
		delete vect_dial;    vect_dial = 0;
	}

	if( (use_hist || use_hist_parade) ) {
		if( !histogram ) {
// printf("ScopeGUI::create_panels %d %d %d %d %d\n", __LINE__,
//  hist_x, hist_y, hist_w, hist_h);
			add_subwindow(histogram = new ScopeHistogram(this,
				hist_x, hist_y, hist_w, hist_h));
			histogram->create_objects();
		}
		else {
			histogram->reposition_window(
				hist_x, hist_y, hist_w, hist_h);
			histogram->clear_box(0, 0, hist_w, hist_h);
		}
	}
	else if( !(use_hist || use_hist_parade) ) {
		delete histogram;  histogram = 0;
	}

	allocate_vframes();
	clear_points(0);
	draw_overlays(1, 1, 0);
}

void ScopeGUI::clear_points(int flash)
{
	if( histogram )
		histogram->clear_point();
	if( waveform )
		waveform->clear_point();
	if( vectorscope )
		vectorscope->clear_point();
	if( histogram && flash )
		histogram->flash(0);
	if( waveform && flash )
		waveform->flash(0);
	if( vectorscope && flash )
		vectorscope->flash(0);
}

void ScopeGUI::toggle_event()
{
}

void ScopeGUI::calculate_sizes(int w, int h)
{
	int margin = theme->widget_border;
	int menu_h = ScopeWaveDial::calculate_h() + margin * 2;
	int text_w = get_text_width(SMALLFONT, "000") + margin * 2;
	int total_panels = ((use_hist || use_hist_parade) ? 1 : 0) +
		((use_wave || use_wave_parade) ? 1 : 0) +
		(use_vector ? 1 : 0);
	int x = margin;

	int panel_w = (w - margin) / (total_panels > 0 ? total_panels : 1);
// Vectorscope determines the size of everything else
// Always last panel
	vector_w = 0;
	if( use_vector ) {
		vector_x = w - panel_w + text_w;
		vector_w = w - margin - vector_x;
		vector_y = menu_h;
		vector_h = h - vector_y - margin;

		if( vector_w > vector_h ) {
			vector_w = vector_h;
			vector_x = w - theme->widget_border - vector_w;
		}
		--total_panels;
		if(total_panels > 0)
			panel_w = (vector_x - text_w - margin) / total_panels;
	}

// Histogram is always 1st panel
	if( use_hist || use_hist_parade ) {
		hist_x = x;
		hist_y = menu_h;
		hist_w = panel_w - margin;
		hist_h = h - hist_y - margin;

		--total_panels;
		x += panel_w;
	}

	if( use_wave || use_wave_parade ) {
		wave_x = x + text_w;
		wave_y = menu_h;
		wave_w = panel_w - margin - text_w;
		wave_h = h - wave_y - margin;
	}

}


void ScopeGUI::allocate_vframes()
{
	if(waveform_vframe) delete waveform_vframe;
	if(vector_vframe) delete vector_vframe;

	int w, h;
//printf("ScopeGUI::allocate_vframes %d %d %d %d %d\n", __LINE__,
// wave_w, wave_h, vector_w, vector_h);
	int xs16 = xS(16), ys16 = yS(16);
	w = MAX(wave_w, xs16);
	h = MAX(wave_h, ys16);
	waveform_vframe = new VFrame(w, h, BC_RGB888);
	w = MAX(vector_w, xs16);
	h = MAX(vector_h, ys16);
	vector_vframe = new VFrame(w, h, BC_RGB888);
}


int ScopeGUI::resize_event(int w, int h)
{
	clear_box(0, 0, w, h);
	this->w = w;
	this->h = h;
	calculate_sizes(w, h);

	if( waveform ) {
		waveform->reposition_window(wave_x, wave_y, wave_w, wave_h);
		waveform->clear_box(0, 0, wave_w, wave_h);
		int px = wave_x + wave_w - ScopeWaveDial::calculate_w() - xS(5);
		int py = wave_y - ScopeWaveDial::calculate_h() - yS(5);
		wave_dial->reposition_window(px, py);
	}

	if( histogram ) {
		histogram->reposition_window(hist_x, hist_y, hist_w, hist_h);
		histogram->clear_box(0, 0, hist_w, hist_h);
	}

	if( vectorscope ) {
		vectorscope->reposition_window(vector_x, vector_y, vector_w, vector_h);
		vectorscope->clear_box(0, 0, vector_w, vector_h);
		int vx = vector_x + vector_w - ScopeVectDial::calculate_w() - xS(5);
		int vy = vector_y - ScopeVectDial::calculate_h() - yS(5);
		vect_dial->reposition_window(vx, vy);
	}

	allocate_vframes();
	clear_points(0);
	update_scope();
	draw_overlays(1, 1, 1);
	return 1;
}

int ScopeGUI::translation_event()
{
	x = get_x();
	y = get_y();
	PluginClientWindow::translation_event();
	return 0;
}


void ScopeGUI::draw_overlays(int overlays, int borders, int flush)
{
	BC_Resources *resources = BC_WindowBase::get_resources();
	int text_color = GREEN;
	int dark_color = (text_color>>2) & 0x3f3f3f;
	if( resources->bg_color == 0xffffff ) {
		text_color = dark_color;
	}

	if( overlays && borders ) {
		clear_box(0, 0, get_w(), get_h());
	}

	if( overlays ) {
		set_line_dashes(1);
		set_color(text_color);
		set_font(SMALLFONT);

		if( histogram && (use_hist || use_hist_parade) ) {
			histogram->draw_line(hist_w * -FLOAT_MIN / (FLOAT_MAX - FLOAT_MIN), 0,
					hist_w * -FLOAT_MIN / (FLOAT_MAX - FLOAT_MIN), hist_h);
			histogram->draw_line(hist_w * (1.0 - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN), 0,
					hist_w * (1.0 - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN), hist_h);
			set_line_dashes(0);
			histogram->draw_point();
			set_line_dashes(1);
			histogram->flash(0);
		}

// Waveform overlay
		if( waveform && (use_wave || use_wave_parade) ) {
			for( int i=0; i<=WAVEFORM_DIVISIONS; ++i ) {
				int y = wave_h * i / WAVEFORM_DIVISIONS;
				int text_y = y + wave_y + get_text_ascent(SMALLFONT) / 2;
				CLAMP(text_y, waveform->get_y() + get_text_ascent(SMALLFONT), waveform->get_y() + waveform->get_h() - 1);
				char string[BCTEXTLEN];
				sprintf(string, "%d", (int)lround((FLOAT_MAX -
					i * (FLOAT_MAX - FLOAT_MIN) / WAVEFORM_DIVISIONS) * 100));
				int text_x = wave_x - get_text_width(SMALLFONT, string) - theme->widget_border;
				set_color(text_color);
				draw_text(text_x, text_y, string);
				CLAMP(y, 0, waveform->get_h() - 1);
				set_color(dark_color);
				waveform->draw_line(0, y, wave_w, y);
				waveform->draw_rectangle(0, 0, wave_w, wave_h);
			}
			set_line_dashes(0);
			waveform->draw_point();
			set_line_dashes(1);
			waveform->flash(0);
		}
// Vectorscope overlay
		if( vectorscope && use_vector ) {
			set_line_dashes(1);
			int radius = MIN(vector_w / 2, vector_h / 2);
			for( int i=1; i<=VECTORSCOPE_DIVISIONS; i+=2 ) {
				int y = vector_h / 2 - radius * i / VECTORSCOPE_DIVISIONS;
				int text_y = y + vector_y + get_text_ascent(SMALLFONT) / 2;
				set_color(text_color);
				char string[BCTEXTLEN];
				sprintf(string, "%d",
					(int)((FLOAT_MAX / VECTORSCOPE_DIVISIONS * i) * 100));
				int text_x = vector_x - get_text_width(SMALLFONT, string) - theme->widget_border;
				draw_text(text_x, text_y, string);
				int x = vector_w / 2 - radius * i / VECTORSCOPE_DIVISIONS;
				int w = radius * i / VECTORSCOPE_DIVISIONS * 2;
				int h = radius * i / VECTORSCOPE_DIVISIONS * 2;
				if( i+2 > VECTORSCOPE_DIVISIONS )
					set_line_dashes(0);
				set_color(dark_color);
				vectorscope->draw_circle(x, y, w, h);
			}
			set_color(text_color);
			vectorscope->draw_point();
			set_line_dashes(1);
			vectorscope->flash(0);
		}

		set_font(MEDIUMFONT);
		set_line_dashes(0);
	}

	if( borders ) {
		if( use_hist || use_hist_parade ) {
			draw_3d_border(hist_x - 2, hist_y - 2, hist_w + 4, hist_h + 4,
				get_bg_color(), BLACK, MDGREY, get_bg_color());
		}
		if( use_wave || use_wave_parade ) {
			draw_3d_border(wave_x - 2, wave_y - 2, wave_w + 4, wave_h + 4,
				get_bg_color(), BLACK, MDGREY, get_bg_color());
		}
		if( use_vector ) {
			draw_3d_border(vector_x - 2, vector_y - 2, vector_w + 4, vector_h + 4,
				get_bg_color(), BLACK, MDGREY, get_bg_color());
		}
	}

	flash(0);
	if(flush) this->flush();
}



void ScopeGUI::process(VFrame *output_frame)
{
	lock_window("ScopeGUI::process");
	this->output_frame = output_frame;
	frame_w = output_frame->get_w();
	//float radius = MIN(vector_w / 2, vector_h / 2);
	bzero(waveform_vframe->get_data(), waveform_vframe->get_data_size());
	bzero(vector_vframe->get_data(), vector_vframe->get_data_size());
	engine->process();

	if( histogram )
		histogram->draw(0, 0);
	if( waveform )
		waveform->draw_vframe(waveform_vframe, 1, 0, 0);
	if( vectorscope )
		vectorscope->draw_vframe(vector_vframe, 1, 0, 0);

	draw_overlays(1, 0, 1);
	unlock_window();
}

void ScopeGUI::update_toggles()
{
	scope_menu->update_toggles();
}

ScopePanel::ScopePanel(ScopeGUI *gui, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->gui = gui;
	is_dragging = 0;
}

void ScopePanel::create_objects()
{
	set_cursor(CROSS_CURSOR, 0, 0);
	clear_box(0, 0, get_w(), get_h());
}

void ScopePanel::update_point(int x, int y)
{
}

void ScopePanel::draw_point()
{
}

void ScopePanel::clear_point()
{
}

int ScopePanel::button_press_event()
{
	if( is_event_win() && cursor_inside() ) {
		gui->clear_points(1);
		is_dragging = 1;
		int x = get_cursor_x();
		int y = get_cursor_y();
		CLAMP(x, 0, get_w() - 1);
		CLAMP(y, 0, get_h() - 1);
		update_point(x, y);
		return 1;
	}
	return 0;
}


int ScopePanel::cursor_motion_event()
{
	if( is_dragging ) {
		int x = get_cursor_x();
		int y = get_cursor_y();
		CLAMP(x, 0, get_w() - 1);
		CLAMP(y, 0, get_h() - 1);
		update_point(x, y);
		return 1;
	}
	return 0;
}


int ScopePanel::button_release_event()
{
	if( is_dragging ) {
		is_dragging = 0;
		return 1;
	}
	return 0;
}


ScopeWaveform::ScopeWaveform(ScopeGUI *gui,
		int x, int y, int w, int h)
 : ScopePanel(gui, x, y, w, h)
{
	drag_x = -1;
	drag_y = -1;
}

void ScopeWaveform::update_point(int x, int y)
{
	draw_point();
	drag_x = x;
	drag_y = y;
	int frame_x = x * gui->frame_w / get_w();

	if( gui->use_wave_parade ) {
		if( x > get_w() / 3 * 2 )
			frame_x = (x - get_w() / 3 * 2) * gui->frame_w / (get_w() / 3);
		else if( x > get_w() / 3 )
			frame_x = (x - get_w() / 3) * gui->frame_w / (get_w() / 3);
		else
			frame_x = x * gui->frame_w / (get_w() / 3);
	}

	float value = ((float)get_h() - y) / get_h() * (FLOAT_MAX - FLOAT_MIN) + FLOAT_MIN;
	char string[BCTEXTLEN];
	sprintf(string, "X: %d Value: %.3f", frame_x, value);
	gui->value_text->update(string, 0);

	draw_point();
	flash(1);
}

void ScopeWaveform::draw_point()
{
	if( drag_x >= 0 ) {
		set_inverse();
		set_color(0xffffff);
		set_line_width(2);
		draw_line(0, drag_y, get_w(), drag_y);
		draw_line(drag_x, 0, drag_x, get_h());
		set_line_width(1);
		set_opaque();
	}
}

void ScopeWaveform::clear_point()
{
	draw_point();
	drag_x = -1;
	drag_y = -1;
}

ScopeVectorscope::ScopeVectorscope(ScopeGUI *gui,
		int x, int y, int w, int h)
 : ScopePanel(gui, x, y, w, h)
{
	drag_radius = 0;
	drag_angle = 0;
}

void ScopeVectorscope::clear_point()
{
// Hide it
	draw_point();
	drag_radius = 0;
	drag_angle = 0;
}

void ScopeVectorscope::update_point(int x, int y)
{
// Hide it
	draw_point();

	int radius = MIN(get_w() / 2, get_h() / 2);
	drag_radius = sqrt(SQR(x - get_w() / 2) + SQR(y - get_h() / 2));
	drag_angle = atan2(y - get_h() / 2, x - get_w() / 2);

	drag_radius = MIN(drag_radius, radius);

	float saturation = (float)drag_radius / radius * FLOAT_MAX;
	float hue = -drag_angle * 360 / 2 / M_PI - 90;
	if( hue < 0 ) hue += 360;

	char string[BCTEXTLEN];
	sprintf(string, "Hue: %.3f Sat: %.3f", hue, saturation);
	gui->value_text->update(string, 0);

// Show it
	draw_point();
	flash(1);
}

void ScopeVectorscope::draw_point()
{
	if( drag_radius > 0 ) {
		int radius = MIN(get_w() / 2, get_h() / 2);
		set_inverse();
		set_color(0xff0000);
		set_line_width(2);
		draw_circle(get_w() / 2 - drag_radius, get_h() / 2 - drag_radius,
			drag_radius * 2, drag_radius * 2);
		draw_line(get_w() / 2, get_h() / 2,
			get_w() / 2 + radius * cos(drag_angle),
			get_h() / 2 + radius * sin(drag_angle));
		set_line_width(1);
		set_opaque();
	}
}



ScopeHistogram::ScopeHistogram(ScopeGUI *gui,
		int x, int y, int w, int h)
 : ScopePanel(gui, x, y, w, h)
{
	drag_x = -1;
}

void ScopeHistogram::clear_point()
{
// Hide it
	draw_point();
	drag_x = -1;
}

void ScopeHistogram::draw_point()
{
	if( drag_x >= 0 ) {
		set_inverse();
		set_color(0xffffff);
		set_line_width(2);
		draw_line(drag_x, 0, drag_x, get_h());
		set_line_width(1);
		set_opaque();
	}
}

void ScopeHistogram::update_point(int x, int y)
{
	draw_point();
	drag_x = x;
	float value = (float)x / get_w() * (FLOAT_MAX - FLOAT_MIN) + FLOAT_MIN;

	char string[BCTEXTLEN];
	sprintf(string, "Value: %.3f", value);
	gui->value_text->update(string, 0);

	draw_point();
	flash(1);
}



void ScopeHistogram::draw_mode(int mode, int color, int y, int h)
{
// Highest of all bins
	int normalize = 1, w = get_w();
	int *bin = gui->bins[mode], v;
	for( int i=0; i<TOTAL_BINS; ++i )
		if( (v=bin[i]) > normalize ) normalize = v;
	double norm = normalize>1 ? log(normalize) : 1e-4;
	double vnorm = h / norm;
	set_color(color);
	for( int x=0; x<w; ++x ) {
		int accum_start = (int)(x * TOTAL_BINS / w);
		int accum_end = (int)((x + 1) * TOTAL_BINS / w);
		CLAMP(accum_start, 0, TOTAL_BINS);
		CLAMP(accum_end, 0, TOTAL_BINS);
		int max = 0;
		for(int i=accum_start; i<accum_end; ++i )
			if( (v=bin[i]) > max ) max = v;
//		max = max * h / normalize;
		max = log(max) * vnorm;
		draw_line(x,y+h - max, x,y+h);
	}
}
void ScopeHistogram::draw(int flash, int flush)
{
	clear_box(0, 0, get_w(), get_h());
	if( gui->use_hist_parade ) {
		draw_mode(0, 0xff0000, 0, get_h() / 3);
		draw_mode(1, 0x00ff00, get_h() / 3, get_h() / 3);
		draw_mode(2, 0x0000ff, get_h() / 3 * 2, get_h() / 3);
	}
	else {
		draw_mode(3, LTGREY, 0, get_h());
	}

	if(flash) this->flash(0);
	if(flush) this->flush();
}


ScopeScopesOn::ScopeScopesOn(ScopeMenu *scope_menu, const char *text, int id)
 : BC_MenuItem(text)
{
	this->scope_menu = scope_menu;
	this->id = id;
}

int ScopeScopesOn::handle_event()
{
	int v = get_checked() ? 0 : 1;
	set_checked(v);
	ScopeGUI *gui = scope_menu->gui;
	switch( id ) {
	case SCOPE_HISTOGRAM:
		gui->use_hist = v;
		if( v ) gui->use_hist_parade = 0;
		break;
	case SCOPE_HISTOGRAM_RGB:
		gui->use_hist_parade = v;
		if( v ) gui->use_hist = 0;
		break;
	case SCOPE_WAVEFORM:
		gui->use_wave = v;
		if( v ) gui->use_wave_parade = 0;
		break;
	case SCOPE_WAVEFORM_RGB:
		gui->use_wave_parade = v;
		if( v ) gui->use_wave = 0;
		break;
	case SCOPE_WAVEFORM_PLY:
		gui->use_wave_parade = -v;
		if( v ) gui->use_wave = 0;
		break;
	case SCOPE_VECTORSCOPE:
		gui->use_vector = v;
		break;
	}
	gui->toggle_event();
	gui->update_toggles();
	gui->create_panels();
	gui->update_scope();
	gui->show_window();
	return 1;
}

ScopeMenu::ScopeMenu(ScopeGUI *gui, int x, int y)
 : BC_PopupMenu(x, y, xS(100), _("Scopes"))
{
	this->gui = gui;
}

void ScopeMenu::create_objects()
{
	add_item(hist_on =
		new ScopeScopesOn(this, _("Histogram"), SCOPE_HISTOGRAM));
	add_item(hist_rgb_on =
		new ScopeScopesOn(this, _("Histogram RGB"), SCOPE_HISTOGRAM_RGB));
	add_item(wave_on =
		new ScopeScopesOn(this, _("Waveform"), SCOPE_WAVEFORM));
	add_item(wave_rgb_on =
		new ScopeScopesOn(this, _("Waveform RGB"), SCOPE_WAVEFORM_RGB));
	add_item(wave_ply_on =
		new ScopeScopesOn(this, _("Waveform ply"), SCOPE_WAVEFORM_PLY));
	add_item(vect_on =
		new ScopeScopesOn(this, _("Vectorscope"), SCOPE_VECTORSCOPE));
}

void ScopeMenu::update_toggles()
{
	hist_on->set_checked(gui->use_hist);
	hist_rgb_on->set_checked(gui->use_hist_parade);
	wave_on->set_checked(gui->use_wave);
	wave_rgb_on->set_checked(gui->use_wave_parade>0);
	wave_ply_on->set_checked(gui->use_wave_parade<0);
	vect_on->set_checked(gui->use_vector);
}

ScopeWaveDial::ScopeWaveDial(ScopeGUI *gui, int x, int y)
 : BC_FPot(x, y, gui->wdial, 1.f, 9.f)
{
	this->gui = gui;
}
int ScopeWaveDial::handle_event()
{
	gui->wdial = get_value();
	gui->update_scope();
	return 1;
}

ScopeVectDial::ScopeVectDial(ScopeGUI *gui, int x, int y)
 : BC_FPot(x, y, gui->vdial, 1.f, 9.f)
{
	this->gui = gui;
}
int ScopeVectDial::handle_event()
{
	gui->vdial = get_value();
	gui->update_scope();
	return 1;
}

