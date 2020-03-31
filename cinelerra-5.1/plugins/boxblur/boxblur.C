
/*
 * CINELERRA
 * Copyright (C) 1997-2020 Adam Williams <broadcast at earthling dot net>
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

#include "guicast.h"
#include "boxblur.h"
#include "filexml.h"
#include "language.h"
#include "pluginvclient.h"
#include "theme.h"

#include <stdint.h>
#include <string.h>

class BoxBlurConfig;
class BoxBlurRadius;
class BoxBlurPower;
class BoxBlurWindow;
class BoxBlurEffect;


class BoxBlurConfig
{
public:
	BoxBlurConfig();
	void copy_from(BoxBlurConfig &that);
	int equivalent(BoxBlurConfig &that);
	void interpolate(BoxBlurConfig &prev, BoxBlurConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame);

	int horz_radius, vert_radius, power;
};


class BoxBlurRadius : public BC_ISlider
{
public:
	BoxBlurRadius(BoxBlurWindow *gui, int x, int y, int w, int *radius);
	int handle_event();

	BoxBlurWindow *gui;
	int *radius;
};

class BoxBlurPower : public BC_ISlider
{
public:
	BoxBlurPower(BoxBlurWindow *gui, int x, int y, int w, int *power);
	int handle_event();

	BoxBlurWindow *gui;
	int *power;
};

class BoxBlurWindow : public PluginClientWindow
{
public:
	BoxBlurWindow(BoxBlurEffect *plugin);
	~BoxBlurWindow();
	void create_objects();

	BoxBlurEffect *plugin;
	BoxBlurRadius *blur_horz;
	BoxBlurRadius *blur_vert;
	BoxBlurPower *blur_power;
};



class BoxBlurEffect : public PluginVClient
{
public:
	BoxBlurEffect(PluginServer *server);
	~BoxBlurEffect();

	PLUGIN_CLASS_MEMBERS(BoxBlurConfig)
	int process_realtime(VFrame *input, VFrame *output);
	void update_gui();
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	BoxBlur *box_blur;
};


BoxBlurConfig::BoxBlurConfig()
{
	horz_radius = 2;
	vert_radius = 2;
	power = 2;
}

void BoxBlurConfig::copy_from(BoxBlurConfig &that)
{
	horz_radius = that.horz_radius;
	vert_radius = that.vert_radius;
	power = that.power;
}

int BoxBlurConfig::equivalent(BoxBlurConfig &that)
{
	return horz_radius == that.horz_radius &&
		vert_radius == that.vert_radius &&
		power == that.power;
}

void BoxBlurConfig::interpolate(BoxBlurConfig &prev, BoxBlurConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	double u = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	double v = 1. - u;
	this->horz_radius = u*prev.horz_radius + v*next.horz_radius;
	this->vert_radius = u*prev.vert_radius + v*next.vert_radius;
	this->power = u*prev.power + v*next.power;
}


BoxBlurRadius::BoxBlurRadius(BoxBlurWindow *gui, int x, int y, int w, int *radius)
 : BC_ISlider(x, y, 0, w, w, 0, 100, *radius)
{
	this->gui = gui;
	this->radius = radius;
}
int BoxBlurRadius::handle_event()
{
	*radius = get_value();
	gui->plugin->send_configure_change();
	return 1;
}

BoxBlurPower::BoxBlurPower(BoxBlurWindow *gui, int x, int y, int w, int *power)
 : BC_ISlider(x, y, 0, w, w, 1, 10, *power)
{
	this->gui = gui;
	this->power = power;
}
int BoxBlurPower::handle_event()
{
	*power = get_value();
	gui->plugin->send_configure_change();
	return 1;
}

BoxBlurWindow::BoxBlurWindow(BoxBlurEffect *plugin)
 : PluginClientWindow(plugin, xS(200), yS(120), xS(200), yS(120), 0)
{
	this->plugin = plugin;
}

BoxBlurWindow::~BoxBlurWindow()
{
}

void BoxBlurWindow::create_objects()
{
	int x = xS(10), y = yS(10);
	int x1 = xS(70), margin = plugin->get_theme()->widget_border;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Box Blur"), MEDIUMFONT, YELLOW));
	y += title->get_h() + 2*margin;
	add_subwindow(title = new BC_Title(x, y, _("Horz:")));
	add_subwindow(blur_horz = new BoxBlurRadius(this, x1, y, xS(120),
		&plugin->config.horz_radius));
	y += blur_horz->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Vert:")));
	add_subwindow(blur_vert = new BoxBlurRadius(this, x1, y, xS(120),
		&plugin->config.vert_radius));
	y += blur_vert->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Power:")));
	add_subwindow(blur_power = new BoxBlurPower(this, x1, y, xS(120),
		&plugin->config.power));
	show_window(1);
}


REGISTER_PLUGIN(BoxBlurEffect)
NEW_WINDOW_MACRO(BoxBlurEffect, BoxBlurWindow)
LOAD_CONFIGURATION_MACRO(BoxBlurEffect, BoxBlurConfig)


BoxBlurEffect::BoxBlurEffect(PluginServer *server)
 : PluginVClient(server)
{
	box_blur = 0;
}

BoxBlurEffect::~BoxBlurEffect()
{
	delete box_blur;
}

const char* BoxBlurEffect::plugin_title() { return N_("BoxBlur"); }
int BoxBlurEffect::is_realtime() { return 1; }


void BoxBlurEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("BOXBLUR");
	output.tag.set_property("HORZ_RADIUS", config.horz_radius);
	output.tag.set_property("VERT_RADIUS", config.vert_radius);
	output.tag.set_property("POWER", config.power);
	output.append_tag();
	output.tag.set_title("/BOXBLUR");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void BoxBlurEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	int result = 0;

	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("BOXBLUR") ) {
			config.horz_radius = input.tag.get_property("HORZ_RADIUS", config.horz_radius);
			config.vert_radius = input.tag.get_property("VERT_RADIUS", config.vert_radius);
			config.power = input.tag.get_property("POWER", config.power);
		}
	}
}

int BoxBlurEffect::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();
	if( !box_blur ) {
		int cpus = input->get_w()*input->get_h()/0x80000 + 1;
		box_blur = new BoxBlur(cpus);
	}
	if( config.horz_radius ) {
		box_blur->hblur(output, input, config.horz_radius, config.power);
		input = output;
	}
	if( config.vert_radius )
		box_blur->vblur(output, input, config.vert_radius, config.power);
	return 1;
}

void BoxBlurEffect::update_gui()
{
	if( !thread ) return;
	load_configuration();
	thread->window->lock_window("BoxBlurEffect::update_gui");
	BoxBlurWindow *gui = (BoxBlurWindow *)thread->window;
	gui->blur_horz->update(config.horz_radius);
	gui->blur_vert->update(config.vert_radius);
	gui->blur_power->update(config.power);
	thread->window->unlock_window();
}

