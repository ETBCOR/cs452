


#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx

#define HDC1080ADDRESS 0x40
#define HDC1080DEVICEIDREG 0xFE
#define I2C_PORT i2c1

int readDeviceID() {
	uint8_t deviceID[2];
    uint8_t val = HDC1080DEVICEIDREG;
    int ret;
    ret = i2c_write_blocking(I2C_PORT, HDC1080ADDRESS, &val, 1, false);
    //sleep_ms(9);
    printf("i2c_write_blocking return=%d\n", ret);


    ret = i2c_read_blocking(I2C_PORT, HDC1080ADDRESS, deviceID, 2, false);
    int returnValue = deviceID[0]<<8|deviceID[1];
    printf("registeraddress=0X%X\n", val);
    printf("dev[0]=0X%X, dev[1]=0X%X\n", deviceID[0], deviceID[1]);
	return returnValue;
}

//    #include <tusb.h>

int main() {
    // Enable UART so we can print status output
  stdio_init_all();
 // while (!tud_cdc_connected()) { sleep_ms(100);  }
  //  printf("HDC1080 connected()\n");
    
   // printf("Test Print\n"); 
    
    
    // This example will use I2C1 on the default SDA and SCL pins
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    //gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    //gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    sleep_ms(1000);
  while(1)
{
    int deviceID;
    deviceID = readDeviceID();
    printf("deviceID=0x%X\n",deviceID);
    sleep_ms(1000);
}

        return 0;

}
