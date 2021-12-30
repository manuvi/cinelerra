
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "blur.h"
#include "blurwindow.h"
#include "language.h"






BlurWindow::BlurWindow(BlurMain *client)
 : PluginClientWindow(client,
	xS(420),
	yS(250),
	xS(420),
	yS(250),
	0)
{
	this->client = client;
}

BlurWindow::~BlurWindow()
{
//printf("BlurWindow::~BlurWindow 1\n");
}

void BlurWindow::create_objects()
{
	int xs10 = xS(10), xs20 = xS(20), xs200 = xS(200);
	int ys10 = yS(10), ys20 = yS(20), ys30 = yS(30), ys40 = yS(40);
	int x2 = xS(80), x3 = xS(180);
	int x = xs10, y = ys10;
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22

	BC_TitleBar *title_bar;
	BC_Bar *bar;

	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xs20, xs10, _("Radius")));
	y += ys20;
	add_subwindow(new BC_Title(x, y, _("Radius:")));
	radius_text = new BlurRadiusText(client, this, (x + x2), y);
	radius_text->create_objects();
	add_subwindow(radius_slider = new BlurRadiusSlider(client, this, x3, y, xs200));
	clr_x = x3 + radius_slider->get_w() + x;
	add_subwindow(radius_Clr = new BlurRadiusClr(client, this, clr_x, y));
// December 2021: Disable and Hide 'Alpha determines radius' checkbox for bug(?)
//	y += ys30;
	add_subwindow(a_key = new BlurAKey(client, x, y));
	y += ys40;

	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xs20, xs10, _("Direction")));
	y += ys20;
	add_subwindow(horizontal = new BlurHorizontal(client, this, x, y));
	y += ys30;
	add_subwindow(vertical = new BlurVertical(client, this, x, y));
	y += ys40;

	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xs20, xs10, _("Color channel")));
	y += ys20;
	int x1 = x;
	int toggle_w = (get_w()-2*x) / 4;
	add_subwindow(r = new BlurR(client, x1, y));
	x1 += toggle_w;
	add_subwindow(g = new BlurG(client, x1, y));
	x1 += toggle_w;
	add_subwindow(b = new BlurB(client, x1, y));
	x1 += toggle_w;
	add_subwindow(a = new BlurA(client, x1, y));
	y += ys30;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_subwindow(reset = new BlurReset(client, this, x, y));

	show_window();

// December 2021: Disable and Hide 'Alpha determines radius' checkbox for bug(?)
	a_key->disable();
	a_key->hide_window();

	flush();
}

// for Reset button
void BlurWindow::update(int clear)
{
	switch(clear) {
		case RESET_RADIUS :
			radius_slider->update(client->config.radius);
			radius_text->update((int64_t)client->config.radius);
			break;
		case RESET_ALL :
		default:
			horizontal->update(client->config.horizontal);
			vertical->update(client->config.vertical);
			radius_slider->update(client->config.radius);
			radius_text->update((int64_t)client->config.radius);
			a_key->update(client->config.a_key);
			a->update(client->config.a);
			r->update(client->config.r);
			g->update(client->config.g);
			b->update(client->config.b);
			break;
	}
}


BlurRadiusSlider::BlurRadiusSlider(BlurMain *client, BlurWindow *gui, int x, int y, int w)
 : BC_ISlider(x, y, 0, w, w, 0, MAXRADIUS, client->config.radius, 0, 0, 0)
{
	this->client = client;
	this->gui = gui;
	enable_show_value(0); // Hide caption
}
BlurRadiusSlider::~BlurRadiusSlider()
{
}
int BlurRadiusSlider::handle_event()
{
	client->config.radius = get_value();
	gui->radius_text->update((int64_t)client->config.radius);
	client->send_configure_change();
	return 1;
}




BlurRadiusText::BlurRadiusText(BlurMain *client, BlurWindow *gui, int x, int y)
 : BC_TumbleTextBox(gui, client->config.radius,
	0, MAXRADIUS, x, y, xS(60), 0)
{
	this->client = client;
	this->gui = gui;
	set_increment(1);
}
BlurRadiusText::~BlurRadiusText()
{
}
int BlurRadiusText::handle_event()
{
	client->config.radius = atoi(get_text());
	if(client->config.radius > MAXRADIUS) client->config.radius = MAXRADIUS;
	else if(client->config.radius < 0) client->config.radius = 0;
	gui->radius_text->update((int64_t)client->config.radius);
	gui->radius_slider->update(client->config.radius);
	client->send_configure_change();
	return 1;
}




BlurVertical::BlurVertical(BlurMain *client, BlurWindow *window, int x, int y)
 : BC_CheckBox(x,
 	y,
	client->config.vertical,
	_("Vertical"))
{
	this->client = client;
	this->window = window;
}
BlurVertical::~BlurVertical()
{
}
int BlurVertical::handle_event()
{
	client->config.vertical = get_value();
	client->send_configure_change();
	return 1;
}

BlurHorizontal::BlurHorizontal(BlurMain *client, BlurWindow *window, int x, int y)
 : BC_CheckBox(x,
 	y,
	client->config.horizontal,
	_("Horizontal"))
{
	this->client = client;
	this->window = window;
}
BlurHorizontal::~BlurHorizontal()
{
}
int BlurHorizontal::handle_event()
{
	client->config.horizontal = get_value();
	client->send_configure_change();
	return 1;
}




BlurA::BlurA(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.a, _("Alpha"))
{
	this->client = client;
}
int BlurA::handle_event()
{
	client->config.a = get_value();
	client->send_configure_change();
	return 1;
}




BlurAKey::BlurAKey(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.a_key, _("Alpha determines radius"))
{
	this->client = client;
}
int BlurAKey::handle_event()
{
	client->config.a_key = get_value();
	client->send_configure_change();
	return 1;
}

BlurR::BlurR(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.r, _("Red"))
{
	this->client = client;
}
int BlurR::handle_event()
{
	client->config.r = get_value();
	client->send_configure_change();
	return 1;
}

BlurG::BlurG(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.g, _("Green"))
{
	this->client = client;
}
int BlurG::handle_event()
{
	client->config.g = get_value();
	client->send_configure_change();
	return 1;
}

BlurB::BlurB(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.b, _("Blue"))
{
	this->client = client;
}
int BlurB::handle_event()
{
	client->config.b = get_value();
	client->send_configure_change();
	return 1;
}

BlurReset::BlurReset(BlurMain *client, BlurWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->client = client;
	this->window = window;
}
BlurReset::~BlurReset()
{
}
int BlurReset::handle_event()
{
	client->config.reset(RESET_ALL);
	window->update(RESET_ALL);
	client->send_configure_change();
	return 1;
}

BlurRadiusClr::BlurRadiusClr(BlurMain *client, BlurWindow *gui, int x, int y)
 : BC_Button(x, y, client->get_theme()->get_image_set("reset_button"))
{
	this->client = client;
	this->gui = gui;
}
BlurRadiusClr::~BlurRadiusClr()
{
}
int BlurRadiusClr::handle_event()
{
	client->config.reset(RESET_RADIUS);
	gui->update(RESET_RADIUS);
	client->send_configure_change();
	return 1;
}

