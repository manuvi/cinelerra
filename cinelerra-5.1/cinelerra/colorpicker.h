
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

#ifndef COLORPICKER_H
#define COLORPICKER_H

#include "bcbutton.h"
#include "bcdialog.h"
#include "bctextbox.h"
#include "bcsubwindow.h"
#include "clip.h"
#include "colorpicker.inc"
#include "condition.inc"
#include "guicast.h"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"

#define PALLETTE_HISTORY_SIZE 16
#define COLOR_PICKER_W xS(540)
#define COLOR_PICKER_H yS(330)


class ColorPicker : public BC_DialogThread
{
public:
	ColorPicker(int do_alpha = 0, const char *title = 0);
	~ColorPicker();
	void start_window(int color, int alpha, int ok_cancel=0);
	virtual void create_objects(ColorWindow *window) {}
	virtual void update_gui(int color, int alpha);
	virtual int handle_new_color(int color, int alpha);
	BC_Window* new_gui();

	int color, alpha;
	int ok_cancel, do_alpha;
	const char *title;
};

class ColorGUI
{
public:
	ColorGUI(BC_WindowBase *window);
	~ColorGUI();

	void add_tool(BC_WindowBase *sub_wdw);
	void start_selection(int color, int alpha, int ok_cancel);
	void create_objects();
	void change_values();
	int close_gui();
	void update_display();
	void update_rgb();
	void update_hsv();
	void update_yuv();
	int handle_gui();
	void get_screen_sample();
	int cursor_motion_gui();
	int button_press_gui();
	int button_release_gui();

	virtual void update_gui(int color, int alpha);
	virtual int handle_new_color(int color, int alpha);
	virtual void create_objects(ColorGUI *gui) {}

	static int calculate_w() { return COLOR_PICKER_W; }
	static int calculate_h() { return COLOR_PICKER_H; }


	struct { float r, g, b; } rgb;
	struct { float y, u, v; } yuv;
	struct { float h, s, v; } hsv;
	float aph;
	void update_rgb(float r, float g, float b);
	void update_hsv(float h, float s, float v);
	void update_yuv(float y, float u, float v);
	void update_rgb_hex(const char *hex);
	int rgb888();
	int alpha8();

	BC_WindowBase *window;
	PaletteWheel *wheel;
	PaletteWheelValue *wheel_value;
	PaletteOutput *poutput;
	PaletteHue *hue;
	PaletteSat *sat;
	PaletteVal *val;
	PaletteRed *red;
	PaletteGrn *grn;
	PaletteBlu *blu;
	PaletteLum *lum;
	PaletteCr  *c_r;
	PaletteCb  *c_b;
	PaletteAlpha *palpha;

	PaletteHSV *hsv_h, *hsv_s, *hsv_v;
	PaletteRGB *rgb_r, *rgb_g, *rgb_b;
	PaletteYUV *yuv_y, *yuv_u, *yuv_v;
	PaletteAPH *aph_a;

	PaletteHexButton *hex_btn;
	PaletteHex *hex_box;
	PaletteGrabButton *grab_btn;
	PaletteHistory *history;

	VFrame *value_bitmap;
	int button_grabbed;

	int orig_color, orig_alpha;
	int color, alpha;
	int do_alpha, ok_cancel;
	const char *title;

	int palette_history[PALLETTE_HISTORY_SIZE];
	void load_history();
	void save_history();
	void update_history(int color);
	void update_history();
};

class ColorWindow : public BC_Window, public ColorGUI
{
public:
	ColorWindow(ColorPicker *thread, int x, int y, int w, int h, const char *title);
	~ColorWindow();

	void update_gui(int color, int alpha);
	int handle_new_color(int color, int alpha);
	void create_objects();

	int close_event() { return close_gui(); }
	int cursor_motion_event() { return cursor_motion_gui(); }
	int button_press_event() { return button_press_gui(); }
	int button_release_event() { return button_release_gui(); }

	ColorPicker *thread;
};

class ColorOK : public BC_OKButton
{
public:
	ColorOK(ColorGUI *gui, BC_WindowBase *window);
	int handle_event();

	BC_WindowBase *window;
	ColorGUI *gui;
};

class ColorCancel : public BC_CancelButton
{
public:
	ColorCancel(ColorGUI *gui, BC_WindowBase *window);
	int handle_event();

	BC_WindowBase *window;
	ColorGUI *gui;
};

class PaletteWheel : public BC_SubWindow
{
public:
	PaletteWheel(ColorGUI *gui, int x, int y);
	~PaletteWheel();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();

	void create_objects();
	int draw(float hue, float saturation);
	int get_angle(float x1, float y1, float x2, float y2);
	float torads(float angle);
	ColorGUI *gui;
	float oldhue;
	float oldsaturation;
	int button_down;
};

class PaletteWheelValue : public BC_SubWindow
{
public:
	PaletteWheelValue(ColorGUI *gui, int x, int y);
	~PaletteWheelValue();
	void create_objects();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	int draw(float hue, float saturation, float value);
	ColorGUI *gui;
	int button_down;
// Gradient
	VFrame *frame;
};

class PaletteOutput : public BC_SubWindow
{
public:
	PaletteOutput(ColorGUI *gui, int x, int y);
	~PaletteOutput();
	void create_objects();
	int handle_event();
	int draw();
	ColorGUI *gui;
};

class PaletteHue : public BC_ISlider
{
public:
	PaletteHue(ColorGUI *gui, int x, int y);
	~PaletteHue();
	int handle_event();
	ColorGUI *gui;
};

class PaletteSat : public BC_FSlider
{
public:
	PaletteSat(ColorGUI *gui, int x, int y);
	~PaletteSat();
	int handle_event();
	ColorGUI *gui;
};

class PaletteVal : public BC_FSlider
{
public:
	PaletteVal(ColorGUI *gui, int x, int y);
	~PaletteVal();
	int handle_event();
	ColorGUI *gui;
};

class PaletteRed : public BC_FSlider
{
public:
	PaletteRed(ColorGUI *gui, int x, int y);
	~PaletteRed();
	int handle_event();
	ColorGUI *gui;
};

class PaletteGrn : public BC_FSlider
{
public:
	PaletteGrn(ColorGUI *gui, int x, int y);
	~PaletteGrn();
	int handle_event();
	ColorGUI *gui;
};

class PaletteBlu : public BC_FSlider
{
public:
	PaletteBlu(ColorGUI *gui, int x, int y);
	~PaletteBlu();
	int handle_event();
	ColorGUI *gui;
};

class PaletteAlpha : public BC_FSlider
{
public:
	PaletteAlpha(ColorGUI *gui, int x, int y);
	~PaletteAlpha();
	int handle_event();
	ColorGUI *gui;
};

class PaletteLum : public BC_FSlider
{
public:
	PaletteLum(ColorGUI *gui, int x, int y);
	~PaletteLum();
	int handle_event();
	ColorGUI *gui;
};

class PaletteCr : public BC_FSlider
{
public:
	PaletteCr(ColorGUI *gui, int x, int y);
	~PaletteCr();
	int handle_event();
	ColorGUI *gui;
};

class PaletteCb : public BC_FSlider
{
public:
	PaletteCb(ColorGUI *gui, int x, int y);
	~PaletteCb();
	int handle_event();
	ColorGUI *gui;
};

class PaletteNum : public BC_TumbleTextBox
{
public:
	ColorGUI *gui;
	float *output;

	PaletteNum(ColorGUI *gui, int x, int y,
			float &output, float min, float max);
	~PaletteNum();
	void update_output() { *output = atof(get_text()); }
	static int calculate_h() { return BC_Tumbler::calculate_h(); }
};

class PaletteRGB : public PaletteNum
{
public:
	PaletteRGB(ColorGUI *gui, int x, int y,
			float &output, float min, float max)
	 : PaletteNum(gui, x, y, output, min, max) {}
	int handle_event();
};

class PaletteYUV : public PaletteNum
{
public:
	PaletteYUV(ColorGUI *gui, int x, int y,
			float &output, float min, float max)
	 : PaletteNum(gui, x, y, output, min, max) {}
	int handle_event();
};

class PaletteHSV : public PaletteNum
{
public:
	PaletteHSV(ColorGUI *gui, int x, int y,
			float &output, float min, float max)
	 : PaletteNum(gui, x, y, output, min, max) {}
	int handle_event();
};

class PaletteAPH : public PaletteNum
{
public:
	PaletteAPH(ColorGUI *gui, int x, int y,
			float &output, float min, float max)
	 : PaletteNum(gui, x, y, output, min, max) {}
	int handle_event();
};

class PaletteHexButton : public BC_GenericButton
{
public:
	PaletteHexButton(ColorGUI *gui, int x, int y);
	~PaletteHexButton();
	int handle_event();
	ColorGUI *gui;
};

class PaletteHex : public BC_TextBox
{
public:
	PaletteHex(ColorGUI *gui, int x, int y, const char *hex);
	~PaletteHex();
	int keypress_event();
	void update();
	ColorGUI *gui;
};

class PaletteGrabButton : public BC_Button
{
public:
	PaletteGrabButton(ColorGUI *gui, int x, int y);
	~PaletteGrabButton();
	int handle_event();

	ColorGUI *gui;
	VFrame *vframes[3];
};

class PaletteHistory : public BC_SubWindow
{
public:
	PaletteHistory(ColorGUI *gui, int x, int y);
	~PaletteHistory();
	void update(int flush=1);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int cursor_leave_event();
	int repeat_event(int64_t duration);

	ColorGUI *gui;
	int button_down;
};

class ColorButton : public BC_Button
{
public:
	ColorButton(const char *title,
		int x, int y, int w, int h,
		int color, int alpha, int ok_cancel);
	~ColorButton();

	virtual void set_color(int color);
	virtual int handle_new_color(int color, int alpha);
	virtual void handle_done_event(int result);

	void close_picker();
	void update_gui(int color, int alpha);
	void update_gui(int color);
	int handle_event();

	const char *title;
	int color, alpha, ok_cancel;
	int orig_color, orig_alpha;

	VFrame *vframes[3];
	ColorButtonPicker *color_picker;
	ColorButtonThread *color_thread;
};

class ColorButtonPicker : public ColorPicker
{
public:
	ColorButtonPicker(ColorButton *color_button);
	~ColorButtonPicker();
	void update(int color, int alpha);
	int handle_new_color(int color, int alpha);
	void handle_done_event(int result);
	void update_gui();
	void update_gui(int color, int alpha);

	ColorButton *color_button;
};

class ColorButtonThread : public Thread
{
public:
	ColorButtonThread(ColorButton *color_button);
	~ColorButtonThread();

	void start();
	void stop();
	void run();

	ColorButton *color_button;
	Condition *update_lock;
	int done;
};

class ColorBoxButton : public ColorButton
{
public:
	ColorBoxButton(const char *title,
		int x, int y, int w, int h,
		int color, int alpha, int ok_cancel);
	~ColorBoxButton();
	void create_objects();

	virtual int handle_new_color(int color, int alpha);
	virtual void handle_done_event(int result);
	void set_color(int color);
};

class ColorCircleButton : public ColorButton
{
public:
	ColorCircleButton(const char *title,
		int x, int y, int w, int h,
		int color, int alpha, int ok_cancel);
	~ColorCircleButton();
	void create_objects();

	virtual int handle_new_color(int color, int alpha);
	virtual void handle_done_event(int result);
	void set_color(int color);
};

#endif
