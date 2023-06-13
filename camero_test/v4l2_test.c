#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>  
#include <fcntl.h>             
#include <unistd.h>
#include <sys/mman.h> 
 #include<time.h>
#include <linux/videodev2.h>

struct buffer{  
	void *start;  
	unsigned int length;  
}*buffers; 


#define CAM_PIX_FMT V4L2_PIX_FMT_YUYV
#define  VIDEO_WIDTH  640
#define  VIDEO_HEIGHT  480
#define DEV_PATH "/dev/video0" //0/9 
#define CAPTURE_FILE "yuv_raw.bin"

int main()
{  
	//1.open device.打开摄像头设备 
	int index = -1;
	int fd = open(DEV_PATH,O_RDWR,0);
	if(fd<0){
		printf("open device failed.\n");
	}
 

    //2.search device property.查询设备属性

	struct v4l2_capability cap;
	if(ioctl(fd,VIDIOC_QUERYCAP,&cap)==-1){
		printf("VIDIOC_QUERYCAP failed.\n");
	}
	printf("VIDIOC_QUERYCAP success.->DriverName:%s CardName:%s BusInfo:%s\n",\
		cap.driver,cap.card,cap.bus_info);//device info.设备信息    
		

	//3.show all supported format.显示所有支持的格式
	struct v4l2_fmtdesc fmtdesc;
	fmtdesc.index = 0; //form number
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//frame type  
	while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc) != -1){  
        //if(fmtdesc.pixelformat && fmt.fmt.pix.pixelformat){
            printf("VIDIOC_ENUM_FMT pixelformat:%s,%d\n",fmtdesc.description,fmtdesc.pixelformat);
        //}

		if(fmtdesc.pixelformat == CAM_PIX_FMT)
			index = fmtdesc.index;
		
        fmtdesc.index ++;
    }


	if(index == -1){
		printf("camero dont support YUYV format\n");
		return 0;
	}

    struct v4l2_format fmt;
	memset ( &fmt, 0, sizeof(fmt) );
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(fd,VIDIOC_G_FMT,&fmt) == -1) {
	   printf("VIDIOC_G_FMT failed.\n");
	   return -1;
    }

  	printf("VIDIOC_G_FMT width %ld, height %d, olorspace is %ld\n",fmt.fmt.pix.width,fmt.fmt.pix.height,fmt.fmt.pix.colorspace);
	
	
    //V4L2_PIX_FMT_RGB32   V4L2_PIX_FMT_YUYV   V4L2_STD_CAMERA_VGA  V4L2_PIX_FMT_JPEG
	fmt.fmt.pix.pixelformat = CAM_PIX_FMT;	
	if (ioctl(fd,VIDIOC_S_FMT,&fmt) == -1) {
	   printf("VIDIOC_S_FMT failed.\n");
	   return -1;
    }


	if (ioctl(fd,VIDIOC_G_FMT,&fmt) == -1) {
	   printf("VIDIOC_G_FMT failed.\n");
	   return -1;
    }
  	//printf("VIDIOC_G_FMT sucess.->fmt.fmt.width is %ld\nfmt.fmt.pix.height is %ld\n\
	//	fmt.fmt.pix.colorspace is %ld\n",fmt.fmt.pix.width,fmt.fmt.pix.height,fmt.fmt.pix.colorspace);


	//6.1 request buffers.申请缓冲区
	struct v4l2_requestbuffers req;  
	req.count = 1;//frame count.帧的个数 
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;//automation or user define．自动分配还是自定义
	if ( ioctl(fd,VIDIOC_REQBUFS,&req)==-1){  
		printf("VIDIOC_REQBUFS map failed.\n");  
		close(fd);  
		exit(-1);  
	} 

	//6.2 manage buffers.管理缓存区
	//应用程序和设备３种交换方式：read/write，mmap，用户指针．这里用memory map.内存映射
	unsigned int n_buffers = 0;  
	//buffers = (struct buffer*) calloc (req.count, sizeof(*buffers)); 
	buffers = calloc (req.count, sizeof(*buffers));


	//struct v4l2_buffer buf;   
	for(n_buffers = 0; n_buffers < req.count; ++n_buffers)
	{  
		struct v4l2_buffer buf;  
		memset(&buf,0,sizeof(buf)); 
		buf.index = n_buffers; 
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
		buf.memory = V4L2_MEMORY_MMAP;  
		//查询序号为n_buffers 的缓冲区，得到其起始物理地址和大小
		if(ioctl(fd,VIDIOC_QUERYBUF,&buf) == -1)
		{ 
			printf("VIDIOC_QUERYBUF failed.\n");
			close(fd);  
			exit(-1);  
		} 

  		//memory map
		buffers[n_buffers].length = buf.length;	
		buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,buf.m.offset);  
		if(MAP_FAILED == buffers[n_buffers].start){  
			printf("memory map failed.\n");
			close(fd);  
			exit(-1);  
		} 

		//Queen buffer.将缓冲帧放入队列 
		if (ioctl(fd , VIDIOC_QBUF, &buf) ==-1) {
		    printf("VIDIOC_QBUF failed.->n_buffers=%d\n", n_buffers);
		    return -1;
		}

	} 

	//7.使能视频设备输出视频流
	enum v4l2_buf_type type; 
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	if (ioctl(fd,VIDIOC_STREAMON,&type) == -1) {
		printf("VIDIOC_STREAMON failed.\n");
		return -1;
	}

	//8.DQBUF.取出一帧
    struct v4l2_buffer buf;  
	memset(&buf, 0, sizeof(buf)); 
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        printf("VIDIOC_DQBUF failed.->fd=%d\n",fd);
        return -1;
    }  

	set_frame_buff(fmt.fmt.pix.width,fmt.fmt.pix.height,buffers[0].start);
	while(1){

		ioctl(fd, VIDIOC_QBUF, &buf);
		ioctl(fd, VIDIOC_DQBUF, &buf);
		cpy_video_buff(buffers[0].start);
#if 0
		for(i=0;i<2;i++){
		buf.index = n_buffers; 
		ioctl(fd, VIDIOC_QBUF, &buf);

		set_frame_buff(VIDEO_WIDTH,VIDEO_HEIGHT,buffers[buf.index].start);
		ioctl(fd, VIDIOC_DQBUF, &buf)
		}
#endif
	}

	
	
	while(1);
	
	
	#if 0
	//9.Process the frame.处理这一帧
    FILE *fp = fopen(CAPTURE_FILE, "wb");
    if (fp < 0) {
        printf("open frame-data-file failed.\n");
        return -1;
    }
	printf("open frame-data-file success.\n");
    fwrite(buffers[buf.index].start, 1, buf.length, fp);
    fclose(fp);
    printf("save one frame success.\n"); //%s, CAPTURE_FILE
	#endif
	
	
	
	//10.QBUF.把一帧放回去
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        printf("VIDIOC_QBUF failed.->fd=%d\n",fd);
        return -1;
    } 
	printf("VIDIOC_QBUF success.\n");

	//11.Release the resource．释放资源
    int i;
	for (i=0; i< 2; i++) {
        munmap(buffers[i].start, buffers[i].length);
		printf("munmap success.\n");
    }

	close(fd); 
	printf("Camera test Done.\n"); 
	return 0;  
}  
