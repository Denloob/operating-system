#include "io.h"

#define IDE_PRIMARY_IO    0x1F0
#define IDE_SECONDARY_IO  0x170
#define IDE_PRIMARY_CTRL  0x3F6
#define IDE_SECONDARY_CTRL 0x376

#define IDE_MASTER  0xA0
#define IDE_SLAVE   0xB0

#define ATA_IDENTIFY 0xEC


void ide_detect() 
{
    uint16_t channels[2] = {IDE_PRIMARY_IO, IDE_SECONDARY_IO};
    const char *channel_names[2] = {"Primary", "Secondary"};
    const char *drive_names[2] = {"Master", "Slave"};
    uint8_t drive_type;

    for (int channel = 0; channel < 2; channel++) 
    {
        for (int drive = 0; drive < 2; drive++)
        {
            uint16_t io_base = channels[channel];
            out_byte(io_base + 0x06, (drive == 0) ? IDE_MASTER : IDE_SLAVE);
            io_delay();

            out_byte(io_base + 0x07, ATA_IDENTIFY);
            io_delay();


            uint8_t status = in_byte(io_base + 0x07);
            if (status == 0) continue;

            while ((status & 0x80) != 0)
                status = in_byte(io_base + 0x07);

            if (in_byte(io_base + 0x04) == 0 && in_byte(io_base + 0x05) == 0)
            {
                uint16_t identify_data[256];
                for (int i = 0; i < 256; i++)
                {
                    identify_data[i] = in_word(io_base);
                }

                char model[41];
                for (int i = 0; i < 40; i += 2)
                {
                    model[i] = identify_data[27 + i/2] >> 8;
                    model[i + 1] = identify_data[27 + i/2] & 0xFF;
                }
                model[40] = '\0';

                printf("Drive detected on %s channel, %s drive: %s\n",
                        channel_names[channel], drive_names[drive], model);
            } else {
                printf("No IDE drive on %s channel, %s drive.\n",
                        channel_names[channel], drive_names[drive]);
            }
        }
    }
}

