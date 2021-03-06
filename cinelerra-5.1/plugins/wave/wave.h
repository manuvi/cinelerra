
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

#ifndef WAVE_H
#define WAVE_H


#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "theme.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>




#define SMEAR 0
#define BLACKEN 1

#define RESET_DEFAULT_SETTINGS 10
#define RESET_ALL        0
#define RESET_AMPLITUDE	 1
#define RESET_PHASE      2
#define RESET_WAVELENGTH 3

#define AMPLITUDE_MIN    0.00
#define AMPLITUDE_MAX  100.00
#define PHASE_MIN        0.00
#define PHASE_MAX      360.00
#define WAVELENGTH_MIN   0.00
#define WAVELENGTH_MAX  50.00

class WaveEffect;
class WaveWindow;
class WaveFText;
class WaveFSlider;
class WaveReset;
class WaveDefaultSettings;
class WaveClr;

class WaveConfig
{
public:
	WaveConfig();

	void reset(int clear);
	void copy_from(WaveConfig &src);
	int equivalent(WaveConfig &src);
	void interpolate(WaveConfig &prev,
		WaveConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);
	int mode;
	int reflective;
	float amplitude;
	float phase;
	float wavelength;
};

class WaveSmear : public BC_Radial
{
public:
	WaveSmear(WaveEffect *plugin, WaveWindow *window, int x, int y);
	int handle_event();
	WaveEffect *plugin;
	WaveWindow *window;
};

class WaveBlacken : public BC_Radial
{
public:
	WaveBlacken(WaveEffect *plugin, WaveWindow *window, int x, int y);
	int handle_event();
	WaveEffect *plugin;
	WaveWindow *window;
};


class WaveReflective : public BC_CheckBox
{
public:
	WaveReflective(WaveEffect *plugin, int x, int y);
	int handle_event();
	WaveEffect *plugin;
};


class WaveFText : public BC_TumbleTextBox
{
public:
	WaveFText(WaveWindow *gui, WaveEffect *plugin,
		WaveFSlider *slider, float *output, int x, int y, float min, float max);
	~WaveFText();
	int handle_event();
	WaveWindow *gui;
	WaveEffect *plugin;
	WaveFSlider *slider;
	float *output;
	float min, max;
};

class WaveFSlider : public BC_FSlider
{
public:
	WaveFSlider(WaveEffect *plugin,
		WaveFText *text, float *output, int x, int y,
		float min, float max);
	~WaveFSlider();
	int handle_event();
	WaveEffect *plugin;
	WaveFText *text;
	float *output;
};

class WaveReset : public BC_GenericButton
{
public:
	WaveReset(WaveEffect *plugin, WaveWindow *gui, int x, int y);
	~WaveReset();
	int handle_event();
	WaveEffect *plugin;
	WaveWindow *gui;
};

class WaveDefaultSettings : public BC_GenericButton
{
public:
	WaveDefaultSettings(WaveEffect *plugin, WaveWindow *gui, int x, int y, int w);
	~WaveDefaultSettings();
	int handle_event();
	WaveEffect *plugin;
	WaveWindow *gui;
};

class WaveClr : public BC_Button
{
public:
	WaveClr(WaveEffect *plugin, WaveWindow *gui, int x, int y, int clear);
	~WaveClr();
	int handle_event();
	WaveEffect *plugin;
	WaveWindow *gui;
	int clear;
};





class WaveWindow : public PluginClientWindow
{
public:
	WaveWindow(WaveEffect *plugin);
	~WaveWindow();
	void create_objects();
	void update_mode();
	void update_gui(int clear);

	WaveEffect *plugin;
//	WaveSmear *smear;
//	WaveBlacken *blacken;
//	WaveReflective *reflective;

	WaveFText *amplitude_text;
	WaveFSlider *amplitude_slider;
	WaveClr *amplitude_Clr;

	WaveFText *phase_text;
	WaveFSlider *phase_slider;
	WaveClr *phase_Clr;

	WaveFText *wavelength_text;
	WaveFSlider *wavelength_slider;
	WaveClr *wavelength_Clr;

	WaveReset *reset;
	WaveDefaultSettings *default_settings;
};







class WaveServer : public LoadServer
{
public:
	WaveServer(WaveEffect *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	WaveEffect *plugin;
};

class WavePackage : public LoadPackage
{
public:
	WavePackage();
	int row1, row2;
};

class WaveUnit : public LoadClient
{
public:
	WaveUnit(WaveEffect *plugin, WaveServer *server);
	void process_package(LoadPackage *package);
	WaveEffect *plugin;
};









class WaveEffect : public PluginVClient
{
public:
	WaveEffect(PluginServer *server);
	~WaveEffect();

	PLUGIN_CLASS_MEMBERS(WaveConfig)
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	VFrame *temp_frame;
	VFrame *input, *output;
	WaveServer *engine;
};

#endif

