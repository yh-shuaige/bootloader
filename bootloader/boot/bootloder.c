#define LOG_TAG    "boot"

#include "main.h"

#define BOOTLOADER_VERSION_MAJOR    1
#define BOOTLOADER_VERSION_MINOR    0
#define uart_buffer_size     512ul
#define BL_TIMEOUT_MS        500ul
#define BL_PACKET_HEAD_SIZE         128ul
#define BL_PACKET_PAYLOAD_SIZE      4096ul
#define BL_PACKET_PARAM_SIZE        (BL_PACKET_HEAD_SIZE + BL_PACKET_PAYLOAD_SIZE)

bool volatile need_recovery_guide = false;   // 状态变量：是否需要引导用户恢复备份
bool bl_successful = false;
extern void bl_button_wait_release(void);
void turn_backup(void);
bool verify_application(void);
bool app_restroed(void);
typedef enum
{
    BL_SM_IDLE,
    BL_SM_START,
    BL_SM_OPCODE,
    BL_SM_PARAM,
    BL_SM_CRC,
} bl_state_machine_t;
void application_boot(void);
typedef enum
{
    BL_OP_NONE = 0x00,
    BL_OP_INQUIRY = 0x10,
    BL_OP_BOOT = 0x11,
    BL_OP_RESET = 0x1F,
    BL_OP_ERASE = 0x20,
    BL_OP_WRITE = 0x22,
    BL_OP_VERIFY,
    BL_OP_END,
} bl_op_t;

typedef struct
{
    uint8_t subcode;
} bl_inquiry_param_t;

typedef struct
{
    bl_op_t  opcode;
    uint16_t length;
    uint32_t crc;

    uint8_t  param[BL_PACKET_PARAM_SIZE];
    uint16_t index;
} bl_pkt_t;

typedef struct
{
    uint8_t data[16];
    uint16_t index;
} bl_rx_t;

typedef struct
{
    bl_pkt_t pkt;   //用于存储完整解析后的数据包
    bl_rx_t rx;     //用于接收过程中的临时数据
    bl_state_machine_t sm;  //状态机当前状态
} bl_ctrl_t;



typedef enum
{
    BL_ERR_OK,
    BL_ERR_OPCODE,

    BL_ERR_OVERFLOW,
    BL_ERR_TIMEOUT,
    BL_ERR_FORMAT,
    BL_ERR_VERIFY,
    BL_ERR_PARAM,
    BL_ERR_UNKNOWN = 0xff,
} bl_err_t;


typedef struct
{
    uint32_t address;
    uint32_t size;
    uint8_t data[];
} bl_write_param_t;

static uint8_t uart_buff[uart_buffer_size]; 
static bl_ctrl_t bl_ctrl;
static uint32_t last_ptktime;    
static ringbuffer8_t serial_rb;

void serial_resc_handle(uint8_t *data, uint32_t len)
{
    rb8_puts(serial_rb,data,len);
}

static void bl_reset(bl_ctrl_t *ctrl)
{
    ctrl->sm = BL_SM_IDLE;
    ctrl->pkt.index = 0;
    ctrl->rx.index = 0;
}


typedef enum
{
    BL_INQUIRY_VERSION,
    BL_INQUIRY_BLOCK_SIZE,
} bl_inquiry_t;

typedef struct
{
    uint32_t address;
    uint32_t size;
} bl_erase_param_t;


typedef struct
{
    uint32_t address;
    uint32_t size;
    uint32_t crc;
} bl_verify_param_t;

static bool backup_verify(void)
{
    // 首先检查CRC存储区域是否有效
    uint32_t stored_crc = *(uint32_t *)FLASH_BACKUP_CRC_ADDRESS;
    if(stored_crc == 0xFFFFFFFF) {
        log_d("No valid backup CRC stored");
        return false;
    }
    
    // 计算备份区的实际CRC
    uint32_t actual_crc = crc32_update(0, (uint8_t *)FLASH_BACKUP_ADDRESS, FLASH_APP_SIZE);
    
    // 调试信息
    log_d("Stored CRC: 0x%08X, Actual CRC: 0x%08X", stored_crc, actual_crc);
    
    return (actual_crc == stored_crc);
}

static void bl_response(bl_op_t op, uint8_t *data, uint16_t length)
{
    const uint8_t head = 0xAA;

    uint32_t crc = 0;
    crc = crc32_update(crc, (uint8_t *)&head, 1);
    crc = crc32_update(crc, (uint8_t *)&op, 1);
    crc = crc32_update(crc, (uint8_t *)&length, 2);
    crc = crc32_update(crc, data, length);

    bl_usart_write((uint8_t *)&head, 1);
    bl_usart_write((uint8_t *)&op, 1);
    bl_usart_write((uint8_t *)&length, 2);
    bl_usart_write(data, length);
    bl_usart_write((uint8_t *)&crc, 4);
}

static void bl_response_ack(bl_op_t op,bl_err_t err)
{
    bl_response(op,(uint8_t *)&err,1);
}

static void bl_op_inquiry_handler(uint8_t *data,uint16_t len)
{
    log_i("inquiry");
    bl_inquiry_param_t *write = (void *)data;
    if(len != sizeof(bl_inquiry_param_t))
    {
        log_w("length dismatch %d != %d",len,sizeof(bl_inquiry_param_t));
        bl_response_ack(BL_OP_INQUIRY, BL_ERR_PARAM);
        return;
    }
    log_i("subcode = %02x",write->subcode);
    switch (write->subcode)
    {
    case BL_INQUIRY_VERSION:
    {   uint8_t version[] = {BOOTLOADER_VERSION_MAJOR,BOOTLOADER_VERSION_MINOR};
        bl_response(BL_OP_INQUIRY,version,sizeof(version));
        break;
    }
    case BL_INQUIRY_BLOCK_SIZE:
    {

        uint16_t size = BL_PACKET_PAYLOAD_SIZE;
        bl_response(BL_OP_INQUIRY,(uint8_t *)&size,sizeof(size));
        break;
    }
    default:
    {
        bl_response_ack(BL_OP_INQUIRY, BL_ERR_PARAM);
        break;
    }
    }
}

static void bl_op_boot_handler(uint8_t *data,uint16_t len)
{
    log_i("boot");
    bl_response_ack(BL_OP_BOOT,BL_ERR_OK);
    /*引导主程序*/
    application_boot();
}

static void bl_op_reset_handler(uint8_t *data,uint16_t len)
{
    log_i("reset");
    bl_response_ack(BL_OP_RESET,BL_ERR_OK);
    NVIC_SystemReset();
}

static void bl_op_erase_handler(uint8_t *data,uint16_t len)
{
    log_i("erase");
    bl_erase_param_t *erase = (void *)data;
    if(len != sizeof(bl_erase_param_t))                 /*检查参数是否合理*/
    {
        log_w("length dismatch %d !=%d",len,sizeof(bl_erase_param_t));
        bl_response_ack(BL_OP_ERASE,BL_ERR_PARAM);
        return;
    }
    if(erase->address >= FLASH_BOOT_ADDRESS && erase->address < FLASH_ARG_ADDRESS)  /*检查地址是否合理*/
    {
        log_w("addr = 0x%08x is not protected",erase->address);
        bl_response_ack(BL_OP_ERASE,BL_ERR_UNKNOWN);
        return;
    }
    log_i("addr=0x%08x,size=0x%08x",erase->address,erase->size);
    /*擦除之前先备份app*/
    bl_norflash_unlock();
    bool backup_success = bl_norflash_write_backup();
    bl_norflash_lock();
    if(backup_success)
    {
        /*备份成功*/
        log_i("backup successful");
        bl_norflash_unlock();
        bl_norflash_erase(erase->address,erase->size);
        bl_norflash_lock();
        bl_response_ack(BL_OP_ERASE,BL_ERR_OK);
    }
    else
    {
        /*备份不成功*/
        log_w("backup failed");
        bl_response_ack(BL_OP_ERASE,BL_ERR_UNKNOWN);
    }
}

static void bl_op_write_handler(uint8_t *data,uint16_t len)
{
    log_i("write");
    bl_write_param_t *write = (void *)data;
    if(len != sizeof(bl_write_param_t) + write->size)
    {
        log_w("length dismatch 0x%02x,0x%02x",len,sizeof(bl_write_param_t) + write->size);
        bl_response_ack(BL_OP_WRITE,BL_ERR_PARAM);
        return;
    }
    if (write->address >= FLASH_BOOT_ADDRESS &&
        write->address < FLASH_BOOT_ADDRESS + FLASH_BOOT_SIZE)
    {
        log_w("address 0x%08X is protected", write->address);
        bl_response_ack(BL_OP_WRITE, BL_ERR_UNKNOWN);
        return;
    }

    log_i("write in 0x%08x,size = 0x%02x",write->address,write->size);
    bl_norflash_unlock();

    bl_norflash_write(write->address,write->data,write->size);
    bl_norflash_lock();
    bl_response_ack(BL_OP_WRITE,BL_ERR_OK);
}


static void bl_op_verify_handler(uint8_t *data,uint16_t len)
{
    log_i("verify");
    bl_verify_param_t *verify = (void *)data;
    if(len != sizeof(bl_verify_param_t))
    {
        log_w("length mismatch %d != %d", len, sizeof(bl_verify_param_t));
        bl_response_ack(BL_OP_VERIFY, BL_ERR_PARAM);
        return;
    }
    log_i("verify 0x%08X, size %d", verify->address, verify->size);
    uint32_t crc;
    crc = crc32_update(0,(uint8_t *)verify->address,verify->size);
    log_i("crc is 0x%08x",verify->crc);
    if (crc == verify->crc)
    {
        bl_successful = true;
        bl_response_ack(BL_OP_VERIFY, BL_ERR_OK);

    }
    else
    {
        bl_response_ack(BL_OP_VERIFY, BL_ERR_VERIFY);
        bl_successful = false;
        if (backup_verify()) {
            log_w("Backup available. Entering recovery mode.");
            // 设置恢复引导标志（这个标志会在bootloader主循环中处理）
            need_recovery_guide = true;
    }
}
}

static void bl_ptk_handle(bl_pkt_t *pkt)
{
    log_i("opcode = 0x%02x length = 0x%02x",pkt->opcode,pkt->length);
    switch (pkt->opcode)
    {
        case BL_OP_INQUIRY:             /*查询信息*/
            bl_op_inquiry_handler(pkt->param, pkt->length);
            break;
        case BL_OP_BOOT:                /*引导主程序*/
            bl_op_boot_handler(pkt->param, pkt->length);
            break;
        case BL_OP_RESET:               /*重启芯片*/
            bl_op_reset_handler(pkt->param, pkt->length);
            break;
        case BL_OP_ERASE:               /*擦除指定地址的内容*/
            bl_op_erase_handler(pkt->param, pkt->length);
            break;
        case BL_OP_WRITE:               /*向指定地址写*/
            bl_op_write_handler(pkt->param, pkt->length);
            break;
        case BL_OP_VERIFY:              /*校验CRC*/
            bl_op_verify_handler(pkt->param, pkt->length);
            break;
        default:
            break;
    }
}

static bool bl_pkt_verify(bl_pkt_t *pkt)
{
    const uint8_t head = 0xAA;

    uint32_t crc = 0;
    crc = crc32_update(crc, (uint8_t *)&head, 1);
    crc = crc32_update(crc, (uint8_t *)&pkt->opcode, 1);
    crc = crc32_update(crc, (uint8_t *)&pkt->length, 2);
    crc = crc32_update(crc, pkt->param, pkt->length);

    return crc == pkt->crc;
}

static bool bl_recv_handler(bl_ctrl_t *ctrl, uint8_t data)
{
    bool fullpkt = false;
    bl_rx_t *rx = &ctrl->rx;
    bl_pkt_t *pkt = &ctrl->pkt;
    rx->data[rx->index++] = data;
    switch (ctrl->sm)
    {
    case BL_SM_IDLE:    //状态码为idle
    {
        log_d("sm idle");
        rx->index = 0;
        if (rx->data[0] == 0xAA)
        {
            ctrl->sm = BL_SM_START;
        }
        break;
    }
    case BL_SM_START:   //状态码为start
    {
        log_d("sm_start");
        rx->index = 0;
        pkt->opcode = (bl_op_t)rx->data[0];
        ctrl->sm = BL_SM_OPCODE;
        break;
    }
    case BL_SM_OPCODE:   //状态码为opcode
    {
        log_d("sm_opcode");
        if(rx->index == 2)
        {
            rx->index = 0;
            uint16_t length = *(uint16_t *)rx->data;
            if(length <= BL_PACKET_PARAM_SIZE)
            {
                pkt->length = length;
                if(pkt->length == 0)
                {
                    ctrl->sm = BL_SM_CRC;
                }
                else ctrl->sm = BL_SM_PARAM;
            }
            else 
            {
                /* 数据超了 做些处理*/
                bl_response_ack(pkt->opcode, BL_ERR_OVERFLOW);
                bl_reset(ctrl);
            }
            
        }
        break;
    }
    case BL_SM_PARAM:
    {
        rx->index = 0;
        if(pkt->index < pkt->length)
        {
            pkt->param[pkt->index++] = rx->data[0];
            if(pkt->index == pkt->length)
            {
            ctrl->sm = BL_SM_CRC;
            }       
        }
        else
        {
            bl_response_ack(pkt->opcode, BL_ERR_OVERFLOW);
            bl_reset(ctrl); 
        }
        break;
    }
    case BL_SM_CRC:
    {
        log_d("sm_CRC");
        if(rx->index == 4)
        {
            rx->index = 0;
            pkt->crc = *(uint32_t *)rx->data;
            if(bl_pkt_verify(pkt))  //CRC校验
            {
                fullpkt = true;
            }
            else 
            {
                log_w("crc dismatch");
                /* crc 校验失败 做些处理*/
                bl_response_ack(pkt->opcode, BL_ERR_VERIFY);
                bl_reset(ctrl);
            }
        }
        break;
    }
    }
    return fullpkt;
}
uint32_t time;
void bootloder(uint32_t boot_delay)
{   
    bool main_trap = false;
    uint32_t main_enter_time = 0;
    serial_rb = rb8_new(uart_buff,uart_buffer_size);
    bl_uart_recv_register(serial_resc_handle);
    main_enter_time = bl_now();
    bl_successful = false;

    bool backup_valid = backup_verify();    /*检查备份是否有效*/

    bool app_vaild = verify_application(); /*检查app是否有效*/

    if((!app_vaild && backup_valid))
    {
        log_w("Main application invalid, but backup available");
        need_recovery_guide = true;
        led_on();
        time = bl_now();
    }
    while(1)
    {
        if(need_recovery_guide)     /*恢复引导模式*/
        {
            if(bl_now() - time > 3000)
            {
                log_i("Bootstrap mode Please press the three-second button");
                time = bl_now();
            }
             if (bl_button_pressed()) 
        {
                uint32_t press_start = bl_now();
                while (bl_button_pressed()) 
            {
                    // 等待保持按下
                    if (bl_now() - press_start >= 3000) 
                    {
                        log_i("button pressed over 3 sceonds");
                        if(app_restroed())
                        {
                            log_i("restored success reset");
                            bl_button_wait_release();
                            NVIC_SystemReset();
                        }
                        else
                        {
                            log_w("restored failed");
                        }
                    }
            }
        }
        }
        else if(boot_delay > 0 && !main_trap && app_vaild)
        {
            static uint32_t last_passed_time = 0;
            uint32_t passed_time = bl_now() - main_enter_time;
            if(last_passed_time == 0)
            {
                log_i("boot in %d seconds", boot_delay);
                last_passed_time = 1; //标记已初始化
                passed_time = 1;    //记录时间
            }
            else if(passed_time / 1000 != last_passed_time / 1000)
            {
                log_i("boot in %d seconds", boot_delay - passed_time /1000);
            }

            if(passed_time > boot_delay * 1000)
            {
                /*引导主程序*/
                application_boot();
            }
            last_passed_time = passed_time;
        }

        if(bl_button_pressed())
        {
            bl_delay_ms(10);
            if(bl_button_pressed())
            {
                while(bl_button_pressed())
                {
                    bl_delay_ms(40);
                }
                log_w("button pressed reset1");
                NVIC_SystemReset();
            }
        }
        if(rb8_empty(serial_rb))
        {
            /*没有接受到数据的情况*/
            if(bl_ctrl.rx.index == 0)
            {
                last_ptktime = bl_now();
            }
            else
            {
                if(bl_now() - last_ptktime > BL_TIMEOUT_MS)
                {
                    log_w("time out");
                    bl_reset(&bl_ctrl);
                }
            }
            continue;          //ringbuffer为空则不进行下面的数据处理
        }

        uint8_t data;
        rb8_get(serial_rb,&data);
        //log_i("resc = %02x",data);  
        if(bl_recv_handler(&bl_ctrl,data))
        {
            bl_ptk_handle(&bl_ctrl.pkt);
            bl_reset(&bl_ctrl);
            main_trap = true;    //受到有效数据包才置位，否则引导主程序
            last_ptktime = bl_now();
        }
    }
}

static void bl_lowlevel_deinit(void)
{
    elog_deinit();
    bl_uart_deinit();
    SysTick->CTRL = 0;
    // 停用所有中断
     __disable_irq();
}

bool verify_application(void)
{
    uint32_t size, crc;
    bool result = bl_arginfo_read(&size, &crc);
    CHECK_RETX(result, false);

    uint32_t address = FLASH_APP_ADDRESS;
    uint32_t ccrc = crc32_update(0, (uint8_t *)address, size);
    if (ccrc != crc)
    {
        log_w("crc mismatch: %08X != %08X", ccrc, crc);
        return false;
    }

    return true;
}

void application_boot(void)
{
    typedef void(*entey_t)(void);
    uint32_t addr = FLASH_APP_ADDRESS;
    uint32_t sp = *(volatile uint32_t*)addr;
    uint32_t pc = *(volatile uint32_t*)(addr + 4);
    (void) sp;
    entey_t enter = (entey_t)pc;
    log_i("booting application at 0x%08X", addr);
    bl_lowlevel_deinit();
    enter();
}


bool app_restroed(void)
{
    uint32_t app_addr = FLASH_APP_ADDRESS;
    /*擦除备份区域flash*/
     log_i("Erasing backup area (0x%08X - 0x%08X)", 
          FLASH_APP_ADDRESS, 
          FLASH_APP_ADDRESS + FLASH_APP_SIZE);
    bl_norflash_unlock();
    bl_norflash_erase(FLASH_APP_ADDRESS,FLASH_APP_SIZE);


    /*将app中flash写入备份flash中*/
    for(uint32_t offset = 0;offset <= FLASH_BACKUP_SIZE;offset += 4)
    {
        uint32_t backup_data = *(uint32_t *)(offset + FLASH_BACKUP_ADDRESS);
        if(FLASH_ProgramWord(FLASH_APP_ADDRESS + offset,backup_data) != FLASH_COMPLETE)
        {
            log_w("write data failed, addr: 0x%08x", FLASH_APP_ADDRESS + offset);
        }
    }
    bl_norflash_lock();
    /*验证备份是否成功*/
    log_i("Verifying backup...");

    // 1. 计算应用程序区域的 CRC
    uint32_t app_crc = crc32_update(0, (uint8_t *)FLASH_APP_ADDRESS, FLASH_APP_SIZE);
    log_d("App CRC: 0x%08X", app_crc);

    // 2. 计算备份区域的 CRC
    uint32_t backup_crc = crc32_update(0, (uint8_t *)FLASH_BACKUP_ADDRESS, FLASH_APP_SIZE);
    log_d("Backup CRC: 0x%08X", backup_crc);

    // 3. 比较两个 CRC 值
    if(app_crc == backup_crc) 
    {
    log_i("Backup verified successfully (CRC: 0x%08X)", app_crc);
    return true;
    } 
    else 
    {
    log_w("Backup verification failed! App CRC: 0x%08X, Backup CRC: 0x%08X", 
          app_crc, backup_crc);
    return false;
    }
}
