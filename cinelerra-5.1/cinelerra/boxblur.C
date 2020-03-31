#include "boxblur.h"
// from ffmpeg vf_boxblur

template<class dst_t, class src_t> static inline
void blurt(dst_t *dst, int dst_step, src_t *src, int src_step,
		int len, int radius, float s)
{
	const int length = radius*2 + 1;
	const int inv = s * ((1<<16) + length/2)/length;
	int x, n, sum = src[radius*src_step];

	for( x=0; x<radius; ++x )
		sum += src[x*src_step]<<1;
	sum = sum*inv + (1<<15);
	for( x=0; x<=radius; ++x ) {
		sum += (src[(radius+x)*src_step] - src[(radius-x)*src_step])*inv;
		dst[x*dst_step] = sum>>16;
	}
	n = len - radius;
	for( ; x<n; ++x ) {
		sum += (src[(radius+x)*src_step] - src[(x-radius-1)*src_step])*inv;
		dst[x*dst_step] = sum >>16;
	}
	for ( ; x<len; ++x ) {
		sum += (src[(2*len-radius-x-1)*src_step] - src[(x-radius-1)*src_step])*inv;
		dst[x*dst_step] = sum>>16;
	}
}
// specialize const float *src
template<class dst_t> static inline
void blurt(dst_t *dst, int dst_step, const float *src, int src_step,
		int len, int radius, float s)
{
	const float length = radius*2 + 1;
	const float inv = s / length;
	int x, n;
	float sum = src[radius*src_step];
	for( x=0; x<radius; ++x )
		sum += src[x*src_step]*2;
	sum = sum*inv;
	for( x=0; x<=radius; ++x ) {
		sum += (src[(radius+x)*src_step] - src[(radius-x)*src_step])*inv;
		dst[x*dst_step] = sum;
	}
	n = len - radius;
	for( ; x<n; ++x ) {
		sum += (src[(radius+x)*src_step] - src[(x-radius-1)*src_step])*inv;
		dst[x*dst_step] = sum;
	}
	for ( ; x<len; ++x ) {
		sum += (src[(2*len-radius-x-1)*src_step] - src[(x-radius-1)*src_step])*inv;
		dst[x*dst_step] = sum;
	}
}

// s scales dst=src first pass
template<class dst_t, class src_t> static inline
void blur_power(dst_t *dst, int dst_step, src_t *src, int src_step,
		int len, int radius, int power, float s)
{
	dst_t atemp[len], btemp[len];
	dst_t *a = atemp, *b = btemp;
	blurt(a, 1, src, src_step, len, radius, s);
	while( power-- > 2 ) {
		blurt(b, 1, (const dst_t*)a, 1, len, radius, 1);
		dst_t *t = a; a = b; b = t;
	}
	if( power > 1 )
		blurt(dst, dst_step, (const dst_t*)a, 1, len, radius, 1);
	else
		for( int i = 0; i<len; ++i ) dst[i*dst_step] = a[i];
}


BoxBlurPackage::BoxBlurPackage()
 : LoadPackage()
{
}

BoxBlurUnit::BoxBlurUnit(BoxBlur *box_blur)
 : LoadClient(box_blur)
{
}

template<class dst_t, class src_t>
void BoxBlurUnit::blurt_package(LoadPackage *package)
{
	BoxBlur *box_blur = (BoxBlur *)server;
	src_t *src_data = (src_t *)box_blur->src_data;
	dst_t *dst_data = (dst_t *)box_blur->dst_data;
	int radius = box_blur->radius;
	int power = box_blur->power;
	int vlen = box_blur->vlen;
	int c0 = box_blur->c0, c1 = box_blur->c1;
	int src_ustep = box_blur->src_ustep;
	int dst_ustep = box_blur->dst_ustep;
	int src_vstep = box_blur->src_vstep;
	int dst_vstep = box_blur->dst_vstep;
	BoxBlurPackage *pkg = (BoxBlurPackage*)package;
	int u1 = pkg->u1, u2 = pkg->u2;
	float s = 1.;
	if( sizeof(src_t) != sizeof(dst_t) ) {
		switch( sizeof(dst_t) ) {
		case 1: s = sizeof(src_t)==2 ? 1/256. : 255.;  break;
		case 2: s = sizeof(src_t)==1 ?   256. : 65535.;  break;
		case 4: s = sizeof(src_t)==1 ? 1/256. : 1/65535.;  break;
		}
	}
	for( int u=u1; u<u2; ++u ) {
		src_t *sp = src_data + u*src_ustep;
		dst_t *dp = dst_data + u*dst_ustep;
		for( int c=c0; c<=c1; ++c ) {
			blur_power(dp+c, dst_vstep, sp+c, src_vstep,
				vlen, radius, power, s);
		}
	}
}

void BoxBlurUnit::process_package(LoadPackage *package)
{
	BoxBlur *box_blur = (BoxBlur *)server;
	int src_bpp = box_blur->src_bpp, dst_bpp = box_blur->dst_bpp;
	switch( src_bpp ) {
	case 1: switch( dst_bpp ) {
		case 1: blurt_package<uint8_t,  const uint8_t>(package);  break;
		case 2: blurt_package<uint16_t, const uint8_t>(package);  break;
		case 4: blurt_package<float,    const uint8_t>(package);  break;
		}
		break;
	case 2: switch( dst_bpp ) {
		case 1: blurt_package<uint8_t,  const uint16_t>(package); break;
		case 2: blurt_package<uint16_t, const uint16_t>(package); break;
		case 4: blurt_package<float,    const uint16_t>(package); break;
		}
		break;
	case 4: switch( dst_bpp ) {
		case 1: blurt_package<uint8_t,  const float>(package);    break;
		case 2: blurt_package<uint16_t, const float>(package);    break;
		case 4: blurt_package<float,    const float>(package);    break;
		}
		break;
	}
}

BoxBlur::BoxBlur(int cpus)
 : LoadServer(cpus, cpus)
{
}
BoxBlur::~BoxBlur()
{
}

LoadClient* BoxBlur::new_client() { return new BoxBlurUnit(this); }
LoadPackage* BoxBlur::new_package() { return new BoxBlurPackage(); }

void BoxBlur::init_packages()
{
	int u = 0;
	for( int i=0,n=LoadServer::get_total_packages(); i<n; ) {
		BoxBlurPackage *pkg = (BoxBlurPackage*)get_package(i);
		pkg->u1 = u;
		pkg->u2 = u = (++i * ulen) / n;
	}
}

//dst can equal src, requires geom(dst)==geom(src)
//uv: 0=hblur, 1=vblur;  comp: -1=rgb,0=r,1=g,2=b
void BoxBlur::process(VFrame *dst, VFrame *src, int uv,
		int radius, int power, int comp)
{
	this->radius = radius;
	this->power = power;
	this->uv = uv;
	int src_w = src->get_w(), src_h = src->get_h();
	ulen = !uv ? src_h : src_w;
	vlen = !uv ? src_w : src_h;
	c0 = comp<0 ? 0 : comp;
	c1 = comp<0 ? 2 : comp;
	src_data = src->get_data();
	dst_data = dst->get_data();
	int src_pixsz = BC_CModels::calculate_pixelsize(src->get_color_model());
	int src_comps = BC_CModels::components(src->get_color_model());
	src_bpp = src_pixsz / src_comps;
	int dst_pixsz = BC_CModels::calculate_pixelsize(dst->get_color_model());
	int dst_comps = BC_CModels::components(dst->get_color_model());
	dst_bpp = dst_pixsz / dst_comps;
	int dst_linsz = dst->get_bytes_per_line() / dst_bpp;
	int src_linsz = src->get_bytes_per_line() / src_bpp;
	src_ustep = !uv ? src_linsz : src_comps;
	dst_ustep = !uv ? dst_linsz: dst_comps;
	src_vstep = !uv ? src_comps : src_linsz;
	dst_vstep = !uv ? dst_comps : dst_linsz;

	process_packages();
}

void BoxBlur::hblur(VFrame *dst, VFrame *src, int radius, int power, int comp)
{
	process(dst, src, 0, radius, power, comp);
}
void BoxBlur::vblur(VFrame *dst, VFrame *src, int radius, int power, int comp)
{
	process(dst, src, 1, radius, power, comp);
}
void BoxBlur::blur(VFrame *dst, VFrame *src, int radius, int power, int comp)
{
	process(dst, src, 0, radius, power, comp);
	process(dst, dst, 1, radius, power, comp);
}

