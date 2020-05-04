/*
FPGA direct pulse to IGBT
This project is based on Intel GHRD program, which demonstrate how to use hps communicate with FPGA through light AXI Bridge.

Author: Cheng Chen
2020-02-24	add ethernet-UDP connection
2020-03 	36 IGBT channels
2020-04-25 	add hall effect sensor function
*/



#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"

#include "hps_linux.h"

#include "alt_generalpurpose_io.h"
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

// interprocess comm
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
// network stuff
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  /* IP address conversion stuff */
#include <netdb.h>

#include "Header/hps_0.h"
#include "Header/UDP_Header.h"
#include "Header/tca9548a_driver.h"
#include "Header/hallsens_als31313_driver.h"
#include "Header/avalon_i2c.h"
#include "Header/GPIO_Address_MSK.h"


void open_physical_memory_device() {
    // We need to access the system's physical memory so we can map it to user
    // space. We will use the /dev/mem file to do this. /dev/mem is a character
    // device file that is an image of the main memory of the computer. Byte
    // addresses in /dev/mem are interpreted as physical memory addresses.
    // Remember that you need to execute this program as ROOT in order to have
    // access to /dev/mem.

    fd_dev_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd_dev_mem  == -1) {
        printf("ERROR: could not open \"/dev/mem\".\n");
        printf("    errno = %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void close_physical_memory_device() {
    close(fd_dev_mem);
}

void mmap_hps_peripherals() {
    hps_gpio = mmap(NULL, hps_gpio_span, PROT_READ | PROT_WRITE, MAP_SHARED, fd_dev_mem, hps_gpio_ofst);
    if (hps_gpio == MAP_FAILED) {
        printf("Error: hps_gpio mmap() failed.\n");
        printf("    errno = %s\n", strerror(errno));
        close(fd_dev_mem);
        exit(EXIT_FAILURE);
    }
}

void munmap_hps_peripherals() {
    if (munmap(hps_gpio, hps_gpio_span) != 0) {
        printf("Error: hps_gpio munmap() failed\n");
        printf("    errno = %s\n", strerror(errno));
        close(fd_dev_mem);
        exit(EXIT_FAILURE);
    }

    hps_gpio = NULL;
}

void mmap_fpga_peripherals() {
    // IMPORTANT: If you try to only mmap the fpga leds, it is possible for the
    // operation to fail, and you will get "Invalid argument" as errno. The
    // mmap() manual page says that you can only map a file from an offset which
    // is a multiple of the system's page size.

    // In our specific case, our fpga leds are located at address 0xFF200000,
    // which is a multiple of the page size, however this is due to luck because
    // the fpga leds are the only peripheral connected to the h2f_lw_axi_master.
    // The typical page size in Linux is 0x1000 bytes.

    // So, generally speaking, you will have to mmap() the closest address which
    // is a multiple of your page size and access your peripheral by a specific
    // offset from the mapped address.
	h2f_lw_axi_master = mmap(NULL, h2f_lw_axi_master_span, PROT_READ | PROT_WRITE, MAP_SHARED, fd_dev_mem, h2f_lw_axi_master_ofst);
	    if (h2f_lw_axi_master == MAP_FAILED) {
	        printf("Error: h2f_lw_axi_master mmap() failed.\n");
	        printf("    errno = %s\n", strerror(errno));
	        close(fd_dev_mem);
	        exit(EXIT_FAILURE);
	    }

	// control start pins for IGBT pulse
	h2p_ctrl_out_addr = h2f_lw_axi_master + CTRL_OUT_BASE;

	int increment = 0x100;
	// pulse length control
	for (i = 0; i<32; i++){
		*(h2p_igbt_pulse_addr+i)	= h2f_lw_axi_master + PULSE_LENGTH_PIO_0_BASE + increment*i;
	}
	/*
	*h2p_igbt_pulse_addr	= h2f_lw_axi_master + PULSE_LENGTH_PIO_0_BASE;
	*(h2p_igbt_pulse_addr+1)= h2f_lw_axi_master + PULSE_LENGTH_PIO_1_BASE;
	h2p_igbt_pulse_addr3 = h2f_lw_axi_master + PULSE_LENGTH_PIO_2_BASE;
	h2p_igbt_pulse_addr4 = h2f_lw_axi_master + PULSE_LENGTH_PIO_3_BASE;
	h2p_igbt_pulse_addr5 = h2f_lw_axi_master + PULSE_LENGTH_PIO_4_BASE;
	h2p_igbt_pulse_addr6 = h2f_lw_axi_master + PULSE_LENGTH_PIO_5_BASE;
	h2p_igbt_pulse_addr7 = h2f_lw_axi_master + PULSE_LENGTH_PIO_6_BASE;
	h2p_igbt_pulse_addr8 = h2f_lw_axi_master + PULSE_LENGTH_PIO_7_BASE;
	h2p_igbt_pulse_addr9 = h2f_lw_axi_master + PULSE_LENGTH_PIO_8_BASE;
	h2p_igbt_pulse_addr10 = h2f_lw_axi_master + PULSE_LENGTH_PIO_9_BASE;
	h2p_igbt_pulse_addr11 = h2f_lw_axi_master + PULSE_LENGTH_PIO_10_BASE;
	h2p_igbt_pulse_addr12 = h2f_lw_axi_master + PULSE_LENGTH_PIO_11_BASE;
	h2p_igbt_pulse_addr13 = h2f_lw_axi_master + PULSE_LENGTH_PIO_12_BASE;
	h2p_igbt_pulse_addr14 = h2f_lw_axi_master + PULSE_LENGTH_PIO_13_BASE;
	h2p_igbt_pulse_addr15 = h2f_lw_axi_master + PULSE_LENGTH_PIO_14_BASE;
	h2p_igbt_pulse_addr16 = h2f_lw_axi_master + PULSE_LENGTH_PIO_15_BASE;
	h2p_igbt_pulse_addr17 = h2f_lw_axi_master + PULSE_LENGTH_PIO_16_BASE;
	h2p_igbt_pulse_addr18 = h2f_lw_axi_master + PULSE_LENGTH_PIO_17_BASE;

	h2p_igbt_pulse_addr35 = h2f_lw_axi_master + PULSE_LENGTH_PIO_35_BASE;
*/
	h2p_i2c_ext_addr				= h2f_lw_axi_master + I2C_EXT_BASE;
	h2p_i2c_int_addr				= h2f_lw_axi_master + I2C_INT_BASE;
	h2p_i2c_ext_1_addr				= h2f_lw_axi_master + I2C_EXT_1_BASE;

}

void munmap_fpga_peripherals() {


	if (munmap(h2f_lw_axi_master, h2f_lw_axi_master_span) != 0) {
        printf("Error: h2f_lw_axi_master munmap() failed\n");
        printf("    errno = %s\n", strerror(errno));
        close(fd_dev_mem);
        exit(EXIT_FAILURE);
    }

    h2f_lw_axi_master	= NULL;
    fpga_leds			= NULL;
    fpga_switches		= NULL;

}


void mmap_peripherals() {
    mmap_hps_peripherals();
    mmap_fpga_peripherals();
}

void munmap_peripherals() {
    munmap_hps_peripherals();
    munmap_fpga_peripherals();
}

void check_i2c_isr_stat (volatile unsigned long * i2c_addr, uint8_t en_mesg) {
	uint32_t isr_status;

	isr_status = alt_read_word( i2c_addr+ISR_OFST);
	if (isr_status & RX_OVER_MSK) {
		printf("\t[ERROR] Receive data FIFO has overrun condition, new data is lost\n");
		alt_write_word( (i2c_addr+ISR_OFST) , RX_OVER_MSK ); // clears receive overrun
	}
	else {
		if (en_mesg) printf("\t[NORMAL] No receive overrun\n");
	}
	if (isr_status & ARBLOST_DET_MSK) {
		printf("\t[ERROR] Core has lost bus arbitration\n");
		alt_write_word( (i2c_addr+ISR_OFST) , ARBLOST_DET_MSK ); // clears receive overrun
	}
	else {
		if (en_mesg) printf("\t[NORMAL] No arbitration lost\n");
	}
	if (isr_status & NACK_DET_MSK) {
		printf("\t[ERROR] NACK is received by the core\n");
		alt_write_word( (i2c_addr+ISR_OFST) , NACK_DET_MSK ); // clears receive overrun
	}
	else {
		if (en_mesg) printf("\t[NORMAL] ACK has been received\n");
	}
	if (isr_status & RX_READY_MSK) {
		printf("\t[WARNING] RX_DATA_FIFO level is equal or more than its threshold\n");
	}
	else {
		if (en_mesg) printf("\t[NORMAL] RX_DATA_FIFO level is less than its threshold\n");
	}
	if (isr_status & TX_READY_MSK) { // NOTE THAT IF THERE'S NO DELAY BETWEEN WRITING TFR_CMD REGISTER AND THIS check_i2c_isr_stat FUNCTION CALL, THE I2C MIGHT NOT FINISH ITS OPERATION YET, WHICH LIKELY WILL GENERATE WARNING HERE
		printf("\t[WARNING] TFR_CMD level is equal or more than its threshold\n");
	}
	else {
		if (en_mesg) printf("\t[NORMAL] TFR_CMD level is less than its threshold\n");
	}

	if (en_mesg) printf("\t --- \n");
}

uint32_t i2c_tfr_cmd_fifo_lvl (volatile unsigned long * i2c_addr) {
	return (   alt_read_word(i2c_addr+TFR_CMD_FIFO_LVL_OFST)  &   TFR_CMD_FIFO_LVL_MSK   );
}

uint32_t i2c_rxdata_fifo_lvl (volatile unsigned long * i2c_addr) {
	return (   alt_read_word(i2c_addr+RX_DATA_FIFO_LVL_OFST)  &   RX_DATA_FIFO_LVL_MSK   );
}

void i2c_rxdata_flush (volatile unsigned long * i2c_addr, uint8_t en_mesg) {
	uint32_t rx_lvl = i2c_rxdata_fifo_lvl (i2c_addr);

	if (en_mesg) {
		printf("i2c_rxdata_flush :: RX-level: %d\n", rx_lvl);
	}

	while (i2c_rxdata_fifo_lvl (i2c_addr) > 0) {
		uint8_t flush = alt_read_word(h2p_i2c_ext_addr+RX_DATA_OFST);
		if (en_mesg) {
			printf("i2c_rxdata_flush :: flushed data: 0x%x\n", flush);
		}
	}
}


void i2c_wait_transfer(volatile unsigned long * i2c_addr) {
	while (! ( (alt_read_word( i2c_addr+ISR_OFST) & TX_READY_MSK) == TX_READY_MSK) ); // wait until TX_READY signal is asserted
}

// write/read file #define TCAADDR 0x70
void tca_channel_select(uint8_t channel_addr, uint8_t en_mesg){

		uint8_t TCAADDR = 0x70;	// hardwired address of TCA9548A is 0x70

		alt_write_word( (h2p_i2c_ext_addr+ISR_OFST) , RX_OVER_MSK|ARBLOST_DET_MSK|NACK_DET_MSK ); // RESET THE I2C FROM PREVIOUS ERRORS
		check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
		i2c_rxdata_flush(h2p_i2c_ext_addr, en_mesg);

		alt_write_word( (h2p_i2c_ext_addr+CTRL_OFST), 1<<CORE_EN_SHFT); // enable i2c core

		alt_write_word( (h2p_i2c_ext_addr+SCL_LOW_OFST), 250); // set the SCL_LOW_OFST to 250 for 100 kHz with 50 MHz clock
		alt_write_word( (h2p_i2c_ext_addr+SCL_HIGH_OFST), 250); // set the SCL_HIGH_OFST to 250 for 100 kHz with 50 MHz clock
		alt_write_word( (h2p_i2c_ext_addr+SDA_HOLD_OFST), 1); // set the SDA_HOLD_OFST to 1 as the default (datasheet requires min 0 ns hold time)

		alt_write_word( (h2p_i2c_ext_addr+TFR_CMD_OFST) , (1<<STA_SHFT) | (0<<STO_SHFT) | (TCAADDR<<AD_SHFT) | (WR_I2C<<RW_D_SHFT) );	//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
		alt_write_word( (h2p_i2c_ext_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (1<<STO_SHFT) | (channel_addr & I2C_DATA_MSK) );	 			//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);

		for (i = 0; i <= 5; i = i +1) {
			if (channel_addr << i && en_mesg){
				printf("Channel %ld turned on \n", i);
			}
		}
}

void wr_hall_sens (uint8_t i2c_addr_relay, uint8_t reg_addr, uint32_t datain, uint8_t en_mesg){ // write command to the hall sensor

	uint8_t datain_31to24 = datain >> EEPROM_Address_bit31to24_OFST; //write bit 31:24
	uint8_t datain_23to16 = datain >> EEPROM_Address_bit23to16_OFST; //write bit 23:16
	uint8_t datain_15to8 = datain >> EEPROM_Address_bit15to8_OFST; //write bit 15:8
	uint8_t datain_7to0 = datain >> EEPROM_Address_bit7to0_OFST; //write bit 7:0

	alt_write_word( (h2p_i2c_ext_1_addr+ISR_OFST) , RX_OVER_MSK|ARBLOST_DET_MSK|NACK_DET_MSK ); // RESET THE I2C FROM PREVIOUS ERRORS
	check_i2c_isr_stat (h2p_i2c_ext_1_addr, en_mesg);
	i2c_rxdata_flush(h2p_i2c_ext_1_addr, en_mesg);

	alt_write_word( (h2p_i2c_ext_1_addr+CTRL_OFST), 1<<CORE_EN_SHFT); // enable i2c core

	alt_write_word( (h2p_i2c_ext_1_addr+SCL_LOW_OFST), 250); // set the SCL_LOW_OFST to 250 for 100 kHz with 50 MHz clock
	alt_write_word( (h2p_i2c_ext_1_addr+SCL_HIGH_OFST), 250); // set the SCL_HIGH_OFST to 250 for 100 kHz with 50 MHz clock
	alt_write_word( (h2p_i2c_ext_1_addr+SDA_HOLD_OFST), 1); // set the SDA_HOLD_OFST to 1 as the default (datasheet requires min 0 ns hold time)

	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (1<<STA_SHFT) | (0<<STO_SHFT) | (i2c_addr_relay<<AD_SHFT) | (WR_I2C<<RW_D_SHFT) );	//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (0<<STO_SHFT) | (reg_addr & I2C_DATA_MSK) ); 								//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);

	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (0<<STO_SHFT) | ((datain_31to24) & I2C_DATA_MSK) );							//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (0<<STO_SHFT) | ((datain_23to16) & I2C_DATA_MSK) );							//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (0<<STO_SHFT) | ((datain_15to8) & I2C_DATA_MSK) );							//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (1<<STO_SHFT) | ((datain_7to0) & I2C_DATA_MSK) );							//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);

	i2c_wait_transfer(h2p_i2c_ext_1_addr);
	usleep(10000); // this delay is important because i2c_wait_transfer apparently doesn't wait until transfer is complete. This can only be eliminated if interrupt is utilized.

	alt_write_word( (h2p_i2c_ext_1_addr+CTRL_OFST), 0<<CORE_EN_SHFT); // disable i2c core

}

uint32_t rd_hall_sens (uint8_t i2c_addr_relay, uint8_t reg_addr, uint8_t en_mesg) { // read command for the hall sensor
	//uint8_t i2c_addr_relay = 102 ;	// hardwired address of ALS31313 is 102
	uint32_t dataout = 0;

	alt_write_word( (h2p_i2c_ext_addr+ISR_OFST) , RX_OVER_MSK|ARBLOST_DET_MSK|NACK_DET_MSK ); // RESET THE I2C FROM PREVIOUS ERRORS
	check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	i2c_rxdata_flush(h2p_i2c_ext_addr, en_mesg);

	alt_write_word( (h2p_i2c_ext_addr+CTRL_OFST), 1<<CORE_EN_SHFT); // enable i2c core

	alt_write_word( (h2p_i2c_ext_addr+SCL_LOW_OFST), 250); // set the SCL_LOW_OFST to 250 for 100 kHz with 50 MHz clock
	alt_write_word( (h2p_i2c_ext_addr+SCL_HIGH_OFST), 250); // set the SCL_HIGH_OFST to 250 for 100 kHz with 50 MHz clock
	alt_write_word( (h2p_i2c_ext_addr+SDA_HOLD_OFST), 1); // set the SDA_HOLD_OFST to 1 as the default (datasheet requires min 0 ns hold time)

	alt_write_word( (h2p_i2c_ext_addr+TFR_CMD_OFST) , (1<<STA_SHFT) | (0<<STO_SHFT) | (i2c_addr_relay<<AD_SHFT) | (WR_I2C<<RW_D_SHFT) );	//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (0<<STO_SHFT) | (reg_addr & I2C_DATA_MSK) ); 								//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);

	alt_write_word( (h2p_i2c_ext_addr+TFR_CMD_OFST) , (1<<STA_SHFT) | (0<<STO_SHFT) | (i2c_addr_relay<<AD_SHFT) | (RD_I2C<<RW_D_SHFT) );	//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (0<<STO_SHFT) | ((0x00) & I2C_DATA_MSK) );							//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (0<<STO_SHFT) | ((0x00) & I2C_DATA_MSK) );							//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (0<<STO_SHFT) | ((0x00) & I2C_DATA_MSK) );							//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (1<<STO_SHFT) | ((0x00) & I2C_DATA_MSK) );							//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);

	i2c_wait_transfer(h2p_i2c_ext_addr);
	usleep(10000); // this delay is important because i2c_wait_transfer apparently doesn't wait until transfer is complete. This can only be eliminated if interrupt is utilized.

	// read 32-bit data inside the rx register. Mind that there's limit in fifo length stated in QSYS
	dataout = (dataout << 8) | (alt_read_word(h2p_i2c_ext_addr+RX_DATA_OFST) & RX_DATA_MSK ) ; // read bit 31:24
	dataout = (dataout << 8) | (alt_read_word(h2p_i2c_ext_addr+RX_DATA_OFST) & RX_DATA_MSK ) ; // read bit 23:16
	dataout = (dataout << 8) | (alt_read_word(h2p_i2c_ext_addr+RX_DATA_OFST) & RX_DATA_MSK ) ; // read bit 15:8
	dataout = (dataout << 8) | (alt_read_word(h2p_i2c_ext_addr+RX_DATA_OFST) & RX_DATA_MSK ) ; // read bit 7:0

	alt_write_word( (h2p_i2c_ext_addr+CTRL_OFST), 0<<CORE_EN_SHFT); // disable i2c core

	if (en_mesg) printf("dataout: %x\n", dataout);

	return dataout;

}
//------------------------------------
uint32_t rd_hall_sens_3 (uint8_t i2c_addr_relay, uint8_t reg_addr, uint8_t en_mesg) { // read command for the hall sensor
	//uint8_t i2c_addr_relay = 102 ;	// hardwired address of ALS31313 is 102
	uint32_t dataout = 0;

	alt_write_word( (h2p_i2c_ext_1_addr+ISR_OFST) , RX_OVER_MSK|ARBLOST_DET_MSK|NACK_DET_MSK ); // RESET THE I2C FROM PREVIOUS ERRORS
	check_i2c_isr_stat (h2p_i2c_ext_1_addr, en_mesg);
	i2c_rxdata_flush(h2p_i2c_ext_1_addr, en_mesg);

	alt_write_word( (h2p_i2c_ext_1_addr+CTRL_OFST), 1<<CORE_EN_SHFT); // enable i2c core

	alt_write_word( (h2p_i2c_ext_1_addr+SCL_LOW_OFST), 250); // set the SCL_LOW_OFST to 250 for 100 kHz with 50 MHz clock
	alt_write_word( (h2p_i2c_ext_1_addr+SCL_HIGH_OFST), 250); // set the SCL_HIGH_OFST to 250 for 100 kHz with 50 MHz clock
	alt_write_word( (h2p_i2c_ext_1_addr+SDA_HOLD_OFST), 1); // set the SDA_HOLD_OFST to 1 as the default (datasheet requires min 0 ns hold time)

	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (1<<STA_SHFT) | (0<<STO_SHFT) | (i2c_addr_relay<<AD_SHFT) | (WR_I2C<<RW_D_SHFT) );	//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (0<<STO_SHFT) | (reg_addr & I2C_DATA_MSK) ); 								//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);

	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (1<<STA_SHFT) | (0<<STO_SHFT) | (i2c_addr_relay<<AD_SHFT) | (RD_I2C<<RW_D_SHFT) );	//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (0<<STO_SHFT) | ((0x00) & I2C_DATA_MSK) );							//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (0<<STO_SHFT) | ((0x00) & I2C_DATA_MSK) );							//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (0<<STO_SHFT) | ((0x00) & I2C_DATA_MSK) );							//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);
	alt_write_word( (h2p_i2c_ext_1_addr+TFR_CMD_OFST) , (0<<STA_SHFT) | (1<<STO_SHFT) | ((0x00) & I2C_DATA_MSK) );							//check_i2c_isr_stat (h2p_i2c_ext_addr, en_mesg);

	i2c_wait_transfer(h2p_i2c_ext_1_addr);
	usleep(10000); // this delay is important because i2c_wait_transfer apparently doesn't wait until transfer is complete. This can only be eliminated if interrupt is utilized.

	// read 32-bit data inside the rx register. Mind that there's limit in fifo length stated in QSYS
	dataout = (dataout << 8) | (alt_read_word(h2p_i2c_ext_1_addr+RX_DATA_OFST) & RX_DATA_MSK ) ; // read bit 31:24
	dataout = (dataout << 8) | (alt_read_word(h2p_i2c_ext_1_addr+RX_DATA_OFST) & RX_DATA_MSK ) ; // read bit 23:16
	dataout = (dataout << 8) | (alt_read_word(h2p_i2c_ext_1_addr+RX_DATA_OFST) & RX_DATA_MSK ) ; // read bit 15:8
	dataout = (dataout << 8) | (alt_read_word(h2p_i2c_ext_1_addr+RX_DATA_OFST) & RX_DATA_MSK ) ; // read bit 7:0

	alt_write_word( (h2p_i2c_ext_1_addr+CTRL_OFST), 0<<CORE_EN_SHFT); // disable i2c core

	if (en_mesg) printf("dataout: %x\n", dataout);

	return dataout;

}

void wr_hall_sens_data (uint8_t i2c_addr_relay, uint8_t Slave_Address) {
	uint32_t Slave_Address_Input = Slave_Address << EEP02_Slave_Address_OFST; //Slave_Address 16-10, 7 bits
	uint32_t REG02 = (EEP02_ChannelX_Enable_BASEMASK << EEP02_ChannelX_Enable_OFST) | (EEP02_ChannelY_Enable_BASEMASK << EEP02_ChannelY_Enable_OFST) | (EEP02_ChannelZ_Enable_BASEMASK << EEP02_ChannelZ_Enable_OFST) | (EEP02_I2C_Threshold_BASEMASK << EEP02_I2C_Threshold_OFST); // build default REG02 address

	//uint32_t Slave_Address_Clear = EEPROM_Address_BASEMASK & ~(EEP02_Slave_Address_BASEMASK<<EEP02_Slave_Address_OFST); // clear slave address bit
	//REG02 = REG02 & Slave_Address_Clear;

	Slave_Address_Input = Slave_Address_Input | REG02; // set new slave address, maintain other bits untouched

	wr_hall_sens (i2c_addr_relay, Customer_Access_OFST, Customer_Access_Data, ENABLE_MESSAGE); // Turn on customer write access
	wr_hall_sens (i2c_addr_relay, EEP02_OFST, Slave_Address_Input, DISABLE_MESSAGE);  // Change slave address

	uint32_t REG02_new = rd_hall_sens_3 (i2c_addr_relay, EEP02_OFST, DISABLE_MESSAGE);
	printf("\tEEP02_Slave_Address now = %d\n", (REG02_new>>EEP02_Slave_Address_OFST) & EEP02_Slave_Address_BASEMASK);
}

void rd_hall_sens_stat (uint8_t i2c_addr_relay) {
	uint32_t REG02 = rd_hall_sens (i2c_addr_relay, EEP02_OFST, DISABLE_MESSAGE);
	uint32_t REG03 = rd_hall_sens (i2c_addr_relay, EEP03_OFST, DISABLE_MESSAGE);
	uint32_t REG27 = rd_hall_sens (i2c_addr_relay, Vola27_OFST, DISABLE_MESSAGE);
	uint32_t REG28 = rd_hall_sens (i2c_addr_relay, Vola28_OFST, DISABLE_MESSAGE);
	uint32_t REG29 = rd_hall_sens (i2c_addr_relay, Vola29_OFST, DISABLE_MESSAGE);

	printf("REG02 = 0x%x\n", REG02);
		printf("\tEEP02_BW_Select = %d\n", (REG02>>EEP02_BW_Select_OFST) & EEP02_BW_Select_BASEMASK);
		printf("\tEEP02_Hall_Mode = %d\n", (REG02>>EEP02_Hall_Mode_OFST) & EEP02_Hall_Mode_BASEMASK);
		printf("\tEEP02_I2C_CRC_Enable = %d\n", (REG02>>EEP02_I2C_CRC_Enable_OFST) & EEP02_I2C_CRC_Enable_BASEMASK);
		printf("\tEEP02_Disable_Slave_ADC = %d\n", (REG02>>EEP02_Disable_Slave_ADC_OFST) & EEP02_Disable_Slave_ADC_BASEMASK);
		printf("\tEEP02_Slave_Address = %d\n", (REG02>>EEP02_Slave_Address_OFST) & EEP02_Slave_Address_BASEMASK);
		printf("\tEEP02_I2C_Threshold = %d\n", (REG02>>EEP02_I2C_Threshold_OFST) & EEP02_I2C_Threshold_BASEMASK);
		printf("\tEEP02_ChannelZ_Enable = %d\n", (REG02>>EEP02_ChannelZ_Enable_OFST) & EEP02_ChannelZ_Enable_BASEMASK);
		printf("\tEEP02_ChannelY_Enable = %d\n", (REG02>>EEP02_ChannelY_Enable_OFST) & EEP02_ChannelY_Enable_BASEMASK);
		printf("\tEEP02_ChannelX_Enable = %d\n", (REG02>>EEP02_ChannelX_Enable_OFST) & EEP02_ChannelX_Enable_BASEMASK);
		printf("\tEEP02_INT_Latch_Enable = %d\n", (REG02>>EEP02_INT_Latch_Enable_OFST) & EEP02_INT_Latch_Enable_BASEMASK);
		printf("\tEEP02_Customer_EEPROM = %d\n", (REG02>>EEP02_Customer_EEPROM_OFST) & EEP02_Customer_EEPROM_BASEMASK);


	printf("REG03 = 0x%x\n", REG03);
		printf("\tEEP03_Signed_INT_Enable   = %d\n", (REG03>>EEP03_Signed_INT_Enable_OFST) 		& EEP03_Signed_INT_Enable_BASEMASK);
		printf("\tEEP03_INT_Mode            = %d\n", (REG03>>EEP03_INT_Mode_OFST			) 	& EEP03_INT_Mode_BASEMASK);
		printf("\tEEP03_INT_EEPROM_Status   = %d\n", (REG03>>EEP03_INT_EEPROM_Status_OFST) 		& EEP03_INT_EEPROM_Status_BASEMASK);
		printf("\tEEP03_INT_EEPROM_Enable   = %d\n", (REG03>>EEP03_INT_EEPROM_Enable_OFST) 		& EEP03_INT_EEPROM_Enable_BASEMASK);
		printf("\tEEP03_Z_INT_Enable        = %d\n", (REG03>>EEP03_Z_INT_Enable_OFST		) 	& EEP03_Z_INT_Enable_BASEMASK);
		printf("\tEEP03_Y_INT_Enable        = %d\n", (REG03>>EEP03_Y_INT_Enable_OFST		) 	& EEP03_Y_INT_Enable_BASEMASK);
		printf("\tEEP03_X_INT_Enable        = %d\n", (REG03>>EEP03_X_INT_Enable_OFST		) 	& EEP03_X_INT_Enable_BASEMASK);
		printf("\tEEP03_Z_INT_Threshold     = %d\n", (REG03>>EEP03_Z_INT_Threshold_OFST	) 		& EEP03_Z_INT_Threshold_BASEMASK);
		printf("\tEEP03_Y_INT_Threshold     = %d\n", (REG03>>EEP03_Y_INT_Threshold_OFST	)	 	& EEP03_Y_INT_Threshold_BASEMASK);
		printf("\tEEP03_X_INT_Threshold     = %d\n", (REG03>>EEP03_X_INT_Threshold_OFST	) 		& EEP03_X_INT_Threshold_BASEMASK);

	printf("REG27 = 0x%x\n", REG27);
		printf("\tVola27_Low_Power_Mode_Count_Max  = %d\n", (REG27>>Vola27_Low_Power_Mode_Count_Max_OFST) 		& Vola27_Low_Power_Mode_Count_Max_BASEMASK);
		printf("\tVola27_I2C_Loop_Mode 			   = %d\n", (REG27>>Vola27_I2C_Loop_Mode_OFST			 ) 		& Vola27_I2C_Loop_Mode_BASEMASK 			);
		printf("\tVola27_Sleep 					   = %d\n", (REG27>>Vola27_Sleep_OFST 				 ) 		& Vola27_Sleep_BASEMASK 					);

	printf("REG28 = 0x%x\n", REG28);
		printf("\tVola28_X_Axis_MSBs      = %d\n", (REG28>>Vola28_X_Axis_MSBs_OFST 	) 		& Vola28_X_Axis_MSBs_BASEMASK 	);
		printf("\tVola28_Y_Axis_MSBs      = %d\n", (REG28>>Vola28_Y_Axis_MSBs_OFST 	) 		& Vola28_Y_Axis_MSBs_BASEMASK 	);
		printf("\tVola28_Z_Axis_MSBs      = %d\n", (REG28>>Vola28_Z_Axis_MSBs_OFST 	) 		& Vola28_Z_Axis_MSBs_BASEMASK 	);
		printf("\tVola28_New_Data         = %d\n", (REG28>>Vola28_New_Data_OFST		) 		& Vola28_New_Data_BASEMASK		);
		printf("\tVola28_Interrupt        = %d\n", (REG28>>Vola28_Interrupt_OFST		) 	& Vola28_Interrupt_BASEMASK 		);
		printf("\tVola28_Temperature_MSBs = %d\n", (REG28>>Vola28_Temperature_MSBs_OFST) 	& Vola28_Temperature_MSBs_BASEMASK);

	printf("REG29 = 0x%x\n", REG29);
		printf("\tVola29_Interrupt_Write    = %d\n", (REG29>>Vola29_Interrupt_Write_OFST	 	) 		& Vola29_Interrupt_Write_BASEMASK	 	);
		printf("\tVola29_X_Axis_LSBs        = %d\n", (REG29>>Vola29_X_Axis_LSBs_OFST		 	) 		& Vola29_X_Axis_LSBs_BASEMASK		 	);
		printf("\tVola29_Y_Axis_LSBs        = %d\n", (REG29>>Vola29_Y_Axis_LSBs_OFST		 	) 		& Vola29_Y_Axis_LSBs_BASEMASK		 	);
		printf("\tVola29_Z_Axis_LSBs        = %d\n", (REG29>>Vola29_Z_Axis_LSBs_OFST		 	) 		& Vola29_Z_Axis_LSBs_BASEMASK		 	);
		printf("\tVola29_Hall_Mode_Status   = %d\n", (REG29>>Vola29_Hall_Mode_Status_OFST 	) 		& Vola29_Hall_Mode_Status_BASEMASK 	);
		printf("\tVola29_Temperature_LSBs   = %d\n", (REG29>>Vola29_Temperature_LSBs_OFST 	) 		& Vola29_Temperature_LSBs_BASEMASK 	);

}

void rd_hall_sens_data (uint8_t i2c_addr_relay) {
	uint32_t REG28 = rd_hall_sens_3 (i2c_addr_relay, Vola28_OFST, DISABLE_MESSAGE);
	uint32_t REG29 = rd_hall_sens_3 (i2c_addr_relay, Vola29_OFST, DISABLE_MESSAGE);

	int16_t XDigit, YDigit, ZDigit, TempDigit;
	double XGauss, YGauss, ZGauss, Temp;
	XDigit 		= (((REG28>>Vola28_X_Axis_MSBs_OFST) & Vola28_X_Axis_MSBs_BASEMASK)<<4) 			| ((REG29>>Vola29_X_Axis_LSBs_OFST) & Vola29_X_Axis_LSBs_BASEMASK); // combine MSB and LSB
	YDigit 		= (((REG28>>Vola28_Y_Axis_MSBs_OFST) & Vola28_Y_Axis_MSBs_BASEMASK)<<4)		 		| ((REG29>>Vola29_Y_Axis_LSBs_OFST) & Vola29_Y_Axis_LSBs_BASEMASK); // combine MSB and LSB
	ZDigit 		= (((REG28>>Vola28_Z_Axis_MSBs_OFST) & Vola28_Z_Axis_MSBs_BASEMASK)<<4) 			| ((REG29>>Vola29_Z_Axis_LSBs_OFST) & Vola29_Z_Axis_LSBs_BASEMASK); // combine MSB and LSB
	TempDigit 	= (((REG28>>Vola28_Temperature_MSBs_OFST) & Vola28_Temperature_MSBs_BASEMASK)<<6) 	| ((REG29>>Vola29_Temperature_LSBs_OFST) & Vola29_Temperature_LSBs_BASEMASK); // combine MSB and LSB

	// The integer container is 16-bit signed, so we need to shift by 4 because the data is 12-bit signed value. We need to maintain the sign at MSB.
	XDigit = XDigit << 4;
	YDigit = YDigit << 4;
	ZDigit = ZDigit << 4;
	TempDigit = TempDigit << 4;

	XGauss = ((double) XDigit/16) / 1; // divide by 1 and .25 is coming from datasheet. Divide by 16 is coming from shift by 4 factor above.
	YGauss = ((double) YDigit/16) / 1;
	ZGauss = ((double) ZDigit/16) / .25;
	Temp = 273.15 + (((double) TempDigit/16) / 8); // divide by 8 is coming from datasheet. Divide by 16 is coming from shift by 4 factor above.

	printf("sensor address %d \n ", i2c_addr_relay);
	printf("X = %5.2f Gauss\n", XGauss);
	printf("Y = %5.2f Gauss\n", YGauss);
	printf("Z = %5.2f Gauss\n", ZGauss);
	printf("Temp = %5.2f %%C\n", Temp);
	printf("\n");

	if (UDP_TRANSMISSION_STATUS){
		sprintf(udp_buffer,"\t, %d \t , %5.2f \t , %5.2f \t , %5.2f \t , %5.2f \n",i2c_addr_relay,XGauss,YGauss,ZGauss,Temp);
		n=sendto(udp_sock, udp_buffer, strlen(udp_buffer),0,(const struct sockaddr *)&udp_server, udp_length);
		if (n < 0) printf("Sendto\n\r");
	}
	else
		fprintf(fptr, "\t, %d \t , %5.2f \t , %5.2f \t , %5.2f \t , %5.2f \n",i2c_addr_relay,XGauss,YGauss,ZGauss,Temp);
}

void write_Hall_reading_to_txt(uint32_t sensor_address){
	//Set time
	time_t t = time(NULL);
	struct tm *local = localtime(&t);

	// set up path and txt file name
	sprintf(pathname,"%04d_%02d_%02d_%02d_%02d_%02d_%s", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec,"HallReading.csv");
	// print to txt
	//pathname=strcat(pathname,".csv");

	fptr = fopen(pathname, "a+");
	fprintf(fptr, "Time \t\t Sensor \t X \t Y \t Z \t Temp \n");
	fprintf(fptr, "%04d_%02d_%02d, %02d_%02d_%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);

	for(i = 0 ; i <= 4; i++){
		fprintf(fptr, "channel_%d \n", sensor_address);
		rd_hall_sens_data(sensor_address);
		usleep(100000);
	}
	fclose(fptr);
}

void IGBT_Single_Pulse(long unsigned pulse_spacing_us, unsigned int number_of_iteration, unsigned int pulse_length[3]){

	int iterate = 1;

	for (iterate=1; iterate<=number_of_iteration; iterate++) {
		usleep(pulse_spacing_us);

		// give pulse length value
		// clock 50 MHz - > 20 ns, minimum time slot 1 us.
		// channel 0-8 --> 9 magnets


		for (i = 0; i<32; i++){
			alt_write_word( *(h2p_igbt_pulse_addr+i) ,  pulse_length[i] );

		}
/*
		alt_write_word( (h2p_igbt_pulse_addr) ,  pulse_length[0] );
		alt_write_word( (h2p_igbt_pulse_addr2) , pulse_length[1] );
		alt_write_word( (h2p_igbt_pulse_addr3) , pulse_length[2] );
		alt_write_word( (h2p_igbt_pulse_addr4) , pulse_length[3] );
		alt_write_word( (h2p_igbt_pulse_addr5) ,  pulse_length[4] );
		alt_write_word( (h2p_igbt_pulse_addr6) , pulse_length[5] );
		alt_write_word( (h2p_igbt_pulse_addr7) , pulse_length[6] );
		alt_write_word( (h2p_igbt_pulse_addr8) , pulse_length[7] );
		alt_write_word( (h2p_igbt_pulse_addr9) ,  pulse_length[8] );
		alt_write_word( (h2p_igbt_pulse_addr10) , pulse_length[9] );
		alt_write_word( (h2p_igbt_pulse_addr11) , pulse_length[10] );
		alt_write_word( (h2p_igbt_pulse_addr12) , pulse_length[11] );
		alt_write_word( (h2p_igbt_pulse_addr13) ,  pulse_length[12] );
		alt_write_word( (h2p_igbt_pulse_addr14) , pulse_length[13] );
		alt_write_word( (h2p_igbt_pulse_addr15) , pulse_length[14] );
		alt_write_word( (h2p_igbt_pulse_addr16) , pulse_length[15] );
		alt_write_word( (h2p_igbt_pulse_addr17) , pulse_length[16] );
		alt_write_word( (h2p_igbt_pulse_addr18) , pulse_length[17] );
*/
		// reset
		alt_write_word( (h2p_ctrl_out_addr) , ctrl_out | (0x01<<IGBT_CNT_RESET_ofst) );
		alt_write_word( (h2p_ctrl_out_addr) , ctrl_out & ~(0x01<<IGBT_CNT_RESET_ofst) );

		// usleep(10);

		// start state machine
		// start the state machine to capture data
		alt_write_word( (h2p_ctrl_out_addr) , ctrl_out | (0x01<<FSM_START_ofst) );
		alt_write_word( (h2p_ctrl_out_addr) , ctrl_out & ~(0x01<<FSM_START_ofst) );

		//printf("Pulse Complete! \n");
	}

}

void create_measurement_folder(char * foldertype) {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char command[100];
	int check;
	sprintf(foldername,"%04d_%02d_%02d_%02d_%02d_%02d_%s",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,foldertype);
	sprintf(command,"mkdir %s",foldername);
	check = mkdir(foldername, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	//cannot use system(). cannot figure out why
	//system(command);

	// copy the executable file to the folder
	// sprintf(command,"cp ./thesis_nmr_de1soc_hdl2.0 %s/execfile",foldername);
	// system(command);
}

void UDP_TRANSMISSION_ON(void){
	// open socket and associate with remote IP address
	udp_sock= socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sock < 0)
		printf("socket\n\r");
	udp_server.sin_family = AF_INET;

	// associate remote IP address
	udp_hp = gethostbyname("192.168.1.128"); // replace with actual
	if (udp_hp==0)
		printf("Unknown host\n\r");
	bcopy((char *)udp_hp->h_addr, (char *)&udp_server.sin_addr, udp_hp->h_length);

	// set IP port number
	udp_server.sin_port = htons(9090);
	udp_length=sizeof(struct sockaddr_in);

	//sprintf(udp_buffer,"%d\n\r",UDP_START);
	//n=sendto(udp_sock, udp_buffer, strlen(udp_buffer),0,(const struct sockaddr *)&udp_server, udp_length);

	n=sendto(udp_sock, UDP_START, strlen(UDP_START),0,(const struct sockaddr *)&udp_server, udp_length);

	if (n < 0) printf("Sendto\n\r");
}

void UDP_TRANSMISSION_OFF(void){
	// send end message
	//sprintf(udp_buffer,"%d\n\r",UDP_END);
	n=sendto(udp_sock, UDP_END, strlen(UDP_END),0,(const struct sockaddr *)&udp_server, udp_length);
	if (n < 0) printf("Sendto\n\r");
}

void UDP_send_frame(int* Asum, int size, int numFrame){
	// send large data in frames through udp
	char udp_tmp[size*4/numFrame];

	for(j = 0; j < numFrame; j++){
		for (i = 0; i < size/numFrame; i++){
				udp_tmp[3+4*i] = Asum[i + size/numFrame*j] & 0xFF;
				udp_tmp[2+4*i] = (Asum[i + size/numFrame*j]>>8) & 0xFF;
				udp_tmp[1+4*i] = (Asum[i + size/numFrame*j]>>16) & 0xFF;
				udp_tmp[4*i] = (Asum[i + size/numFrame*j]>>24) & 0xFF;
		}
		n = sendto(udp_sock, udp_tmp, size/numFrame*4, 0, (struct sockaddr*)&udp_server, udp_length);
	}
}

void time_test_function(){

	int size = 180000;
	int numFrame = 1000;
	int Asum[size];
	for (i = 0; i < size; i++){
		Asum[i] = round((i-size)/1000);
	}

	clock_t start, end;
	double cpu_time_used;
	start = clock();

    UDP_send_frame(Asum, size, numFrame);

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("%d", cpu_time_used);
    UDP_TRANSMISSION_OFF();
}

/*

// IGBT main function
int main(int argc, char * argv[]) {

	int igbt_pulse_num = 18;
	unsigned int pulse_length[] = {atoi(argv[1])*50, atoi(argv[2])*50, atoi(argv[3])*50, atoi(argv[4])*50,
			atoi(argv[5])*50, atoi(argv[6])*50, atoi(argv[7])*50, atoi(argv[8])*50,
			atoi(argv[9])*50, atoi(argv[10])*50, atoi(argv[11])*50, atoi(argv[12])*50,
			atoi(argv[13])*50, atoi(argv[14])*50, atoi(argv[15])*50, atoi(argv[16])*50,
			atoi(argv[17])*50, atoi(argv[18])*50
	}; // Quartus minimum resolution 20ns, convert into us
	long unsigned pulse_spacing_us  = atoi(argv[19]);
	unsigned int number_of_iteration = atoi(argv[20]);

	open_physical_memory_device();
    mmap_peripherals();

/*  Control from C to FPGA
 * void *virtual_base;
	int fd;
	int loop_count;
	int led_direction;
	int led_mask;
	void *h2p_lw_led_addr;

	virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd_dev_mem, HW_REGS_BASE );
	if( virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap() failed...\n" );
		close( fd_dev_mem );
		return( 1 );
	}
	//h2p_lw_led_addr=virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + LED_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) );

	// Set address output to zero
	*(uint32_t *)h2p_lw_led_addr = ~led_mask & IGBT_channel_all_off;

	led_mask = (0x01 << channel[0]) | (0x01 << channel[1]);
	*(uint32_t *)h2p_lw_led_addr = led_mask;9090

	usleep(10);
	*(uint32_t *)h2p_lw_led_addr = ~led_mask & IGBT_channel_all_off;
	//printf("PulseTime = 50 \n");



    //create_measurement_folder("CPMG");

    // UDP test
    UDP_TRANSMISSION_STATUS = UDP_TRANSMISSION_DISABLE;
    if (UDP_TRANSMISSION_DISABLE){
		UDP_TRANSMISSION_ON();
		time_test_function();
		UDP_TRANSMISSION_OFF();
    }

	if (HUB_IN_USE){
		tca_channel_select(HUB_channel_all_on, ENABLE_MESSAGE); 	// tca_channel_select(uint8_t channel_addr, uint8_t en_mesg); channel_addr 0-5
																	// HUB_channel_all_on/ HUB_channel_{X}_on, X = 0-5
		// tca_channel_check
		printf("Hall Effect Sensor connects to HUB\n");
		//printf("ojbk");
	}
	else {
		printf("Hall Effect Sensor directly connects to FPGA\n");
	}

	write_Hall_reading_to_txt(0);
	write_Hall_reading_to_txt(2);

    /-------------------Write Hall effect sensor----------------------/
   	// Hall effect sensor address can be programmed
    wr_hall_sens_data(0, 2); // write Hall effect sensor address

    // pulse length < pulse_spacing. pulse_spacing should be as wide as pulse length, i.e. 50% duty cycle
    for(i = 1; i< igbt_pulse_num-1; i++){
    	if ( pulse_length[i]/50 >= pulse_spacing_us  ){
	        printf("Error: duty cycle is smaller than pulse length! \n");
	        close(fd_dev_mem);
	        exit(EXIT_FAILURE);
    	}

    }

    IGBT_Single_Pulse(pulse_spacing_us, number_of_iteration, pulse_length);
    printf("Pulse Complete! \n");

    write_Hall_reading_to_txt(2);

    // clean up our memory mapping and exit
	//close_system();

    munmap_peripherals();
    close_physical_memory_device();

	return( 0 );
}
*/

// set hall effect sensor
int main(int argc, char * argv[]) {

	int igbt_pulse_num = 18;
	unsigned int sen_addr = atoi(argv[1]);

	open_physical_memory_device();
    mmap_peripherals();

	if (HUB_IN_USE){
		tca_channel_select(HUB_channel_all_on, ENABLE_MESSAGE); 	// tca_channel_select(uint8_t channel_addr, uint8_t en_mesg); channel_addr 0-5
																	// HUB_channel_all_on/ HUB_channel_{X}_on, X = 0-5
		// tca_channel_check
		printf("Hall Effect Sensor connects to HUB\n");
		//printf("ojbk");
	}
	else {
		printf("Hall Effect Sensor directly connects to FPGA\n");
	}

	write_Hall_reading_to_txt(0);
	write_Hall_reading_to_txt(sen_addr);

    //*****************Write Hall effect sensor********************//
   	// Hall effect sensor address can be programmed
    //wr_hall_sens_data(0, sen_addr); // write Hall effect sensor address

    //write_Hall_reading_to_txt(2);

    // clean up our memory mapping and exit
	//close_system();

    munmap_peripherals();
    close_physical_memory_device();

	return( 0 );
}
