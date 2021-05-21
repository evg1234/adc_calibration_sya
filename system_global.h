#ifndef PRPD_FRONTEND_GLOB_H
#define PRPD_FRONTEND_GLOB_H


//uio
#define UIO_CLASS_DIR                       "/sys/class/uio"
#define UIO_DEV_DIR                         "/dev"
#define UIO_DEVICE_NAME_1                   "name"
#define UIO_DEVICE_MAP_1                    "maps"
#define UIO_DEVICE_MAP_2                    "map0"
#define UIO_DEVICE_SIZE_3                   "size"
#define UIO_DEVICE_NAME_3                   "name"
#define UIO_DEVICE_ADDR_3                   "addr"
#define UIO_DEVICE_OFFSET_3                 "offset"

#define BITSLIP_SM_UIO_NAME_PATTERN         "one_33_ch_ip"
#define BRAM_UIO_NAME_PATTERN               "pdchannel"
#define REG2AXI_NAME_PATTERN                "axicontrol"

//err codes
#define ERR_NO_BITSLIP_UIO                  1
#define ERR_NO_BRAM_UIO                     2
#define ERR_NO_REG2AXI_UIO                  3
#define ERR_NO_SPI                          4
#define ERR_SPI_NOT_OPENED                  5
#define ERR_BERR                            6
#define ERR_NO_REG2AXI                      7

#define SPIDEV00                             "/dev/spidev0.0"
#define SPIDEV01                             "/dev/spidev0.1"
#define SPIDEV02                             "/dev/spidev0.2"
#define SPIDEV10                             "/dev/spidev1.0"
#define SPIDEV11                             "/dev/spidev1.1"
#define SPIDEV12                             "/dev/spidev1.2"

//bit_slip_fsm (8 bit)
#define control_in_Data 				0x100 	//data register for Inport control_in
#define pattern_in_Data 				0x104 	//data register for Inport pattern_in
#define status_Data 					0x108 	//data register for Outport status
#define FSM_BALANCE_BITSLIP_COMMAND 	0x01  	//0x01 | 0x02 | 0x04
#define FSM_BITSLIP_COMMAND     		0x04	//just bitslip
#define ADC_CALIBRATION_PATTERN         0xA3

//bit_slip_fsm (12 bit)
#define control_lower_in                0x100 	// 0x01 | 0x02 | 0x04
#define control_upper_in                0x104 	// 0x01 | 0x02 | 0x04
#define pattern_lower_in                0x108 	// six lower bits of the pattern (in hex)
#define pattern_upper_in                0x10C 	// six upper bits of the pattern (in hex)
#define status_lower_out                0x114   // lower status
#define status_upper_out                0x118   // upper status
#define FSM_COMMAND                     0x01    // 0x01 | 0x02 | 0x04


//reg2axi
#define BER_CHAN_A_OFFSET                   0
#define BER_CHAN_B_OFFSET                   4
#define BER_CHAN_C_OFFSET                   8
#define BER_CHAN_D_OFFSET                   12
#define THRSHOLD_A_OFFSET                   16
#define THRSHOLD_B_OFFSET                   20
#define THRSHOLD_C_OFFSET                   24
#define THRSHOLD_D_OFFSET                   28

//metric data
#define METRIC_SIZE                         32

//payload max size (in bytes)
#define PAYLOAD_MAX                         4096


# define CRITICAL_MESSAGE                   qCritical() << "Critical:" << __FILE__ << __LINE__ << __func__

#endif // PRPD_FRONTEND_GLOB_H
