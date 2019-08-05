
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

#include "bcsignals.h"
#include "condition.h"
#include "clip.h"
#include "maskauto.h"
#include "maskautos.h"
#include "maskengine.h"
#include "mutex.h"
#include "track.h"
#include "transportque.inc"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

void write_mask(VFrame *vfrm, const char *fmt, ...)
{
  va_list ap;    va_start(ap, fmt);
  char fn[256];  vsnprintf(fn, sizeof(fn), fmt, ap);
  va_end(ap);
  FILE *fp = !strcmp(fn,"-") ? stdout : fopen(fn,"w");
  if( fp ) {
    int w = vfrm->get_w(), h = vfrm->get_h();
    int m = vfrm->get_color_model();
    fprintf(fp,"P5\n%d %d\n%d\n",w,h,m==BC_A8? 0xff : 0xffff);
    int bpp = m==BC_A8? 1 : 2;
    fwrite(vfrm->get_data(),bpp*w,h,fp);  fflush(fp);
    if( fp != stdout ) fclose(fp);
  }
}

MaskPackage::MaskPackage()
{
}

MaskPackage::~MaskPackage()
{
}

MaskUnit::MaskUnit(MaskEngine *engine)
 : LoadClient(engine)
{
	this->engine = engine;
	spot = 0;
	r = 0;
}

MaskUnit::~MaskUnit()
{
}

void MaskUnit::draw_line(int v, int ix1, int iy1, int ix2, int iy2)
{
	if( iy1 == iy2 ) return;
	int x1 = iy1 < iy2 ? ix1 : ix2;
	int y1 = iy1 < iy2 ? iy1 : iy2;
	int x2 = iy1 < iy2 ? ix2 : ix1;
	int y2 = iy1 < iy2 ? iy2 : iy1;
	float slope = (float)(x2-x1) / (y2-y1);
	int dy = y1 - start_y;
	int i = dy < 0 ? (y1=start_y, -dy) : 0;
	if( y2 > end_y ) y2 = end_y;
	if( y2 < start_y || y1 >= end_y ) return;

	VFrame *temp = engine->temp;
	int w1 = temp->get_w()-1;
	temp_t **rows = (temp_t **)temp->get_rows();
	for( int y=y1; y<y2; ++i,++y ) {
		int x = (int)(i*slope + x1);
		bclamp(x, 0, w1);
		rows[y][x] = rows[y][x] == v ? 0 : v;
	}
}

void MaskUnit::draw_fill(int v)
{
	VFrame *temp = engine->temp;
	int temp_w = temp->get_w();
	temp_t **rows = (temp_t**)temp->get_rows();

	for( int y=start_y; y<end_y; ++y ) {
		temp_t *row = rows[y];
		int value = 0, total = 0;
		for( int x=0; x<temp_w; ++x )
			if( row[x] == v ) ++total;
		if( total < 2 ) continue;
		if( total & 0x1 ) --total;
		for( int x=0; x<temp_w; ++x ) {
			if( row[x]==v && total>0 ) {
				--total;
				value = value ? 0 : v;
			}
			else if( value )
				row[x] = value;
		}
	}
}

void MaskUnit::draw_feather(int ix1,int iy1, int ix2,int iy2)
{
	int x1 = iy1 < iy2 ? ix1 : ix2;
	int y1 = iy1 < iy2 ? iy1 : iy2;
	int x2 = iy1 < iy2 ? ix2 : ix1;
	int y2 = iy1 < iy2 ? iy2 : iy1;
	VFrame *temp = engine->temp;
	int h = temp->get_h();
	if( y2 < 0 || y1 >= h ) return;

	int x = x1, y = y1;
	int dx = x2-x1, dy = y2-y1;
	int dx2 = 2*dx, dy2 = 2*dy;
	if( dx < 0 ) dx = -dx;
	int m = dx > dy ? dx : dy, n = m;
	if( dy >= dx ) {
		if( dx2 >= 0 ) do {     /* +Y, +X */
			draw_spot(x, y++);
			if( (m -= dx2) < 0 ) { m += dy2;  ++x; }
		} while( --n >= 0 );
		else do {	       /* +Y, -X */
			draw_spot(x, y++);
			if( (m += dx2) < 0 ) { m += dy2;  --x; }
		} while( --n >= 0 );
	}
	else {
		if( dx2 >= 0 ) do {     /* +X, +Y */
			draw_spot(x++, y);
			if( (m -= dy2) < 0 ) { m += dx2;  ++y; }
		} while( --n >= 0 );
		else do {	       /* -X, +Y */
			draw_spot(x--, y);
			if( (m -= dy2) < 0 ) { m -= dx2;  ++y; }
		} while( --n >= 0 );
	}
}

void MaskUnit::draw_spot(int ix, int iy)
{
	int rr = r * r, n = abs(r), rv = r * v;
	if( iy < start_y-n || iy >= end_y+n ) return;
	VFrame *temp = engine->temp;
	int w1 = temp->get_w()-1, h1 = temp->get_h()-1;
	int xs = ix - n;  bclamp(xs, 0, w1);
	int xn = ix + n;  bclamp(xn, 0, w1);
	int ys = iy - n;  bclamp(ys, 0, h1);
	int yn = iy + n;  bclamp(yn, 0, h1);

	temp_t **rows = (temp_t**)temp->get_rows();
	for( int y=ys ; y<=yn; ++y ) {
		temp_t *row = rows[y];
		for( int x=xs; x<=xn; ++x ) {
			int dx = x-ix, dy = y-iy;
			int dd = dx*dx + dy*dy;
			if( dd >= rr ) continue;
			temp_t *rp = &row[x], a = spot[dd];
			if( rv*(*rp-a) < 0 ) *rp = a;
		}
	}
}

void MaskUnit::process_package(LoadPackage *package)
{
	MaskPackage *ptr = (MaskPackage*)package;
	start_y = ptr->start_y;
	end_y = ptr->end_y;
	if( start_y >= end_y ) return;
	mask_model = engine->mask->get_color_model();
	VFrame *temp = engine->temp;
	if( engine->recalculate && engine->step == DO_MASK ) {
// Draw masked region of polygons on temp
		for( int k=0; k<engine->edges.size(); ++k ) {
			if( !engine->edges[k] ) continue;
			MaskEdge &edge = *engine->edges[k];
			if( edge.size() < 3 ) continue;
			int v = k + 1;
			for( int i=0; i<edge.size(); ++i ) {
				MaskCoord a = edge[i];
				MaskCoord b = i<edge.size()-1 ? edge[i+1] : edge[0];
				draw_line(v, a.x,a.y, b.x,b.y);
			}
			draw_fill(v);
		}
// map temp to fader alpha
		int temp_w = temp->get_w();
		temp_t **rows = (temp_t**)temp->get_rows();
		temp_t *fade = engine->fade;
		for( int y=start_y; y<end_y; ++y ) {
			temp_t *tp = rows[y];
			for( int i=temp_w; --i>=0; ++tp ) *tp = fade[*tp];
		}
	}
	if( engine->recalculate && engine->step == DO_FEATHER ) {
// draw feather
		for( int k=0; k<engine->edges.size(); ++k ) {
			if( !(v = engine->faders[k]) ) continue;
			if( !(r = engine->feathers[k]) ) continue;
			MaskEdge &edge = *engine->edges[k];
			if( !edge.size() ) continue;
			float rv = r * v, vv = fabs(v);
			int fg = 0xffff * (rv >= 0 ? vv : 0);
			int bg = 0xffff * (rv >= 0 ? 0 : vv);
			int rr = r*r;  double dr = 1./rr;
			// gausian, rr is x**2, limit 1/255
			double sig2 = -log(255.0);
			temp_t psf[rr+1];  spot = psf;
			for( int i=0; i<=rr; ++i ) {
				double d = exp(i*dr * sig2);
				psf[i] = d*fg + (1-d)*bg;
			}
			int n = edge.size();
			for( int i=0; i<n; ++i ) {
				MaskCoord &a = edge[i];
				MaskCoord &b = i<edge.size()-1 ? edge[i+1] : edge[0];
				draw_feather(a.x,a.y, b.x,b.y);
			}
		}

#define REMAP(cmodel, type, expr) case cmodel: { \
type **msk_rows = (type**)engine->mask->get_rows(); \
for( int y=start_y; y<end_y; ++y ) { \
	temp_t *rp = rows[y]; \
	type *mp = msk_rows[y]; \
	for( int i=temp_w; --i>=0; ++rp,++mp ) *mp = expr; \
} } break
// map alpha to mask
		const float to_flt = 1/65535.;
		int temp_w = temp->get_w();
		temp_t **rows = (temp_t**)temp->get_rows();
		switch( mask_model ) {
		REMAP(BC_A8, uint8_t, *rp >> 8);
		REMAP(BC_A16, uint16_t, *rp);
		REMAP(BC_A_FLOAT, float, *rp * to_flt);
		}
	}

// Apply mask
	if( engine->step == DO_APPLY ) {
		int mask_w = engine->mask->get_w();
		uint8_t **out_rows = engine->output->get_rows();
		uint8_t **msk_rows = engine->mask->get_rows();
#define APPLY_MASK_ALPHA(cmodel, type, max, components, do_yuv) \
case cmodel: \
for( int y=ptr->start_y; y<ptr->end_y; ++y ) { \
	type *out_row = (type*)out_rows[y]; \
	type *msk_row = (type*)msk_rows[y]; \
	type chroma_offset = (int)(max + 1) / 2; \
	for( int x=0; x<mask_w; ++x ) { \
		type a = msk_row[x], b = max-a; \
		if( components == 4 ) { \
			out_row[x*4 + 3] = out_row[x*4 + 3]*b / max; \
		} \
		else { \
			out_row[x*3 + 0] = out_row[x*3 + 0]*b / max; \
			out_row[x*3 + 1] = out_row[x*3 + 1]*b / max; \
			out_row[x*3 + 2] = out_row[x*3 + 2]*b / max; \
			if( do_yuv ) { \
				out_row[x*3 + 1] += chroma_offset*a / max; \
				out_row[x*3 + 2] += chroma_offset*a / max; \
			} \
		} \
	} \
} break

		switch( engine->output->get_color_model() ) { \
		APPLY_MASK_ALPHA(BC_RGB888, uint8_t, 0xff, 3, 0); \
		APPLY_MASK_ALPHA(BC_RGB_FLOAT, float, 1.0, 3, 0); \
		APPLY_MASK_ALPHA(BC_YUV888, uint8_t, 0xff, 3, 1); \
		APPLY_MASK_ALPHA(BC_RGBA_FLOAT, float, 1.0, 4, 0); \
		APPLY_MASK_ALPHA(BC_YUVA8888, uint8_t, 0xff, 4, 1); \
		APPLY_MASK_ALPHA(BC_RGBA8888, uint8_t, 0xff, 4, 0); \
		APPLY_MASK_ALPHA(BC_RGB161616, uint16_t, 0xffff, 3, 0); \
		APPLY_MASK_ALPHA(BC_YUV161616, uint16_t, 0xffff, 3, 1); \
		APPLY_MASK_ALPHA(BC_YUVA16161616, uint16_t, 0xffff, 4, 1); \
		APPLY_MASK_ALPHA(BC_RGBA16161616, uint16_t, 0xffff, 4, 0); \
		}
	}
}


MaskEngine::MaskEngine(int cpus)
 : LoadServer(cpus, 2*cpus)
// : LoadServer(1, 1)
{
	mask = 0;
	temp = 0;
}

MaskEngine::~MaskEngine()
{
	delete mask;
	delete temp;
	for( int i = 0; i < point_sets.total; i++ )
		point_sets[i]->remove_all_objects();
	point_sets.remove_all_objects();
}

int MaskEngine::points_equivalent(MaskPoints *new_points,
	MaskPoints *points)
{
//printf("MaskEngine::points_equivalent %d %d\n", new_points->total, points->total);
	if( new_points->total != points->total ) return 0;

	for( int i = 0; i < new_points->total; i++ ) {
		if( !(*new_points->get(i) == *points->get(i)) ) return 0;
	}

	return 1;
}

void MaskEngine::do_mask(VFrame *output,
	int64_t start_position_project,
	MaskAutos *keyframe_set,
	MaskAuto *keyframe,
	MaskAuto *default_auto)
{
	this->output = output;
	recalculate = 0;
	int mask_model = 0;

	switch( output->get_color_model() ) {
	case BC_RGB_FLOAT:
	case BC_RGBA_FLOAT:
		mask_model = BC_A_FLOAT;
		break;

	case BC_RGB888:
	case BC_RGBA8888:
	case BC_YUV888:
	case BC_YUVA8888:
		mask_model = BC_A8;
		break;

	case BC_RGB161616:
	case BC_RGBA16161616:
	case BC_YUV161616:
	case BC_YUVA16161616:
		mask_model = BC_A16;
		break;
	}

// Determine if recalculation is needed
SET_TRACE

	int mask_w = output->get_w(), mask_h = output->get_h();
	if( mask && ( mask->get_color_model() != mask_model ||
	    mask->get_w() != mask_w || mask->get_h() != mask_h ) ) {
		delete mask;  mask = 0;
		recalculate = 1;
	}
	if( temp && ( temp->get_w() != mask_w || temp->get_h() != mask_h ) ) {
		delete temp;  temp = 0;
	}

	total_submasks = keyframe_set->total_submasks(start_position_project, PLAY_FORWARD);
	if( total_submasks != point_sets.size() )
		recalculate = 1;

	for( int i=0; i<total_submasks && !recalculate; ++i ) {
		float new_fader = keyframe_set->get_fader(start_position_project, i, PLAY_FORWARD);
		if( new_fader != faders[i] ) { recalculate = 1;  break; }
		float new_feather = keyframe_set->get_feather(start_position_project, i, PLAY_FORWARD);
		if( new_feather != feathers[i] ) { recalculate = 1;  break; }
		MaskPoints new_points;
		keyframe_set->get_points(&new_points, i,
				start_position_project, PLAY_FORWARD);
		if( !points_equivalent(&new_points, point_sets[i]) )
			recalculate = 1;
	}

	if( recalculate ) {
		for( int i = 0; i < point_sets.total; i++ ) {
			MaskPoints *points = point_sets[i];
			points->remove_all_objects();
		}
		point_sets.remove_all_objects();
		edges.remove_all_objects();
		faders.remove_all();
		feathers.remove_all();
		fade[0] = 0;

		int show_mask = keyframe_set->track->masks;
		for( int i=0; i<total_submasks; ++i ) {
			float fader = keyframe_set->get_fader(start_position_project, i, PLAY_FORWARD);
			float v = fader / 100;
			faders.append(v);
			temp_t t = fabs(v) * 0xffff;
			if( fader < 0 ) {
				if( fade[0] < t ) fade[0] = t;
				t = 0;
			}
			fade[i+1] = t;
			float feather = keyframe_set->get_feather(start_position_project, i, PLAY_FORWARD);
			feathers.append(feather);
			MaskPoints *new_points = new MaskPoints();
			keyframe_set->get_points(new_points, i, start_position_project, PLAY_FORWARD);
			point_sets.append(new_points);
			MaskEdge *edge = edges.append(new MaskEdge());
			if( !((show_mask>>i) & 1) ) continue;
			edge->load(*new_points, 0);
		}
// draw mask
		if( !mask ) mask = new VFrame(mask_w, mask_h, mask_model, 0);
		if( !temp ) temp = new VFrame(mask_w, mask_h, BC_A16, 0);
		mask->clear_frame();
		temp->clear_frame();
		step = DO_MASK;
		process_packages();
		step = DO_FEATHER;
		process_packages();
	}
// Run units
SET_TRACE
	step = DO_APPLY;
	process_packages();
SET_TRACE
}

void MaskEngine::init_packages()
{
SET_TRACE
//printf("MaskEngine::init_packages 1\n");
	int x0 = 0, y0 = 0, i = 0, n = get_total_packages();
	int out_w = output->get_w(), out_h = output->get_h();
SET_TRACE
	while( i < n ) {
		MaskPackage *ptr = (MaskPackage*)get_package(i++);
		int x1 = (out_w * i) / n, y1 = (out_h * i) / n;
		ptr->start_x = x0;  ptr->end_x = x1;
		ptr->start_y = y0;  ptr->end_y = y1;
		x0 = x1;  y0 = y1;
	}
SET_TRACE
//printf("MaskEngine::init_packages 2\n");
}

LoadClient* MaskEngine::new_client()
{
	return new MaskUnit(this);
}

LoadPackage* MaskEngine::new_package()
{
	return new MaskPackage;
}

