#include "xhci.h"
#include <linux/module.h>
#include <linux/kernel.h>

// 在模块顶部添加BlockData结构体定义
typedef struct {
    int number;
    char type[64];
    char deque[20];
    char doorbell[20];
    char iman[20];
    char eint[20];
    char data_trb[20];
    char dma_addr[20];
    char dma_len[20];
    char TD_flag[20];
} BlockData;
BlockData current = {0};
// 添加解析函数
static void parse_block_line(BlockData *block, const char *line) {
    char key[32], value[32];
    const char *p = line;

    p = strchr(p, '"') + 1;
    char *end = strchr(p, '"');
    strncpy(key, p, end - p);
    key[end - p] = '\0';

    p = strchr(end, ':') + 1;
    while (*p && isspace(*p)) p++;
    end = p + strlen(p);
    while (end > p && isspace(*(end-1))) end--;
    strncpy(value, p, end - p);
    value[end - p] = '\0';

    if (strcmp(key, "deque") == 0) 
        strcpy(block->deque, value);
    else if (strcmp(key, "doorbell") == 0) 
        strcpy(block->doorbell, value);
    else if (strcmp(key, "iman") == 0) 
        strcpy(block->iman, value);
    else if (strcmp(key, "eint") == 0) 
        strcpy(block->eint, value);
    else if (strcmp(key, "data_trb") == 0) 
        strcpy(block->data_trb, value);
    else if (strcmp(key, "dma_addr") == 0) 
        strcpy(block->dma_addr, value);
    else if (strcmp(key, "dma_len") == 0) 
        strcpy(block->dma_len, value);
    else if (strcmp(key, "TD_flag") == 0) 
        strcpy(block->TD_flag, value);
}

// 添加配置解析函数
static int parse_config(void) {
    struct file *fp;
    loff_t pos = 0;
    char line[512];

    int in_block = 0;
    mm_segment_t old_fs;

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    fp = filp_open("/path/to/output_3_19_myloog.txt", O_RDONLY, 0);
    if (IS_ERR(fp)) {
        printk(KERN_ERR "Error opening config file\n");
        return PTR_ERR(fp);
    }

    while (kernel_read(fp, line, sizeof(line)-1, &pos) > 0) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        if (strstr(line, "Block ")) {
            if (in_block) {
                // 这里处理块数据，例如设置硬件参数
                printk("Loaded Block %d (%s)\n", current.number, current.type);
            }
            memset(&current, 0, sizeof(current));
            sscanf(line, "Block %d - Type: %63[^\n]", 
                 &current.number, current.type);
            in_block = 1;
        }
        else if (in_block && strstr(line, "block[\"")) {
            parse_block_line(&current, line);
            queue_data(DIR_IN);
        }
    }

    if (in_block) {
        printk("Loaded Block %d (%s)\n", current.number, current.type);
    }

    filp_close(fp, NULL);
    set_fs(old_fs);
    return 0;
}





int TRB_CYCLE;
int CYCLE_TEMP=0;
void __iomem * microframe_index;
//void *data_total=NULL;
int record_frame=0;
int snd_usb_ctl_msg(struct usb_device *dev, unsigned int pipe, __u8 request,
            __u8 requesttype, __u16 value, __u16 index, void *data,
            __u16 size)
{
    int err;
    void *buf = NULL;
    int timeout;

    if (usb_pipe_type_check(dev, pipe))
        return -EINVAL;

    if (size > 0) {
        buf = kmemdup(data, size, GFP_KERNEL);
        if (!buf)
            return -ENOMEM;
    }

    if (requesttype & USB_DIR_IN)
        timeout = USB_CTRL_GET_TIMEOUT;
    else
        timeout = USB_CTRL_SET_TIMEOUT;

    err = usb_control_msg(dev, pipe, request, requesttype,
                  value, index, buf, size, timeout);

    if (size > 0) {
        memcpy(data, buf, size);
        kfree(buf);
    }
    return err;
}

struct usb_hcd *hcd;

struct my_usb_audio {
  __le32 __iomem *db;
  uint8_t ep_in_index;
  __le32 *ep_in_ring;
  uint32_t ep_in_pointer;
};
int temp=0;
#define EP_ISOC_IN 5
static struct my_usb_audio *mua;
static void *data[520];
static dma_addr_t data_dma;

// static void *get_stor_dc(void *dcbaa, uint32_t rs_num)
// {
//   void *stor_dc;
// for (int i = 1; i <= 31; ++i) {
// pr_info("[YLOG]\tdc_s%d_dma: %016llx\n", i, xhci_virt_readq(dcbaa + 0x8 * i));
// }
//   for (int i = 1; i <= 31; ++i) {
//     dma_addr_t dc_s_dma = xhci_virt_readq(dcbaa + 0x8 * i);
//     pr_info("[YLOG]\tdc_s%d_dma: %016llx\n", i, dc_s_dma);
//     if (dc_s_dma == 0) {
//       break;
//     }

//     void *dc_s = xhci_dma_to_virt(dc_s_dma);
//     void *dc_s_sc = dc_s + 0x00;

//     uint32_t sc_rs = SCRS(xhci_virt_readl(dc_s_sc + 0x00));

//     if (sc_rs == rs_num) {
//       pr_info("[YLOG]\tstor_dc_dma: %016llx\n", (uint64_t)dc_s_dma);
//       stor_dc = dc_s;
//       break;
//     }
//   }

//   return stor_dc;
// }

// static void *get_stor_in_ec(void *stor_dc)
// {
//   void *stor_in_ec;

//   void *stor_dc_sc = stor_dc;
//   //uint32_t stor_dc_scce = SCCE(xhci_virt_readl(stor_dc_sc + 0x00));

//   for (uint32_t i = 1; i <= 31; ++i) {
//     void *ec = stor_dc + 0x20 * i;
//     printk("%d",i);
// print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 16, 1, ec, 0x20, false);
//     uint32_t ec_type = ECEPTYPE(xhci_virt_readl(ec + 0x4));
// printk("[zxk]ec_type:%d",ec_type);
//     if (ec_type == 5) {
//     printk("yes");
//       stor_in_ec = ec;
//       break;
//     }
//   }

//   return stor_in_ec;
// }

// static void init_audio_param(uint32_t rs_num)
// {
//   dma_addr_t dcbaa_dma = xhci_readq(op_reg + DCBAAP) & DCBAAMASK;
//   //current_frame_id = readl(&xhci->run_regs->microframe_index);
//   microframe_index= run_reg;
//   printk("microframe_index:%x",microframe_index);
//   pr_info("[zxk]\tdcbaa_dma: %016llx\n", dcbaa_dma);
//   void *dcbaa = xhci_dma_to_virt(dcbaa_dma);
//   printk("[zxk]dcbaa_dma:%llx",dcbaa_dma);

//   void *stor_dc = get_stor_dc(dcbaa, rs_num);
//   print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 16, 1, stor_dc, 16, false);
//   uint32_t slot_id = get_slot_id(stor_dc, dcbaa);
  
//   mua->db = get_slot_db(dba, slot_id);

//   void *stor_in_ec = get_stor_in_ec(stor_dc);
//   printk("[zxk]stor_in_ec:%llx",stor_in_ec);
//   mua->ep_in_index = get_ep_index(stor_in_ec, stor_dc);

//   mua->ep_in_ring = get_transfer_ring(stor_in_ec);

//   mua->ep_in_pointer = 0;
// }


static void queue_data(uint8_t dir)
{
    void *data_trb;
//   void *data_trb = mua->ep_in_ring + 0x4 * mua->ep_in_pointer;  //generic_trb
//   //uint32_t data_trb_status = TRB_TYPE(TRB_NORMAL) | TRB_IOC | (dir == DIR_IN ? TRB_ISP : 0) | TRB_ISOC;
// uint32_t data_trb_status=0x1424;
  uint32_t field0, field1, field2, field3;


// 	if (mua->ep_in_pointer ==255) {
//         if (CYCLE_TEMP==1){
// 		    TRB_CYCLE ^= 1;
//             CYCLE_TEMP=0;
//         }
//         else{
//             CYCLE_TEMP=1;

//         }
// 		void *trb_temp = mua->ep_in_ring + 0x4 * mua->ep_in_pointer+ 0x3;
//         *(uint32_t*)trb_temp ^=cpu_to_le32(1<<0);
//         dcache_clean_poc(data_trb,data_trb+0x10);
//         printk("data_trb[0]:%llx,data_trb[1]:%llx",*(uint32_t*)data_trb,*(uint32_t*)(data_trb+0x4));
//         mua->ep_in_ring=xhci_dma_to_virt(((uint64_t)(*(uint32_t*)(data_trb+0x4)) << 32) | (*(uint32_t*)data_trb));
//         mua->ep_in_pointer=0;
// 	}
//   wmb();

//   field0 = lower_32_bits(data_dma);
//   field1 = upper_32_bits(data_dma);
//   field2 = data_size;
//   field3 = data_trb_status;
char *endptr;
  field0 = lower_32_bits(strtol(current->dma_addr, &endptr, 16));
  field1 = upper_32_bits(strtol(current->dma_addr, &endptr, 16));
  field2 = strtol(current->dma_len, &endptr, 16);
  field3 = strtol(current->TD_flag, &endptr, 16);

//   field3 = field3 | (0x800<<20);


	// field3 = field3 | TRB_CYCLE;
  printk("mua->ep_in_pointer:%d",mua->ep_in_pointer);
	// data_trb = mua->ep_in_ring + 0x4 * mua->ep_in_pointer;
    data_trb=strtol(current->data_trb, &endptr, 16);
  xhci_queue_trb(data_trb, field0, field1, field2, field3);
  mua->ep_in_pointer++;
  xhci_ring_doorbell(strtol(current->doorbell, &endptr, 16),mua->db);
// for (uint8_t i = 3; i <= 7; ++i) {
//   xhci_ring_doorbell(i, mua->db);
// }
  //xhci_ring_doorbell(mua->ep_in_index, mua->db);
}




////////////////////////////////////////////////////////////////
#define global_xhci_base_addr 0xfebd0000
static void *xhci_base;

void __iomem *cap_reg;
void __iomem *op_reg;
void __iomem *run_reg;
void __iomem *dba;

void xhci_ring_doorbell(uint32_t value, void *db_addr) {
  __le32 __iomem *db = (__le32 __iomem *)db_addr;
  writel(value, db);
  readl(db);

  pr_info("[YLOG]\tdoorbell: db = 0x%px\tvalue = 0x%x\n", db, value);
}

void xhci_queue_trb(void *trb_addr, uint32_t field0, uint32_t field1, uint32_t field2, uint32_t field3) {
  __le32 *trb = (__le32 *)trb_addr;

  trb[0] = field0;
  trb[1] = field1;
  trb[2] = field2;
  wmb();
  trb[3] = field3;

dcache_clean_poc((char*)trb,(char*)trb+0x10);
  pr_info("[YLOG]\ttrb: 0x%px\tfield0 = 0x%08x, field1 = 0x%08x, field2 = 0x%08x, field3 = 0x%08x\n", trb, field0, field1, field2, field3);
}



// int xhci_reg_base_init(void) {
// xhci_base=global_xhci_base_addr;
//   cap_reg = xhci_base + 0x0;
//   printk("[zxk]cap_reg:",cap_reg);
//   op_reg = cap_reg + readb(cap_reg + CAPLENGTH);
//   run_reg = xhci_base + (readl(cap_reg + RTSOFF) & RTSMASK);
//   dba = xhci_base + (readl(cap_reg + DBOFF) & DBAMASK);
  

//   return 0;
// }
/// /////////////////////////////////// /////////////////////////////////// ////////////////////////////////
static int usb_mass_stor_read_10(uint32_t rs_num)
{
  pr_info("[YLOG]\tsyscall: usb_mass_stor_read_10\n");

char *trans_buffer;

struct usb_bus *found_bus = NULL;
idr_for_each(&usb_bus_idr, usb_bus_callback, &found_bus);
hcd=bus_to_hcd(found_bus);

struct usb_device *usb_dev = NULL;
//const struct { unsigned short vendor_id; unsigned short product_id; } match_data = {0x1234, 0x5678};

usb_dev = usb_find_device();

int err;

/////////////////////////////////////////////////4 front control message
usb_set_interface(usb_dev, 2, 1);
msleep(20);

unsigned char datax[3];
datax[0] = 0x80;
datax[1] = 0xbb;
datax[2] = 0x00;

__u16 fmt_endpoint=0x3;
printk("success_2");
err = snd_usb_ctl_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0), UAC_SET_CUR,
                  USB_TYPE_CLASS | USB_RECIP_ENDPOINT | USB_DIR_OUT,
                  UAC_EP_CS_ATTR_SAMPLE_RATE << 8,
                  fmt_endpoint, datax, sizeof(datax));
msleep(20);
err = snd_usb_ctl_msg(usb_dev, usb_rcvctrlpipe(usb_dev, 0), UAC_GET_CUR,
                  USB_TYPE_CLASS | USB_RECIP_ENDPOINT | USB_DIR_IN,
                  UAC_EP_CS_ATTR_SAMPLE_RATE << 8,
                  fmt_endpoint, datax, sizeof(datax));









//   init_audio_param(rs_num);
data_total = kzalloc(0xd0*520, GFP_KERNEL);
  
  if (!data_total) {
    pr_err("[YLOG]\tkzalloc failed\n");
    return -ENOMEM;
  }
mysig=1;
int ret=parse_config();
//   for (int i = 0; i < 520; i++) {
//     printk("num_I:%d",i);
//     data[i] = kzalloc(0xd0, GFP_KERNEL);
//     data_dma = xhci_virt_to_dma(data[i]);
    
//     queue_data(data_dma, 0xd0, DIR_IN);
//     //dcache_inval_poc(data,data+ 0xd0);

//     //memcpy(data_total+i*0xd0,data,0xd0);
//     //kfree(data);
//   }

// msleep(1000);
// for (int i = 0; i < 520; i++) {
// kfree(data[i]);

// }
mysig=0;
  pr_info("[YLOG]\tdata[0x00] = 0x%02x\n", xhci_virt_readb(data_total + 0x00));
  
  struct file *file;
  file =filp_open("/home/test/Desktop/test1",O_WRONLY | O_CREAT | O_APPEND , S_IWUSR | S_IRUSR);
                      
  if (IS_ERR(file)){
      printk("Failed_to_open");
  }
  else{
    loff_t pos=0;
    ssize_t ret1=kernel_write(file,data_total,0xc0*1000,&pos);
    if (ret1>=0){
      printk("success_write,%zd bytes",ret1);
    }
    else{
      printk("failed_write");
    }
    filp_close(file, NULL);
  }
  
  

  kfree(data_total);
  return 0;
}

static dev_t devno;
static struct cdev cdev;
static struct class *class;



static int hello_open(struct inode *inode, struct file *file) {
    return 0;
}

static long hello_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {

    switch(cmd) {
        case MY_IOCTL_READ:
        TRB_CYCLE=(1<<0);
        // if (temp==0){
          
        //   xhci_reg_base_init(); 
        //   temp=1;
        // }
            printk("test_success");
            int ret;
            
            ret=usb_mass_stor_read_10(0);
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}




static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = hello_open,
    .unlocked_ioctl = hello_ioctl,
};

static int __init usb_mass_stor_init(void)
{
  printk("[YLOG]\tusb_mass_stor_init\n");

    alloc_chrdev_region(&devno, 0, 1, "hello1");
    class = class_create("hello1");
    device_create(class, NULL, devno, NULL, "hello1");
    cdev_init(&cdev, &fops);
    cdev_add(&cdev, devno, 1);

  mua = kmalloc(sizeof(struct my_usb_audio), GFP_KERNEL);
  if (!mua) {
    pr_err("[YLOG]\tkmalloc failed\n");
    return -ENOMEM;
  }

  

  return 0;
}

static void __exit usb_mass_stor_exit(void)
{
  pr_info("[YLOG]\tusb_mass_stor_exit\n");
  kfree(mua);
  unregister_chrdev_region(devno, 1);
  device_destroy(class, devno);
  class_unregister(class);
  class_destroy(class);
}

module_init(usb_mass_stor_init);
module_exit(usb_mass_stor_exit);
