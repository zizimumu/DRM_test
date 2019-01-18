/*
 * Author: Leon.He
 * e-mail: 343005384@qq.com
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>


struct buffer_object {
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t handle;
	uint32_t size;
	uint8_t *vaddr;
	uint32_t fb_id;
};

struct property_arg {
	uint32_t obj_id;
	uint32_t obj_type;
	char name[DRM_PROP_NAME_LEN+1];
	uint32_t prop_id;
	uint64_t value;
};


struct buffer_object buf;
struct buffer_object plane_buf[4];


static void write_color(struct buffer_object *bo,unsigned int color)
{
	unsigned int *pt;
	int i;
	
	pt = bo->vaddr;
	for (i = 0; i < (bo->size / 4); i++){
		*pt = color;
		pt++;
	}	
	
}
static void write_color_half(struct buffer_object *bo,unsigned int color1,unsigned int color2)
{
	unsigned int *pt;
	int i,size;
	
	pt = bo->vaddr;
	size = bo->size/4/5;
	for (i = 0; i < (size); i++){
		*pt = (139<<16)|255;
		pt++;
	}	
	for (i = 0; i < (size); i++){
		*pt = 255;
		pt++;
	}	
	for (i = 0; i < (size); i++){
		*pt = (127<<8)| 255;
		pt++;
	}
	for (i = 0; i < (size); i++){
		*pt = 255<<8;
		pt++;
	}
	for (i = 0; i < (size); i++){
		*pt = (255<<16) | (255<<8);
		pt++;
	}
	
}

static int modeset_create_fb(int fd, struct buffer_object *bo)
{
	struct drm_mode_create_dumb create = {};
 	struct drm_mode_map_dumb map = {};
	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
	int ret;

	

	create.width = bo->width;
	create.height = bo->height;
	create.bpp = 32;
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);

	bo->pitch = create.pitch;
	bo->size = create.size;
	bo->handle = create.handle;


	map.handle = create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);

	bo->vaddr = mmap(0, create.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, map.offset);




#if 0
	drmModeAddFB(fd, bo->width, bo->height, 24, 32, bo->pitch,
			   bo->handle, &bo->fb_id);
#else
	offsets[0] = 0;
	handles[0] = bo->handle;
	pitches[0] = bo->pitch;

	ret = drmModeAddFB2(fd, bo->width, bo->height,
		    DRM_FORMAT_XRGB8888, handles, pitches, offsets,&bo->fb_id, 0);
	if(ret ){
		printf("drmModeAddFB2 return err %d\n",ret);
		return 0;
	}

	printf("bo->pitch %d\n",bo->pitch);

			   
#endif



	memset(bo->vaddr, 0xff, bo->size);

	return 0;
}




static int modeset_create_yuvfb(int fd, struct buffer_object *bo)
{
	struct drm_mode_create_dumb create = {};
 	struct drm_mode_map_dumb map = {};
	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
	int ret,virtual_height,hsub,vsub,height;


#if 0
	// for yUV 422 model ,atmel_hlcdc_plane.c atmel_hlcdc_format_to_plane_mode case DRM_FORMAT_YUV422:*mode = ATMEL_HLCDC_YUYV_MODE
	virtual_height = bo->height * 2;
	create.width = bo->width;
	create.height = virtual_height;
	create.bpp = 8;
#else
	virtual_height = bo->height;
	create.width = bo->width;
	create.height = virtual_height;
	create.bpp = 16;


#endif
	
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);

	bo->pitch = create.pitch;
	bo->size = create.size;
	bo->handle = create.handle;


	map.handle = create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);

	bo->vaddr = mmap(0, create.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, map.offset);

	printf("bo->pitch %d\n",bo->pitch);


#if 0
		// for YUV 422 model
		hsub = 2;
		vsub = 1;
		height = bo->height;
	
 		offsets[0] = 0;
 		handles[0] = bo->handle;
 		pitches[0] = bo->pitch;

		pitches[1] = pitches[0] / hsub;
 		offsets[1] = pitches[0] * height;
 		handles[1] = bo->handle;
 		pitches[2] = pitches[1];
		offsets[2] = offsets[1] + pitches[1] * height / vsub;
 		handles[2] = bo->handle;
#else
		offsets[0] = 0;
		handles[0] = bo->handle;
		pitches[0] = bo->pitch;

#endif


	ret = drmModeAddFB2(fd, bo->width, bo->height,
		    DRM_FORMAT_YUYV, handles, pitches, offsets,&bo->fb_id, 0);
	if(ret ){
		printf("drmModeAddFB2 return err %d\n",ret);
		return 0;
	}


	memset(bo->vaddr, 0xff, bo->size);

	return 0;
}


static int modeset_create_planefb(int fd, struct buffer_object *bo)
{

	return 0;
}

static void modeset_destroy_fb(int fd, struct buffer_object *bo)
{
	struct drm_mode_destroy_dumb destroy = {};

	drmModeRmFB(fd, bo->fb_id);

	munmap(bo->vaddr, bo->size);

	destroy.handle = bo->handle;
	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}


void get_planes_property(int fd,drmModePlaneRes *pr)
{
	drmModeObjectPropertiesPtr props;
	int i,j;
	drmModePropertyPtr p;
	
	for(i = 0; i < pr->count_planes; i++) {
		
		printf("planes id %d\n",pr->planes[i]);
		props = drmModeObjectGetProperties(fd, pr->planes[i],
					DRM_MODE_OBJECT_PLANE);
					
		for (j = 0;j < props->count_props; j++) {
			p = drmModeGetProperty(fd, props->props[j]);
			printf("get property ,name %s, id %d\n",p->name,p->prop_id);
			drmModeFreeProperty(p);
		}	
		
		printf("\n\n");
	}
	
}

void set_plane_property(int fd, int plane_id,struct property_arg *p)
{

	drmModeObjectPropertiesPtr props;
	drmModePropertyPtr pt;
	const char *obj_type;
	int ret;
	int i;


	props = drmModeObjectGetProperties(fd, plane_id,
				DRM_MODE_OBJECT_PLANE);
	
	for (i = 0; i < (int)props->count_props; ++i) {

		pt = drmModeGetProperty(fd, props->props[i]);
	
		if (!pt)
			continue;
		if (strcmp(pt->name, p->name) == 0)
			break;
	}

	if (i == (int)props->count_props) {
		printf("%i has no %s property\n",p->obj_id, p->name);
		return;
	}

	p->prop_id = props->props[i];

	ret = drmModeObjectSetProperty(fd, p->obj_id, p->obj_type,
				       p->prop_id, p->value);
	if (ret < 0)
		printf("faild to set property %s,id %d,value %d\n",p->obj_id, p->name, p->value);

	
}


void set_rotation(int fd, int plane_id,int value)
{
	struct property_arg prop;
	

	memset(&prop, 0, sizeof(prop));
	prop.obj_type = 0;
	prop.name[DRM_PROP_NAME_LEN] = '\0';
	prop.obj_id = plane_id;
	memcpy(prop.name,"rotation",sizeof("rotation"));
	prop.value = value; // 1->0^, 2->90^, 3 ->180^,
	set_plane_property(fd,plane_id,&prop);

}

//the alpha is globle setting,e.g. HEO alpha is 255, but still cant cover over1,2
void set_alpha(int fd, int plane_id,int value)
{
	struct property_arg prop;
	

	memset(&prop, 0, sizeof(prop));
	prop.obj_type = 0;
	prop.name[DRM_PROP_NAME_LEN] = '\0';
	prop.obj_id = plane_id;
	memcpy(prop.name,"alpha",sizeof("alpha"));
	prop.value = value; //0~255
	set_plane_property(fd,plane_id,&prop);

}


int cpy_video_buff(unsigned char *cemaro_buf)
{
	memcpy(plane_buf[2].vaddr,cemaro_buf,plane_buf[2].size);
}
int set_frame_buff(int width, int height,unsigned char *cemaro_buf)
{
	int fd;
	drmModeConnector *conn;
	drmModeRes *res;
	drmModePlaneRes *plane_res;
	uint32_t conn_id;
	uint32_t crtc_id,x,y;
	uint32_t plane_id;
	int ret;
	int rotation = 1,alpha = 10;;

	

	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
	if(!fd){
		printf("open DRM device error\n");
		return -1;
	}

	res = drmModeGetResources(fd);
	crtc_id = res->crtcs[0];
	conn_id = res->connectors[0];

	drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	ret = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (ret) {
		printf("failed to set client cap\n");
		return -1;
	}
	plane_res = drmModeGetPlaneResources(fd);
	plane_id = plane_res->planes[0];
	
	printf("get plane count %d,plane_id %d\n",plane_res->count_planes,plane_id);
	


	conn = drmModeGetConnector(fd, conn_id);
	buf.width = conn->modes[0].hdisplay;
	buf.height = conn->modes[0].vdisplay;

	if(conn->modes[0].vdisplay <  height || conn->modes[0].hdisplay < width){
		printf("cemora size over the max limit\n");
		printf("get connector nanme %s,vdisplay %d, hdisplay %d,vrefresh %d\n",conn->modes[0].name,conn->modes[0].vdisplay,\
			conn->modes[0].hdisplay,conn->modes[0].vrefresh);

		return -1;
	}

	modeset_create_fb(fd, &buf);
	drmModeSetCrtc(fd, crtc_id, buf.fb_id,0, 0, &conn_id, 1, &conn->modes[0]);
	write_color(&buf,0xffffffff);


	//getchar();
			
	
	// -------------------  overlay 1
	plane_buf[0].width = 200;
	plane_buf[0].height = 200;
	modeset_create_fb(fd, &plane_buf[0]);
	write_color(&plane_buf[0],0x00ff0000);

	ret = drmModeSetPlane(fd, plane_res->planes[1], crtc_id, plane_buf[0].fb_id, 0,
			50, 50, plane_buf[0].width,plane_buf[0].height,
			0, 0, (plane_buf[0].width) << 16, (plane_buf[0].height) << 16);
	if(ret < 0)
		printf("drmModeSetPlane err %d\n",ret);
	

	// -------------------  overlay 2
	plane_buf[1].width = 200;
	plane_buf[1].height = 200;
	modeset_create_fb(fd, &plane_buf[1]);
	write_color(&plane_buf[1],0x0000ff00);

	ret = drmModeSetPlane(fd, plane_res->planes[2], crtc_id, plane_buf[1].fb_id, 0,
			200, 200, plane_buf[1].width,plane_buf[1].height,
			0, 0, plane_buf[1].width << 16, plane_buf[1].height << 16);
	if(ret < 0)
		printf("drmModeSetPlane err %d\n",ret);
	
	
	get_planes_property(fd,plane_res);
	



	// -------------------  HEO	
	plane_buf[2].width = width;
	plane_buf[2].height = height;
	modeset_create_yuvfb(fd, &plane_buf[2]);
	//write_color_half(&plane_buf[2],0x000000ff,0x00000000);

	//printf("press any key continue\n");
	//getchar();
	
	
	x = 0;
	y = 0;
	//set_alpha(fd,plane_res->planes[3],255);
	
	ret = drmModeSetPlane(fd, plane_res->planes[3], crtc_id, plane_buf[2].fb_id, 0,
			x, y, plane_buf[2].width,plane_buf[2].height,
			0<<16, 0<<16, 
			(plane_buf[2].width) << 16, (plane_buf[2].height) << 16);
	if(ret < 0)
		printf("drmModeSetPlane err %d\n",ret);	
		
	memcpy(plane_buf[2].vaddr,cemaro_buf,plane_buf[2].size);
#if 0	
		
	while(1){
		/* Source values are 16.16 fixed point */
		ret = drmModeSetPlane(fd, plane_res->planes[3], crtc_id, plane_buf[2].fb_id, 0,
				x, y, plane_buf[2].width,plane_buf[2].height,
				0<<16, 0<<16, 
				(plane_buf[2].width) << 16, (plane_buf[2].height) << 16);
		if(ret < 0)
			printf("drmModeSetPlane err %d\n",ret);	
		
		usleep(10000);
		
		x += 1;
		if(x + plane_buf[2].width >= conn->modes[0].hdisplay){
			x = 0;
			y += 50;
			if(y + plane_buf[2].height >= conn->modes[0].vdisplay)
				y = 0;

		}

		#if 0
		if(x && (x % 400) == 0){
			set_rotation(fd,plane_res->planes[3],++rotation);
			if(rotation >= 4)
				rotation = 0;

			
		}
		#endif
		// the property can work right now,no need to use drmModeSetPlane
		set_alpha(fd,plane_res->planes[3],alpha++);
		if(alpha >= 255)
			alpha = 10;

		

	}
	
	

	modeset_destroy_fb(fd, &buf);

	drmModeFreeConnector(conn);
	drmModeFreePlaneResources(plane_res);
	drmModeFreeResources(res);

	close(fd);
#endif

	return 0;
}
