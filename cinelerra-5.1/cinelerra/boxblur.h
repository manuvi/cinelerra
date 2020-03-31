#include "boxblur.inc"
#include "loadbalance.h"
#include "vframe.h"

#include <stdint.h>

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
	int radius, power, uv;
	int ulen, vlen, c0, c1;
	int src_bpp, dst_bpp;
};

