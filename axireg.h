#ifndef AXIREG_H
#define AXIREG_H

typedef unsigned char u8 ;
typedef unsigned short u16 ;
typedef unsigned long u32 ;
typedef short int16_t;

// для управления синхронизацией  смещение 0 (запись)
#pragma pack(push, 1)
typedef struct {
    u8  outconfig :2;
    u8  BERreset:1;
    u32 reserv :29;
} reg_control_t;
#pragma pack(pop)

// общее состояние смещение 0 (чтение)
#pragma pack(push, 1)
typedef struct {
    u16 PDinSec :14;
    u8 BERtriger :4;
    u32 resh :14;
} reg_state_t;
#pragma pack(pop)

//---------------------------------------
// состояние канала PD чтение смещение 4 8 12 16
#pragma pack(push, 1)
typedef struct  // 3 !!!
{
    int16_t rawADC :12;
    unsigned short reserv0 :1;
    unsigned short BRAMAddresWrite :11;
    unsigned short reserv :8;
} reg_state_chanal_pd_t;
#pragma pack(pop)

// для управления PD каналами запись ----
#pragma pack(push, 1)
typedef struct {
    unsigned short threshold :8;
    int16_t offset :8;
    unsigned short reservH :16;
} reg_control_chanal_pd_t;
#pragma pack(pop)

// --------------------------------

typedef union {
    reg_state_chanal_pd_t BitField;
    u32 Data32;
} state_chanal_pd_t;

typedef union {
    reg_control_chanal_pd_t BitField;
    u32 Data32;
} control_chanal_pd_t;

typedef union {
    reg_state_t BitField;
    u32 Data32;
} state_hw_t;

typedef union {
    reg_control_t BitField;
    u32 Data32;
} control_hw_t;

#endif // AXIREG_H
