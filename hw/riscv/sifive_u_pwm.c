/*
 * SiFive FU540 PWM
 *
 * Copyright (c) 2020 Western Digital
 *
 * Author:  Alistair Francis <alistair.francis@wdc.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "hw/irq.h"
#include "hw/misc/sifive_u_pwm.h"
#include "hw/qdev-properties.h"
#include "hw/registerfields.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/module.h"

#ifndef STM_SYSCFG_ERR_DEBUG
#define STM_SYSCFG_ERR_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_SYSCFG_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

REG32(CONFIG,                   0x00)
    FIELD(CONFIG, SCALE,        0, 4)
    FIELD(CONFIG, STICKY,       8, 1)
    FIELD(CONFIG, ZEROCMP,      9, 1)
    FIELD(CONFIG, DEGLITCH,     10, 1)
    FIELD(CONFIG, ENALWAYS,     12, 1)
    FIELD(CONFIG, ENONESHOT,    13, 1)
    FIELD(CONFIG, CMP0CENTER,   16, 1)
    FIELD(CONFIG, CMP1CENTER,   17, 1)
    FIELD(CONFIG, CMP2CENTER,   18, 1)
    FIELD(CONFIG, CMP3CENTER,   19, 1)
    FIELD(CONFIG, CMP0GANG,     24, 1)
    FIELD(CONFIG, CMP1GANG,     25, 1)
    FIELD(CONFIG, CMP2GANG,     26, 1)
    FIELD(CONFIG, CMP3GANG,     27, 1)
    FIELD(CONFIG, CMP0IP,       28, 1)
    FIELD(CONFIG, CMP1IP,       29, 1)
    FIELD(CONFIG, CMP2IP,       30, 1)
    FIELD(CONFIG, CMP3IP,       31, 1)
REG32(COUNT,                   0x08)
REG32(PWMS,                    0x10)
REG32(PWMCMP0,                 0x20)
REG32(PWMCMP1,                 0x24)
REG32(PWMCMP2,                 0x28)
REG32(PWMCMP3,                 0x2C)

static inline int64_t sifive_u_pwm_ns_to_ticks(SiFiveUPwmState *s, int64_t t)
{
    uint64_t scale = s->pwmcfg & R_CONFIG_SCALE_MASK;
    return muldiv64(t, s->freq_hz, 1000000000ULL) * (1 << scale);
}

static void sifive_u_pwm_set_alarms(SiFiveUPwmState *s)
{
    int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    int64_t now_ticks = sifive_u_pwm_ns_to_ticks(s, now);
    int64_t current_count = (now_ticks - s->tick_offset) & 0x7FFFFFFF;
    uint64_t offset;
    int i;

    for (i = 0; i < SIFIVE_U_PWM_CHANS; i++) {
        if (s->pwmcmp[i] == 0 || s->pwmcmp[i] == 0xFFFF) {
            qemu_irq_lower(s->irqs[i]);
            continue;
        }

        int64_t ticks = sifive_u_pwm_ns_to_ticks(s, s->pwmcmp[i]);

        DB_PRINT("ticks: 0x%lx, current_count: 0x%lx\n", ticks, current_count);

        qemu_irq_lower(s->irqs[i]);

        if (ticks > current_count) {
            offset = ticks - current_count;
        } else {
            offset = 0x7FFFFFFF - current_count + ticks;
        }

        DB_PRINT("Setting alarm to: 0x%lx, now: 0x%lx\n", now + offset, now);
        timer_mod(&s->timer[i], now + offset);
    }
}

static void sifive_u_pwm_interrupt(SiFiveUPwmState *s, int num)
{
    DB_PRINT("Interrupt %d\n", num);

    qemu_irq_raise(s->irqs[num]);

    if (s->pwmcfg & R_CONFIG_ZEROCMP_MASK) {
        s->pwmcmp[num] = 0;
    }

    if (s->pwmcfg & R_CONFIG_ENALWAYS_MASK) {
        sifive_u_pwm_set_alarms(s);
    }
}

static void sifive_u_pwm_interrupt_0(void *opaque)
{
    SiFiveUPwmState *s = opaque;

    sifive_u_pwm_interrupt(s, 0);
}

static void sifive_u_pwm_interrupt_1(void *opaque)
{
    SiFiveUPwmState *s = opaque;

    sifive_u_pwm_interrupt(s, 1);
}

static void sifive_u_pwm_interrupt_2(void *opaque)
{
    SiFiveUPwmState *s = opaque;

    sifive_u_pwm_interrupt(s, 2);
}

static void sifive_u_pwm_interrupt_3(void *opaque)
{
    SiFiveUPwmState *s = opaque;

    sifive_u_pwm_interrupt(s, 3);
}
static uint64_t sifive_u_pwm_read(void *opaque, hwaddr addr,
                                  unsigned int size)
{
    SiFiveUPwmState *s = opaque;

    DB_PRINT("0x%"HWADDR_PRIx"\n", addr);

    switch (addr) {
    case A_CONFIG:
        return s->pwmcfg;
    case A_COUNT:
        /*
         * Return the value in the counter with bit 31 always 0
         * This is allowed to wrap around so we don't need to check that.
         */
        return (sifive_u_pwm_ns_to_ticks(s,
                                    qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL))
                - s->tick_offset) & 0x7FFFFFFF;
    case A_PWMS:
        return s->pwms;
    case A_PWMCMP0:
        return s->pwmcmp[0];
    case A_PWMCMP1:
        return s->pwmcmp[1];
    case A_PWMCMP2:
        return s->pwmcmp[2];
    case A_PWMCMP3:
        return s->pwmcmp[3];
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
        return 0;
    }

    return 0;
}

static void sifive_u_pwm_write(void *opaque, hwaddr addr,
                               uint64_t val64, unsigned int size)
{
    SiFiveUPwmState *s = opaque;
    uint32_t value = val64;
    int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

    DB_PRINT("0x%x, 0x%"HWADDR_PRIx"\n", value, addr);

    switch (addr) {
    case A_CONFIG:
        if (value & (R_CONFIG_CMP0CENTER_MASK | R_CONFIG_CMP1CENTER_MASK |
                     R_CONFIG_CMP2CENTER_MASK | R_CONFIG_CMP3CENTER_MASK)) {
            qemu_log_mask(LOG_UNIMP, "%s: CMPxCENTER is not supported\n",
                          __func__);
        }

        if (value & (R_CONFIG_CMP0GANG_MASK | R_CONFIG_CMP1GANG_MASK |
                     R_CONFIG_CMP2GANG_MASK | R_CONFIG_CMP3GANG_MASK)) {
            qemu_log_mask(LOG_UNIMP, "%s: CMPxGANG is not supported\n",
                          __func__);
        }

        if (value & (R_CONFIG_CMP0IP_MASK | R_CONFIG_CMP1IP_MASK |
                     R_CONFIG_CMP2IP_MASK | R_CONFIG_CMP3IP_MASK)) {
            qemu_log_mask(LOG_UNIMP, "%s: CMPxIP is not supported\n",
                          __func__);
        }
        s->pwmcfg = value;
        break;
    case A_COUNT:
        /* The guest changed the counter, updated the offset value. */
        s->tick_offset = sifive_u_pwm_ns_to_ticks(s, now) - value;
        break;
    case A_PWMS:
        s->pwms = value;
        break;
    case A_PWMCMP0:
        s->pwmcmp[0] = value;
        // fprintf(stderr, "s->pwmcmp[0]: 0x%lx\n", s->pwmcmp[0]);
        break;
    case A_PWMCMP1:
        s->pwmcmp[1] = value;
        break;
    case A_PWMCMP2:
        s->pwmcmp[2] = value;
        break;
    case A_PWMCMP3:
        s->pwmcmp[3] = value;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
    }

    /* Update the alarms */
    sifive_u_pwm_set_alarms(s);
}

static void sifive_u_pwm_reset(DeviceState *dev)
{
    SiFiveUPwmState *s = SIFIVE_U_PWM(dev);
    int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

    s->pwmcfg = 0x00000000;
    s->pwms = 0x00000000;
    s->pwmcmp[0] = 0x00000000;
    s->pwmcmp[1] = 0x00000000;
    s->pwmcmp[2] = 0x00000000;
    s->pwmcmp[3] = 0x00000000;

    s->tick_offset = sifive_u_pwm_ns_to_ticks(s, now);
}

static const MemoryRegionOps sifive_u_pwm_ops = {
    .read = sifive_u_pwm_read,
    .write = sifive_u_pwm_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_sifive_u_pwm = {
    .name = TYPE_SIFIVE_U_PWM,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_TIMER_ARRAY(timer, SiFiveUPwmState, 4),
        VMSTATE_INT64(tick_offset, SiFiveUPwmState),
        VMSTATE_UINT32(pwmcfg, SiFiveUPwmState),
        VMSTATE_UINT32(pwms, SiFiveUPwmState),
        VMSTATE_UINT32_ARRAY(pwmcmp, SiFiveUPwmState, 4),
        VMSTATE_END_OF_LIST()
    }
};

static Property sifive_u_pwm_properties[] = {
    DEFINE_PROP_UINT64("clock-frequency", struct SiFiveUPwmState,
                       freq_hz, 3333000000ULL),
    DEFINE_PROP_END_OF_LIST(),
};

static void sifive_u_pwm_init(Object *obj)
{
    SiFiveUPwmState *s = SIFIVE_U_PWM(obj);
    int i;

    for (i = 0; i < SIFIVE_U_PWM_IRQS; i++) {
        sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irqs[i]);
    }

    memory_region_init_io(&s->mmio, obj, &sifive_u_pwm_ops, s,
                          TYPE_SIFIVE_U_PWM, 0x100);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void sifive_u_pwm_realize(DeviceState *dev, Error **errp)
{
    SiFiveUPwmState *s = SIFIVE_U_PWM(dev);

    timer_init_ns(&s->timer[0], QEMU_CLOCK_VIRTUAL,
                  sifive_u_pwm_interrupt_0, s);

    timer_init_ns(&s->timer[1], QEMU_CLOCK_VIRTUAL,
                  sifive_u_pwm_interrupt_1, s);

    timer_init_ns(&s->timer[2], QEMU_CLOCK_VIRTUAL,
                  sifive_u_pwm_interrupt_2, s);

    timer_init_ns(&s->timer[3], QEMU_CLOCK_VIRTUAL,
                  sifive_u_pwm_interrupt_3, s);
}

static void sifive_u_pwm_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = sifive_u_pwm_reset;
    device_class_set_props(dc, sifive_u_pwm_properties);
    dc->vmsd = &vmstate_sifive_u_pwm;
    dc->realize = sifive_u_pwm_realize;
}

static const TypeInfo sifive_u_pwm_info = {
    .name          = TYPE_SIFIVE_U_PWM,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(SiFiveUPwmState),
    .instance_init = sifive_u_pwm_init,
    .class_init    = sifive_u_pwm_class_init,
};

static void sifive_u_pwm_register_types(void)
{
    type_register_static(&sifive_u_pwm_info);
}

type_init(sifive_u_pwm_register_types)
