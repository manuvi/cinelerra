
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

#ifndef CWINDOWTOOL_H
#define CWINDOWTOOL_H

#include "condition.inc"
#include "cwindowgui.inc"
#include "cwindowtool.inc"
#include "guicast.h"
#include "keyframe.inc"
#include "maskauto.inc"
#include "maskautos.inc"
#include "mwindow.inc"


// This common thread supports all the tool GUI's.
class CWindowTool : public Thread
{
public:
	CWindowTool(MWindow *mwindow, CWindowGUI *gui);
	~CWindowTool();

// Called depending on state of toggle button
	void start_tool(int operation);
	void stop_tool();

// Called when window is visible
	void show_tool();
	void hide_tool();

	void run();
	void update_show_window();
	void raise_window();
	void update_values();

	MWindow *mwindow;
	CWindowGUI *gui;
	CWindowToolGUI *tool_gui;
	int done;
	int current_tool;
	Condition *input_lock;
	Condition *output_lock;
// Lock run and update_values
	Mutex *tool_gui_lock;
};

class CWindowToolGUI : public BC_Window
{
public:
	CWindowToolGUI(MWindow *mwindow,
		CWindowTool *thread,
		const char *title,
		int w,
		int h);
	~CWindowToolGUI();

	virtual void create_objects() {};
// Update the keyframe from text boxes
	virtual void handle_event() {};
// Update text boxes from keyframe here
	virtual void update() {};
// Update EDL and preview only
	virtual void update_preview() {};
	int current_operation;
	int close_event();
	int keypress_event();
	int translation_event();

	MWindow *mwindow;
	CWindowTool *thread;
	CWindowCoord *event_caller;
};

class CWindowCoord : public BC_TumbleTextBox
{
public:
	CWindowCoord(CWindowToolGUI *gui, int x, int y,
			float value, int logincrement);
	CWindowCoord(CWindowToolGUI *gui, int x, int y,
			int value);

// Calls the window's handle_event
	int handle_event();

	CWindowToolGUI *gui;
};

class CWindowCropOK : public BC_GenericButton
{
public:
	CWindowCropOK(MWindow *mwindow, CWindowToolGUI *gui,
			int x, int y);
// Perform the cropping operation
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowCropGUI : public CWindowToolGUI
{
public:
	CWindowCropGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowCropGUI();
	void create_objects();
	void update();
// Update the gui
	void handle_event();
	CWindowCoord *x1, *y1, *width, *height;
};

class CWindowMaskTrack : public BC_Title
{
public:
	CWindowMaskTrack(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y, const char *text);
	~CWindowMaskTrack();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskItems : public ArrayList<BC_ListBoxItem*>
{
public:
	CWindowMaskItems() {}
	~CWindowMaskItems() { remove_all_objects(); }
};

class CWindowMaskName : public BC_PopupTextBox
{
public:
	CWindowMaskName(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y, const char *text);
	~CWindowMaskName();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
	CWindowMaskItems mask_items;

	int handle_event();
	void update_items(MaskAuto *keyframe);
};

class CWindowMaskDelMask : public BC_GenericButton
{
public:
	CWindowMaskDelMask(MWindow *mwindow, CWindowToolGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskClrMask : public BC_Button
{
public:
	CWindowMaskClrMask(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskClrMask();
	static int calculate_w(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskButton : public BC_CheckBox
{
public:
	CWindowMaskButton(MWindow *mwindow, CWindowMaskGUI *gui,
			 int x, int y, int no, int v);
	~CWindowMaskButton();

	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
	int no;
};

class CWindowMaskThumbler : public BC_Tumbler
{
public:
	CWindowMaskThumbler(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskThumbler();
	int handle_up_event();
	int handle_down_event();
	int do_event(int dir);

	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskFade : public BC_TumbleTextBox
{
public:
	CWindowMaskFade(MWindow *mwindow, CWindowToolGUI *gui,
			int x, int y);
	~CWindowMaskFade();
	int update(float v);
	int update_value(float v);
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskFadeSlider : public BC_ISlider
{
public:
	CWindowMaskFadeSlider(MWindow *mwindow, CWindowToolGUI *gui,
		int x, int y, int w);
	~CWindowMaskFadeSlider();
	int handle_event();
	int update(int64_t v);
	char *get_caption() { return 0; }
	MWindow *mwindow;
	CWindowToolGUI *gui;
	int stick;
	float last_v;
	Timer *timer;
};

class CWindowMaskGangFader : public BC_Toggle
{
public:
	CWindowMaskGangFader(MWindow *mwindow, CWindowToolGUI *gui,
			int x, int y);
	~CWindowMaskGangFader();
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskAffectedPoint : public BC_TumbleTextBox
{
public:
	CWindowMaskAffectedPoint(MWindow *mwindow, CWindowToolGUI *gui,
			int x, int y);
	~CWindowMaskAffectedPoint();
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskFocus : public BC_CheckBox
{
public:
	CWindowMaskFocus(MWindow *mwindow, CWindowToolGUI *gui,
			int x, int y);
	~CWindowMaskFocus();
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskDrawCenter : public BC_CheckBox
{
public:
	CWindowMaskDrawCenter(MWindow *mwindow, CWindowToolGUI *gui,
			int x, int y);
	~CWindowMaskDrawCenter();
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskDrawMarkers : public BC_CheckBox
{
public:
	CWindowMaskDrawMarkers(MWindow *mwindow, CWindowToolGUI *gui,
			int x, int y);
	~CWindowMaskDrawMarkers();
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskDrawBoundary : public BC_CheckBox
{
public:
	CWindowMaskDrawBoundary(MWindow *mwindow, CWindowToolGUI *gui,
			int x, int y);
	~CWindowMaskDrawBoundary();
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskDelPoint : public BC_GenericButton
{
public:
	CWindowMaskDelPoint(MWindow *mwindow, CWindowToolGUI *gui,
			int x, int y);
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskFeather : public BC_TumbleTextBox
{
public:
	CWindowMaskFeather(MWindow *mwindow, CWindowToolGUI *gui,
			int x, int y);
	~CWindowMaskFeather();
	int update(float v);
	int update_value(float v);
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskFeatherSlider : public BC_FSlider
{
public:
	CWindowMaskFeatherSlider(MWindow *mwindow, CWindowToolGUI *gui,
		int x, int y, int w, float v);
	~CWindowMaskFeatherSlider();
	int handle_event();
	int update(float v);
	char *get_caption() { return 0; }
	MWindow *mwindow;
	CWindowToolGUI *gui;
	int stick;
	float last_v;
	Timer *timer;
};

class CWindowMaskGangFeather : public BC_Toggle
{
public:
	CWindowMaskGangFeather(MWindow *mwindow, CWindowToolGUI *gui,
			int x, int y);
	~CWindowMaskGangFeather();
	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowMaskBeforePlugins : public BC_CheckBox
{
public:
	CWindowMaskBeforePlugins(CWindowToolGUI *gui,
			int x, int y);
	int handle_event();
	CWindowToolGUI *gui;
};

class CWindowDisableOpenGLMasking : public BC_CheckBox
{
public:
	CWindowDisableOpenGLMasking(CWindowToolGUI *gui,
			int x, int y);
	int handle_event();
	CWindowToolGUI *gui;
};

class CWindowMaskGUI : public CWindowToolGUI
{
public:
	CWindowMaskGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowMaskGUI();
	void create_objects();
	void update();
	void set_focused(int v, float cx, float cy);
	void update_buttons(MaskAuto *keyframe, int k);
	void handle_event();
	void get_keyframe(Track* &track, MaskAutos* &autos, MaskAuto* &keyframe,
		SubMask* &mask, MaskPoint* &point, int create_it);
	void update_preview();

	CWindowMaskTrack *mask_track;
	CWindowMaskName *name;
	CWindowMaskButton *mask_buttons[SUBMASKS];
	CWindowMaskThumbler *mask_thumbler;
	BC_Title *mask_blabels[SUBMASKS];
	CWindowMaskDelMask *del_mask;
	CWindowMaskClrMask *clr_mask;
	CWindowMaskFade *fade;
	CWindowMaskFadeSlider *fade_slider;
	CWindowMaskGangFader *gang_fader;
	CWindowMaskAffectedPoint *active_point;
	CWindowMaskDelPoint *del_point;
	CWindowCoord *x, *y;
	CWindowMaskFocus *focus;
	int focused;
	CWindowMaskDrawCenter *draw_center;
	int center_mark;
	CWindowMaskDrawMarkers *draw_markers;
	int markers;
	CWindowMaskDrawBoundary *draw_boundary;
	int boundary;
	CWindowCoord *focus_x, *focus_y;
	CWindowMaskFeather *feather;
	CWindowMaskFeatherSlider *feather_slider;
	CWindowMaskGangFeather *gang_feather;
	CWindowMaskBeforePlugins *apply_before_plugins;
	CWindowDisableOpenGLMasking *disable_opengl_masking;
};


class CWindowEyedropGUI : public CWindowToolGUI
{
public:
	CWindowEyedropGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowEyedropGUI();

	void handle_event();
	void create_objects();
	void update();

	BC_Title *current;
	CWindowCoord *radius;
	CWindowEyedropCheckBox *use_max;
	BC_Title *red, *green, *blue, *y, *u, *v;
	BC_Title *rgb_hex, *yuv_hex;
	BC_SubWindow *sample;
};


class CWindowEyedropCheckBox : public BC_CheckBox
{
public:
	CWindowEyedropCheckBox(MWindow *mwindow, 
		CWindowEyedropGUI *gui,
		int x, 
		int y);

	int handle_event();
	MWindow *mwindow;
	CWindowEyedropGUI *gui;
};



class CWindowCameraGUI : public CWindowToolGUI
{
public:
	CWindowCameraGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowCameraGUI();
	void create_objects();
	void update();
	void update_preview();

// Update the keyframe from text boxes
	void handle_event();
//	BezierAuto* get_keyframe();
	CWindowCoord *x, *y, *z;
private:
// Toggles for keyframe curve mode (for camera automation only)
	CWindowCurveToggle *t_smooth, *t_linear;
};

class CWindowCameraLeft : public BC_Button
{
public:
	CWindowCameraLeft(MWindow *mwindow, CWindowCameraGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraCenter : public BC_Button
{
public:
	CWindowCameraCenter(MWindow *mwindow, CWindowCameraGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraRight : public BC_Button
{
public:
	CWindowCameraRight(MWindow *mwindow, CWindowCameraGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraTop : public BC_Button
{
public:
	CWindowCameraTop(MWindow *mwindow, CWindowCameraGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraMiddle : public BC_Button
{
public:
	CWindowCameraMiddle(MWindow *mwindow, CWindowCameraGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraBottom : public BC_Button
{
public:
	CWindowCameraBottom(MWindow *mwindow, CWindowCameraGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowProjectorGUI : public CWindowToolGUI
{
public:
	CWindowProjectorGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowProjectorGUI();
	void create_objects();
	void update();
	void update_preview();
	void handle_event();
//	BezierAuto* get_keyframe();
	CWindowCoord *x, *y, *z;
private:
// Toggles for keyframe curve mode (projector automation only)
	CWindowCurveToggle *t_smooth, *t_linear;
};

class CWindowProjectorLeft : public BC_Button
{
public:
	CWindowProjectorLeft(MWindow *mwindow, CWindowProjectorGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorCenter : public BC_Button
{
public:
	CWindowProjectorCenter(MWindow *mwindow, CWindowProjectorGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorRight : public BC_Button
{
public:
	CWindowProjectorRight(MWindow *mwindow, CWindowProjectorGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorTop : public BC_Button
{
public:
	CWindowProjectorTop(MWindow *mwindow, CWindowProjectorGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorMiddle : public BC_Button
{
public:
	CWindowProjectorMiddle(MWindow *mwindow, CWindowProjectorGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorBottom : public BC_Button
{
public:
	CWindowProjectorBottom(MWindow *mwindow, CWindowProjectorGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};




class CWindowRulerGUI : public CWindowToolGUI
{
public:
	CWindowRulerGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowRulerGUI();
	void create_objects();
	void update();
// Update the gui
	void handle_event();

	BC_TextBox *current;
	BC_TextBox *point1;
	BC_TextBox *point2;
	BC_TextBox *deltas;
	BC_TextBox *distance;
	BC_TextBox *angle;
};



#endif
