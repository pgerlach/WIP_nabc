// Data sheet du CR14 :
// https://datasheet.octopart.com/CR14-MQP/1GE-STMicroelectronics-datasheet-10836722.pdf
// Protocol avec les tags :
// http://zxevo-files.perestoroniny.ru/datasheets/www.st.com/internet/com/TECHNICAL_RESOURCES/TECHNICAL_LITERATURE/DATASHEET/CD00236829.pdf

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define CRX14_PARAMETER_REGISTER 0x00
#define CRX14_IO_FRAME_REGISTER 0x01

int write_register(int file_i2c, int addr, int reg, const unsigned char* buffer, int len) {
    unsigned char buf[len + 1];
    memcpy(buf + 1, buffer, len);
    buf[0] = reg;
    while (1) {
        if (write(file_i2c, buf, len + 1) < len + 1) {
            if (errno == 121) {
                printf(".");
                continue;
            }
            printf("Failed to write register (ioctl). (errno=%i)\n", errno);
            return -1;
        } else {
            break;
        }
    }
    return 0;
}

int read_register(int file_i2c, int addr, int reg, unsigned char* buffer, int len) {
    struct i2c_msg m[2];
    struct i2c_rdwr_ioctl_data d;
    unsigned char rnum = reg;

    d.msgs = m;
    d.nmsgs = 2;
    m[0].addr = addr;
    m[0].len = sizeof(rnum);
    m[0].flags = 0;
    m[0].buf = &rnum;
    m[1].addr = addr;
    m[1].len = len;
    m[1].flags = I2C_M_RD;
    m[1].buf = buffer;

    while (1) {
        if (ioctl(file_i2c, I2C_RDWR, &d) < 0) {
            /* ERROR HANDLING: i2c transaction failed */
            if (errno == 121) {
                printf(".");
                continue;
            }
            printf("Failed to read register (ioctl). (errno=%i)\n", errno);
            return -1;
        } else {
            break;
        }
    }
    return 0;
}

int completion_rfid(int file_i2c, int addr) {
    unsigned char buf[2];
    buf[0] = 0x01;
    buf[1] = 0x0F;
    return write_register(file_i2c, addr, CRX14_IO_FRAME_REGISTER, buf, sizeof(buf));
}

int close_rfid(int file_i2c, int addr) {
    int r = completion_rfid(file_i2c, addr);
    if (r < 0) {
        return r;
    }
    unsigned char buf[1];
    buf[0] = 0x00;
    r = write_register(file_i2c, addr, CRX14_PARAMETER_REGISTER, buf, sizeof(buf));
    if (r < 0) {
        return r;
    }
    if (read(file_i2c, buf, sizeof(buf)) != sizeof(buf)) {
		printf("Failed to read (close_rfid)");
		return -1;
    }
    if (buf[0] != 0x00) {
		printf("Read %02X", buf[0]);
		return -1;
    }
    return 0;
}

int init_rfid(int file_i2c, int addr) {
    int r = close_rfid(file_i2c, addr);
    if (r < 0) {
        return r;
    }
    unsigned char buf[1];
    buf[0] = 0x10;
    r = write_register(file_i2c, addr, CRX14_PARAMETER_REGISTER, buf, sizeof(buf));
    if (r < 0) {
        return r;
    }
    if (read(file_i2c, buf, sizeof(buf)) != sizeof(buf)) {
		printf("Failed to read (close_rfid)");
		return -1;
    }
    if (buf[0] != 0x10) {
		printf("Read %02X", buf[0]);
		return -1;
    }
    return 0;
}

int main(int argc, char** argv) {
	int file_i2c;
	int length;
	unsigned char buffer[60] = {0};
	unsigned char ret[36];
	
	//----- OPEN THE I2C BUS -----
	char *filename = (char*)"/dev/i2c-1";
	if ((file_i2c = open(filename, O_RDWR)) < 0)
	{
		printf("Failed to open the i2c bus");
		return 1;
	}
	
	int addr = 0x50;

	if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave.\n");
		return 1;
	}

#define CARRIER_FREQ_RF_OUT_ON 0x10
#define WATCHDOG_TIMEOUT_5MS 0x20

//	buffer[0] = WATCHDOG_TIMEOUT_5MS | CARRIER_FREQ_RF_OUT_ON;
	buffer[0] = CARRIER_FREQ_RF_OUT_ON;
	// https://github.com/RedoXyde/nabgcc/blob/wpa2/src/hal/rfid.c#L59
	// Is 5ms timeout really required ?
	if (write_register(file_i2c, addr, CRX14_PARAMETER_REGISTER, buffer, 1) < 0) {
		printf("Failed to write register 0 <- 0x30.\n");
		return 1;
	}

    while (1) {
        if (init_rfid(file_i2c, addr) < 0) {
            printf("init_rfid failed\n");
            return 1;
        }
        
        // Initiate
        buffer[0] = 0x02;
        buffer[1] = 0x06;
        buffer[2] = 0x00;
        if (write_register(file_i2c, addr, CRX14_IO_FRAME_REGISTER, buffer, 3) < 0) {
            printf("Failed to write register 0x01 <- 0x02, 0x06, 0x00.\n");
            return 1;
        }
        
        usleep(1000);

        // https://github.com/RedoXyde/nabgcc/blob/wpa2/src/hal/rfid.c#L214
        // Pourquoi deux octets ??
        if (read_register(file_i2c, addr, CRX14_IO_FRAME_REGISTER, buffer, 2) < 0) {
            printf("Failed to read register 0x01 for 2 bytes.\n");
            return 1;
        }
        
        if (buffer[0] == 0) {
            printf("No tag, sleeping for 1 sec - %02X%02X\n", buffer[0], buffer[1]);
            sleep(1);
            continue;
        } else {
            unsigned char chip_id = buffer[0];
            
            buffer[0] = 0x02;
            buffer[1] = 0x0E;
            buffer[2] = chip_id;
            if (write_register(file_i2c, addr, CRX14_IO_FRAME_REGISTER, buffer, 3) < 0) {
                printf("Failed to write register 0x01 <- 0x02, 0x0E, chip_id.\n");
                return 1;
            }
            
            if (read_register(file_i2c, addr, CRX14_IO_FRAME_REGISTER, buffer, 1) < 0) {
                printf("Failed to read register 0x01 for 2 bytes.\n");
                return 1;
            }
            
            if (buffer[0] != chip_id) {
                printf("Mismatch chip ID after select !\n");
                continue;
            }
            
            buffer[0] = 0x01;
            buffer[1] = 0x0B;
            if (write_register(file_i2c, addr, CRX14_IO_FRAME_REGISTER, buffer, 2) < 0) {
                printf("Failed to write register 0x01 <- 0x01, 0x0B.\n");
                return 1;
            }
            
            if (read_register(file_i2c, addr, CRX14_IO_FRAME_REGISTER, buffer, 8) < 0) {
                printf("Failed to read register 0x01 for 8 bytes.\n");
                return 1;
            }
            
            printf("got tag: %02X%02X%02X%02X%02X%02X%02X%02X\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
            break;
        }
    }
	
	return 0;
}
