#include   <stdio.h>
#include   <string.h>
#include   <sys/types.h>
#include   <sys/socket.h>
#include   <sys/ioctl.h>
#include   <netinet/in.h>
#include   <fcntl.h>
#include   <errno.h>
//#include   <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/route.h>
#include <sys/statfs.h>
#include <sys/ioctl.h>

#include"vpx3_prt_include.h"

/*housir: 结构体定义 */
/*FPGA控制命令结构体*/
typedef struct FPGA_CONTROL_CMD_ST
{
    unsigned short usRegAddr; /*寄存器地址*/
    unsigned char ucReseved; /*预留*/
    unsigned char  ucCmd; /*cmd 0:读取1:写寄存器*/ 
}FPGA_CONTROL_CMD;

/*housir: 宏定义 */
#define  FPGA_POSITION_REG           0x10 /*  */
#define  FPGA_TEMP0_REG           0x7C /*  */
#define  FPGA_TEMP1_REG           0x80 /*  */
#define  FPGA_TEMP2_REG           0x84 /*  */
#define  FPGA_TEMP3_REG           0x88 /*  */
#define  FPGA_VOLT0_REG           0xA0 /*  */
#define  FPGA_VOLT1_REG           0xA4 /*  */
#define  FPGA_VOLT2_REG           0xA8 /*  */
#define  FPGA_VOLT3_REG           0xAC /*  */
#define  FPGA_VOLT4_REG           0xB0 /*  */
#define  FPGA_VOLT5_REG           0xB4 /*  */
#define  FPGA_VOLT6_REG           0xB8 /*  */
#define  FPGA_TOTAL_DISK_INFO_REG           0xD0 /*  */
#define  FPGA_AVAILABLE_DISK_INFO_REG           0xD4 /*  */
#define  FPGA_SYS_RUN_STATUS_REG           0xD8/*  */

#define FPGA_REG_READ       0     /*  */
#define FPGA_REG_WRITE       1     /*  */
/*housir: 宏 */
#define NEGATIVE       1            /*  负值*/
#define POSITIVE       0            /* 正值 */

#define TOTAL_SENSOES            10/*一共10个传感器采集点*/
#define SENSOR_TEMP_TOTAL           4            /*4个温度采集点  */
#define SENSOR_V_MAX_NUM       6            /*传感器中测的电压的个数  */

typedef struct lct2991_v_temp_data_tag{
    //    int16_t adc_code;/*housir: 温度采集到的adc_code或者电压采集到的code */
    unsigned char sign ;   /*housir: 符号位 */ 
    int hvtemp ;     /*housir: 温度/电压的整数位 */
    int lvtemp ;     /*housir: 温度/电压的小数位 */    
}st_sensorinfo,*pst_sensorinfo;

/*housir: 全局变量 */
st_sensorinfo stasensor_value [TOTAL_SENSOES] =
{
    {
        .sign     = POSITIVE,
        .hvtemp   = 0,
        .lvtemp   = 0,
    },
    0,
};
int gfd_fpga;/*housir: fpga 字符设备操作的文件描述符*/
/*housir: 函数定义 */
static int fpgaRegRead(unsigned int addr, unsigned int * Value);
static int fpgaRegWrite(unsigned int addr, unsigned int  Value);

/*****************************************************************************
func : ipAutoSet
description : 根据单板的槽位信息自动配置serdes网络的ip/mask信息
input : 网卡ID，IP地址，掩码，网关
output : NA
return : 0：success,1:fail
作者:聂飞
时间:2013-11-18
 *****************************************************************************/
int   ipAutoSet(const   char   *ifname,   const   char   *ipaddr,const char *netmask,const char *gwip)
{
    struct   sockaddr_in   sin;
    struct ifreq ifr;
    int   fd;

    bzero(&ifr,   sizeof(struct   ifreq));
    if   (ifname   ==   NULL)
        return   (-1);
    if   (ipaddr   ==   NULL)
        return   (-1);
    if(gwip == NULL)
        return(-1);
    fd   =   socket(AF_INET,   SOCK_DGRAM,   0);
    if   (fd   ==   -1)
    {
        perror( "Not   create   network   socket   connection\n ");
        return   (-1);
    }
    strncpy(ifr.ifr_name,   ifname,   IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ   -   1]   =   0;
    memset(&sin,   0,   sizeof(sin));
    sin.sin_family   =   AF_INET;
    sin.sin_addr.s_addr   =   inet_addr(ipaddr);
    memcpy(&ifr.ifr_addr,   &sin,   sizeof(sin));
    if   (ioctl(fd,   SIOCSIFADDR,   &ifr)   <   0)
    {
        perror( "Not   setup   interface\n ");
        return   (-1);
    }

    bzero(&ifr,   sizeof(struct   ifreq));
    strncpy(ifr.ifr_name,   ifname,   IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ   -   1]   =   0;
    memset(&sin,   0,   sizeof(sin));
    sin.sin_family   =   AF_INET;
    sin.sin_addr.s_addr   =   inet_addr(netmask);
    memcpy(&ifr.ifr_addr,   &sin,   sizeof(sin));
    if(ioctl(fd, SIOCSIFNETMASK, &ifr ) < 0)
    {
        perror("net mask ioctl error/n");
        return (-1);
    }
    struct rtentry rm;
    bzero(&rm,   sizeof(struct rtentry));
    rm.rt_dst.sa_family = AF_INET;
    rm.rt_gateway.sa_family = AF_INET;
    rm.rt_genmask.sa_family = AF_INET;
    memset(&sin,   0,   sizeof(sin));
    sin.sin_family   =   AF_INET;
    sin.sin_addr.s_addr   =   inet_addr(gwip);
    memcpy(&rm.rt_gateway, &sin,   sizeof(sin));
    rm.rt_dev = (char *)ifname;
    rm.rt_flags = RTF_UP | RTF_GATEWAY ;
    if(ioctl(fd, SIOCADDRT, &rm ) < 0)
    {
        perror("gateway ioctl error/n");
        return (-1);
    }
    ifr.ifr_flags   |=   IFF_UP   |   IFF_RUNNING;
    if (ioctl(fd,   SIOCSIFFLAGS,   &ifr)   <   0)
    {
        perror( "SIOCSIFFLAGS ");
        return   (-1);
    }
    return   (0);
}
/*****************************************************************************
  函数名 : vpx3Ssd1GetSlotInfo
  函数描述 : 从字符设备(读取FPGA)获取SSD1单板获取单板槽位信息
  输入参数 : 
  输出参数 :
  返回值 :
  作者:聂飞
  时间:2013-11-18
  备注:
  GAP	奇偶校验   bit7
  CABIN0	设备ID编号 bit6
  CABIN1	设备ID编号 bit5
  GA4*	插槽编号   bit4
  GA3*	插槽编号   bit3
  GA2*	插槽编号   bit2
  GA1*	插槽编号   bit1
  GA0*	插槽编号   bit0
 *****************************************************************************/
static unsigned char vpx3Ssd1GetSlotInfo(void)
{
#define FPGA_REG_NUM 40
#define FPAG_REG_SLOT_OFFSET 4
#define ERROR_CHAR -1
    unsigned char aucReg[FPGA_REG_NUM];
    unsigned int uiRegValue;



    //	fd = open("/dev/fpga_reg",O_RDWR,S_IRUSR | S_IWUSR);
    if (-1 == fpgaRegRead(FPGA_POSITION_REG, &uiRegValue))
    {
        PRT_IBMC_ERROR("fpgaRegRead error!\n");
    }

   PRT_IBMC_DEBUG("===> %s uiRegValue [0x%x]\n", __func__, uiRegValue);

 //   PRT_IBMC_DEBUG("===> %s:uiRegValue [0x%x] \n", __func__, uiRegValue);
#if 0
    if(ERROR_CHAR != fd)
    {
        memset(aucReg, 0, FPGA_REG_NUM);
        read(fd,&aucReg,FPGA_REG_NUM);
        close(fd);
    }
#endif
    /*fpga reg 0x4 此处的reg地址为偏移后的地址，属于逻辑地址减去实际
      地址0xf0000000*/
    return (uiRegValue&0xFF000000) >> 24;
}

/*****************************************************************************
  函数名 : bmcAutoConfigEth
  函数描述 : 1:依据槽位自动完成IP地址设置
  输入参数 : 
  输出参数 :
  返回值 :
  作者:聂飞
  时间:2013-11-18
  备注:
  GAP	奇偶校验   bit7
  CABIN0	设备ID编号 bit6
  CABIN1	设备ID编号 bit5
  GA4*	插槽编号   bit4
  GA3*	插槽编号   bit3
  GA2*	插槽编号   bit2
  GA1*	插槽编号   bit1
  GA0*	插槽编号   bit0
 *****************************************************************************/
static int bmcAutoConfigEth(void)
{
#define LENGTH_IPV4 16
#define SET_SUCCESS 0
    /*第三个网络字节序.后字节偏移*/
#define IP_ADDR_4_OFFSET 10
    char *pNetMaskDefault = "255.255.255.0";
    char *pGateWayDefault = "192.168.1.1";
    char *pIpAddr="192.168.1.251";
    char aucIpAddr[LENGTH_IPV4 + 1];
    char ucSlotInfoReg = 0;
    unsigned char ucSlotId = 0;
    unsigned char ucDeviceId = 0;
    unsigned char ucValue = 0;
    int iRv = 0x1;

    /*设置默认IP地址*/
    memcpy(aucIpAddr,"192.168.1.251",LENGTH_IPV4);
    aucIpAddr[LENGTH_IPV4] = '\0';
    /*获取槽位ID和DEVICE_ID*/
    ucSlotInfoReg = vpx3Ssd1GetSlotInfo();

    PRT_IBMC_DEBUG("===> %s:ucSlotInfoReg [%d] \n", __func__, ucSlotInfoReg);
    /*打桩验证*/
    //	ucSlotInfoReg = 0x74;
    ucDeviceId = (ucSlotInfoReg & 0x60)>> 0x5;
    ucSlotId = ucSlotInfoReg & 0x1f;
    if (((ucDeviceId << 0x5) + ucSlotId) <= 255)
    {
        ucValue = ((ucDeviceId << 0x5) + ucSlotId);
    }
    else
    {
        PRT_IBMC_DEBUG("ip num is more than 255\n");
        return (-1);
    }
    PRT_IBMC_DEBUG("===> [%s] ucValue [0x%x]\n", __func__, ucValue);
    /*依据规则直接修改IP地址0x0 2bit(deviceId) 5bit(slotId)*/
    if(ucValue / 100)
    {
        aucIpAddr[IP_ADDR_4_OFFSET] = ucValue / 100 + '0';
        ucValue = ucValue %100;
        aucIpAddr[IP_ADDR_4_OFFSET +1] = ucValue/10 + '0';
        ucValue = ucValue %10;
        aucIpAddr[IP_ADDR_4_OFFSET +2] = ucValue + '0';
        aucIpAddr[IP_ADDR_4_OFFSET +3] = '\0';
    }
    else if(ucValue / 10)
    {
        aucIpAddr[IP_ADDR_4_OFFSET ] = (ucValue/10) + '0';
        ucValue = ucValue %10;
        aucIpAddr[IP_ADDR_4_OFFSET +1] = ucValue+'0';
        aucIpAddr[IP_ADDR_4_OFFSET +2] = '\0';
    }
    else
    {
        aucIpAddr[IP_ADDR_4_OFFSET] = ucValue + '0';
        aucIpAddr[IP_ADDR_4_OFFSET +1] = '\0';
    }
    //int i = 0;
    //for(i = 0 ;i< 16; i++)
    {
        //	PRT_IBMC_DEBUG("aucIpAddr[%d]=%c\n",i,aucIpAddr[i]);
    }
    /*发起命令设置*/
    pIpAddr = aucIpAddr;
	PRT_IBMC_DEBUG("ip add = %s\n",pIpAddr);
    //ipAutoSet("em1","192.168.1.250","255.255.255.0","192.168.1.1");
    iRv = ipAutoSet("eth1",pIpAddr,pNetMaskDefault,pGateWayDefault);
    if(SET_SUCCESS != iRv)
    {
        PRT_IBMC_ERROR("auto set net ip addr fail,check card...\n");	
    }
}
/**
 * @brief 从fpga指定地址中读取数据
 *
 * @param addr 需要读的fpga中的地址         [IN]
 * @param Value 读出的值                    [OUT]
 *
 * @return 
 */
static int fpgaRegRead(unsigned int addr, unsigned int * Value)
{
    int cmd;
    unsigned int uiRegValue;
    FPGA_CONTROL_CMD stcmd=
    {
        .usRegAddr = (unsigned short)addr,
        .ucReseved = 0,
        .ucCmd     = FPGA_REG_READ,
    };
    if(ioctl(gfd_fpga, *(int *)&stcmd, &uiRegValue) < 0)
    {
        uiRegValue = -1;
        *Value = uiRegValue;
        PRT_IBMC_ERROR(" ioctl read error\n");
        return -1;
    }
    *Value = uiRegValue;

    return 0;
}

/**
 * @brief  向fpga中指定的位置写值
 *
 * @param addr          [IN]
 * @param Value         [IN]
 *
 * @return 
 */
static int fpgaRegWrite(unsigned int addr, unsigned int  Value)
{
    unsigned int uiRegValue = Value;
    FPGA_CONTROL_CMD stcmd=
    {
        .usRegAddr = (unsigned short)addr,
        .ucCmd     = FPGA_REG_WRITE,
    };
    
    if (addr > 0x100)
    {
        PRT_IBMC_ERROR("error ===> %s addr [0x%x] is too large!\n", __func__, addr);
    }
    //	fd = open("/dev/fpga_reg",O_RDWR,S_IRUSR | S_IWUSR);
    if(ioctl(gfd_fpga, *(int *)&stcmd, &uiRegValue) < 0)
    {
        PRT_IBMC_ERROR(" ioctl write error\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
  函数名 : nfsDiskInfoUpdate
  函数描述 : 更新NFS共享存储总容量和剩余可用容量
  输入参数 : 
  输出参数 :
  返回值 :
  作者:聂飞
  时间:2013-11-19
  备注:

 *****************************************************************************/
static int nfsDiskInfoUpdate(void)
{
    struct statfs stDiskInfo;  
    unsigned long long ulBlockSize = 0;//每个block里包含的字节数  
    unsigned long long  ullTotalSize = 0;//总的字节数，f_blocks为block的数目  
    unsigned long long ullAvailableSize = 0;
    unsigned long long ulTotalMSize = 0; //共享磁盘总M数
    unsigned long long ulAvailableMsize = 0; //共享磁盘剩余M数
    //unsigned long ulFreeSize = 0;

    statfs("/mnt", &stDiskInfo);  
    ulBlockSize = stDiskInfo.f_bsize;    
    ullTotalSize = ulBlockSize * stDiskInfo.f_blocks;   
//    PRT_IBMC_DEBUG("Total_size = %llu B = %llu KB = %llu MB\n",   
 //           ullTotalSize, ullTotalSize>>10, ullTotalSize>>20);  

    //ulFreeSize = stDiskInfo.f_bfree * ulBlockSize; //剩余空间的大小  
    ullAvailableSize = stDiskInfo.f_bavail * ulBlockSize;   //可用空间大小  
//   PRT_IBMC_DEBUG("Disk_available = %llu MB \n",ullAvailableSize>>20); 
    ulTotalMSize = ullTotalSize>> 20;
    ulAvailableMsize = ullAvailableSize >> 20;

   PRT_IBMC_DEBUG("Total_size = %llu MB Disk_available = %llu MB \n",ulTotalMSize, ulAvailableMsize); 
    /*将信息刷新到FPAG中IOCTL控制寄存器读写*/
    if (-1 == fpgaRegWrite(FPGA_TOTAL_DISK_INFO_REG, ulTotalMSize))
    {
        PRT_IBMC_ERROR("fpgaRegWrite FPGA_TOTAL_DISK_INFO_REG error!\n");
    }
    if (-1 == fpgaRegWrite(FPGA_AVAILABLE_DISK_INFO_REG, ulAvailableMsize))
    {
        PRT_IBMC_ERROR("fpgaRegWrite FPGA_AVAILABLE_DISK_INFO_REG error!\n");
    }


    return 0; 

}
/**
 * @brief 将温度和电压值 写入FPGA指定内存单元中
 *
 * @return 
 */
static int TempVInfoUpdata(void)
{
    int index = 0;
    int n = 20;
    int dev_fd;
    int i=0;
    int vtemp_v = 0; 
    int tmp=0;/*housir: 用来存取从fpga读出的数据 */

    dev_fd = open("/dev/read_sensor",O_RDWR,S_IRUSR | S_IWUSR);
    if ( dev_fd == -1 ) 
    {
        PRT_IBMC_ERROR("Cann't open file /dev/read_sensor\n");
        exit(1);
    }

    memset(stasensor_value, 0, sizeof(stasensor_value));
    read(dev_fd, stasensor_value, sizeof(stasensor_value));
    for( index=0;index<SENSOR_TEMP_TOTAL;index++)
    {
        vtemp_v = 0;
        vtemp_v =  stasensor_value[index].hvtemp<<8 |  stasensor_value[index].lvtemp;
        if (NEGATIVE == stasensor_value[index].sign )
        {
            vtemp_v &= 0x8000;/*housir: 按照存储规范 最高位为符号位 */
        }
        PRT_IBMC_DEBUG("read v%d v%d tem is %c %d.%0.2d\n", 2*index+1, 2*index+2, stasensor_value[index].sign == NEGATIVE  ? '-' : '+', stasensor_value[index].hvtemp , stasensor_value[index].lvtemp );
        if (-1 == fpgaRegWrite(FPGA_TEMP0_REG + index*sizeof(unsigned int), vtemp_v))
        {
            PRT_IBMC_ERROR("fpgaRegWrite FPGA_TEMP0_REG error!\n");
        }       
 //       fpgaRegRead(FPGA_TEMP0_REG + index*sizeof(unsigned int), &tmp);
 //       PRT_IBMC_DEBUG("read  from fpga 0x%x\n", tmp);
   //     tmp = 0;


    }

    for( index=0;index<SENSOR_V_MAX_NUM;index++)
    {
        vtemp_v = 0;
        vtemp_v =  stasensor_value[index + SENSOR_TEMP_TOTAL].hvtemp<<8 |  stasensor_value[index + SENSOR_TEMP_TOTAL].lvtemp;
      //  PRT_IBMC_DEBUG("stasensor_value[index].hvtemp [%d], ltemp [%d]\n",stasensor_value[index+ SENSOR_TEMP_TOTAL].hvtemp, stasensor_value[index+ SENSOR_TEMP_TOTAL].lvtemp );

        if (NEGATIVE == stasensor_value[index].sign )
        {
            vtemp_v &= 0x8000;/*housir: 按照存储规范 最高位为符号位 */
        }
        PRT_IBMC_DEBUG("read v%d V is %c %d.%0.2d\n", index+1, stasensor_value[index + SENSOR_TEMP_TOTAL].sign == NEGATIVE  ? '-' : '+',stasensor_value[ index + SENSOR_TEMP_TOTAL ].hvtemp ,  stasensor_value[ index + SENSOR_TEMP_TOTAL ].lvtemp );

        if (-1 == fpgaRegWrite(FPGA_VOLT0_REG + index*sizeof(unsigned int), vtemp_v))
        {
            PRT_IBMC_ERROR("fpgaRegWrite FPGA_TEMP0_REG error!\n");
        }      
	//PRT_IBMC_DEBUG("vtemp_v [0x%x]\n", vtemp_v);
        //fpgaRegRead(FPGA_VOLT0_REG + index*sizeof(unsigned int), &tmp);
        //PRT_IBMC_DEBUG("read  from fpga 0x%x\n", tmp);
        //tmp = 0;
    }

    close(dev_fd);

    return 0;

}
/**
 * @brief 检查nfs状态 若nfs正常启动则向fpga中指定位置写1 否则写0
 *
 * @return open 1 else close 0;
 */
static int check_nfs_status()
{
    static unsigned char run_status = 0;

    run_status = 1;/*housir: NFS port status  1成功 0 失败 */
#if 0 /*housir: 用套接字连接nfs特有的端口2049 */
	struct sockaddr_in servaddr;
	int servfd ;
    unsigned long ul = 1;
#define NFS_PORT        2049            /*  NFS的端口 用来检测 端口是否被打开*/
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(NFS_PORT);

	// 客户端调用connect连接NFS服务器端指定socket套接字
    if ( -1 ==(servfd = socket(AF_INET, SOCK_DGRAM, 0)))
    {
        PRT_IBMC_ERROR("socket fail!\n");
    }

    ioctl(servfd, FIONBIO, &ul); //设置为非阻塞模式
     
    if( -1 == connect(servfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))
    {
        run_status = 0;/*housir: NFS port fail */
        PRT_IBMC_ERROR("socket connect fail!\n");
    }
    else
    {
        run_status = 1;
        PRT_IBMC_DEBUG("NFS check connect success!\n");/*housir: NFS port success */
    }

    if ( 0 != close(servfd))
    {
        PRT_IBMC_ERROR("socket closed fail\n");
    }
    else
    {
        PRT_IBMC_DEBUG("socket closed success\n");
    }
#else/*housir: 利用shell命令查询进程名字 */
    FILE *fp = popen("ps -e | grep \'nfsd\' | awk \'{print $4}\' | grep \'\\[nfsd\\]\'", "r");
    
    char buffer[10] = {0};


    if (NULL == fgets(buffer, 10, fp))
    {
        //写0
        run_status = 0;/*housir: NFS port fail */
        PRT_IBMC_ERROR("NFS check connect fail!\n");        
    }
    else/*nfs服务正常启动*/
    {
        //写1
        run_status = 1;
        PRT_IBMC_DEBUG("NFS check connect success!\n");/*housir: NFS port success */
    }

    pclose(fp); //关闭返回的文件指针，注意不是用fclose

#endif
    if (-1 == fpgaRegWrite(FPGA_SYS_RUN_STATUS_REG , run_status))
    {
        PRT_IBMC_ERROR("fpgaRegWrite FPGA_TEMP0_REG error!\n");
    }     

    return 0;
}

/*****************************************************************************
func : main
description : 实现VPX3-SSD1单板的BMC功能
1:依据槽位自动完成IP地址设置
2: update nfs memory /mnt/ total_size and avaliable_size 
input : 
output :
return :
作者:聂飞
时间:2013-11-18
 *****************************************************************************/
void main()
{
#define SET_SUCCESS 0
    int iRv = 0x1;

    gfd_fpga = open("/dev/fpga",O_RDWR,S_IRUSR | S_IWUSR);

    if(-1 == gfd_fpga)
    {
        PRT_IBMC_ERROR("open failed !\n");
        return ;
    }

    /*自动设置网卡IP地址上电配置一次即可*/
    iRv = bmcAutoConfigEth();
    if(SET_SUCCESS != iRv)
    {
        PRT_IBMC_ERROR("bmc config eth \n");	
    }

    while(1)
    {
        sleep(5);
        /*更新磁盘容量信息*/
        (void)nfsDiskInfoUpdate();
        (void)TempVInfoUpdata();
        (void)check_nfs_status();

    }
    return;
}

