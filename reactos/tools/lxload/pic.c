#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include "luserhw.h"

#define PIC_MODE_ICW2 1
#define PIC_MODE_ICW3 2
#define PIC_MODE_ICW4 3
#define PIC_MODE_GO 4

#define ICW1_ICW4 1
#define ICW1_NOTCASCADE 2
#define ICW1_4BYTEIVEC 4
#define ICW1_LEVELTRIG 8
#define ICW1_RESET 16

#define ICW4_80x86 1
#define ICW4_AUTO_EOI 2
#define ICW4_SFNM 16

#define OCW2_OCW3 8

#define OCW_EOI_NONSPECIFIC (1<<5)
#define OCW_EOI_NOP (2<<5)
#define OCW_EOI_SPECIFIC (3<<5)
#define OCW_EOI_ROTATE_AUTO (4<<5)
#define OCW_EOI_ROTATE_NONSPECIFC (5<<5)
#define OCW_EOI_SET_PRIORITY (6<<5)
#define OCW_EOI_ROTATE_SPECIFIC (7<<5)
#define OCW_EOI_MASK 0xe0

void sendInterrupt(char intNum);

struct pic_data {
    char mode;
    char ocw[3];
    char icw[4];
    char pending;
} 
    pic_data1 = { PIC_MODE_GO, { -1 } }, 
    pic_data2 = { PIC_MODE_GO, { -1 } };

void interruptFire(struct dev_t *dev, void *context)
{
    assert((int)context);
    sendInterrupt((int)context);
}

void evalPending(struct dev_t *dev, struct pic_data *picdata)
{
    int i, mask;
    struct timeval when = { 0,1 };

    for (i = 7; i >= 0; i--)
    {
        mask = 1 << i;
        if ((picdata->pending & mask) && !(picdata->ocw[0] & mask))
        {
            picdata->pending &= ~mask;
            scheduleInterrupt
                (dev, 
                 &when, 
                 interruptFire, 
                 (void *)(i + picdata->icw[1]));
        }
    }
}

void eoi(struct pic_data *picdata, int i)
{
    int mask;

    if (i == -1)
        for (i = 7; i >= 0; i--)
        {
            mask = 1 << i;
            if (picdata->ocw[0] & mask) break;
        }
    else
        mask = 1 << i;

    picdata->ocw[0] &= ~mask;
}

int picReadByte(struct dev_t *dev, int addr)
{
    return -1;
}

void picWriteByte(struct dev_t *dev, int addr, int val)
{
    struct pic_data *picdata = dev->self;
    switch (addr)
    {
    case 0:
        if (val & ICW1_RESET)
        {
            picdata->icw[0] = val;
            picdata->mode = PIC_MODE_ICW2;
        }
        else if (val & OCW2_OCW3)
            picdata->ocw[2] = val;
        else
        {
            picdata->ocw[1] = val;
            switch (picdata->ocw[1] & OCW_EOI_MASK)
            {
            case OCW_EOI_NONSPECIFIC:
                eoi(picdata, -1);
                break;

            case OCW_EOI_SPECIFIC:
                eoi(picdata, val & 7);
                break;

            default:
                fprintf(stderr, "Unsupported pic command %d\n",
                        picdata->ocw[1] >> 5);
                abort();
                break;
            }
        }
        break;

    case 1:
        switch (picdata->mode)
        {
        case PIC_MODE_GO:
            picdata->ocw[0] = val;
            break;

        case PIC_MODE_ICW2:
            picdata->icw[1] = val;
            picdata->mode = PIC_MODE_ICW3;
            break;

        case PIC_MODE_ICW3:
            picdata->icw[2] = val;
            picdata->mode = PIC_MODE_ICW4;
            break;

        case PIC_MODE_ICW4:
            picdata->icw[3] = val;
            picdata->mode = PIC_MODE_GO;
            break;
        }
        break;
    }

    evalPending(dev, picdata);
}

void raiseInterrupt(struct dev_t *dev, char intNum)
{
    struct timeval when = { 0,1 };

    if (intNum < 8)
    {
        if (!(pic_data1.ocw[0] & (1 << intNum)) && 
            !(pic_data1.pending & (1 << intNum)))
        {
            pic_data1.ocw[0] |= (1<<intNum);
            scheduleInterrupt
                (dev, 
                 &when, 
                 interruptFire, 
                 (void *)(intNum + pic_data1.icw[1]));
        }
        else
        {
            pic_data1.pending |= (1<<intNum);
        }
    }
    else
    {
        intNum -= 8;
        if (!(pic_data2.ocw[0] & (1 << intNum)) &&
            !(pic_data2.pending & (1 << intNum)))
        {
            pic_data2.ocw[0] |= (1<<intNum);
            scheduleInterrupt
                (dev, 
                 &when, 
                 interruptFire, 
                 (void *)(intNum + pic_data2.icw[1]));
        }
        else
        {
            pic_data2.pending |= (1<<intNum);
        }
    }
}
