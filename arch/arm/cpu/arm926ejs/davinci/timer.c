/*
 * (C) Copyright 2003
 * Texas Instruments <www.ti.com>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002-2004
 * Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
 *
 * (C) Copyright 2004
 * Philippe Robin, ARM Ltd. <philippe.robin@arm.com>
 *
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>

DECLARE_GLOBAL_DATA_PTR;

struct davinci_timer {
	u_int32_t	pid12;
	u_int32_t	emumgt;
	u_int32_t	na1;
	u_int32_t	na2;
	u_int32_t	tim12;
	u_int32_t	tim34;
	u_int32_t	prd12;
	u_int32_t	prd34;
	u_int32_t	tcr;
	u_int32_t	tgcr;
	u_int32_t	wdtcr;
};

static struct davinci_timer * const timer =
	(struct davinci_timer *)CONFIG_SYS_TIMERBASE;
static struct davinci_timer * const wdtimer =
	(struct davinci_timer *) DAVINCI_WDOG_BASE;

#define TIMER_LOAD_VAL	0xffffffff

#define TIM_CLK_DIV	16

#ifdef CONFIG_WATCHDOG
static void wd_timer_init(void)
{
	uint64_t timeo = ((uint64_t) CONFIG_SYS_HZ_CLOCK) *
				((uint64_t) CONFIG_SYS_WATCHDOG_VALUE);

	/* Disable, internal clock source */
	writel(0x0, &wdtimer->tcr);

	/* Reset timer, set mode to 64-bit watchdog, and unreset */
	writel(0x0, &wdtimer->tgcr);
	writel((2 << 2) | 3, &wdtimer->tgcr);

	/* Clear counter regs */
	writel(0x0, &wdtimer->tim12);
	writel(0x0, &wdtimer->tim34);

	/* Set timeout period */
	writel(timeo & 0xffffffff, &wdtimer->prd12);
	writel(timeo >> 32, &wdtimer->prd34);

	/* Enable run continuously */
	writel((2 << 6) | (2 << 22), &wdtimer->tcr);

	writel(0xa5c64000, &wdtimer->wdtcr);
	writel(0xda7e4000, &wdtimer->wdtcr);
}

void watchdog_reset(void)
{
	writel(0x0, &wdtimer->tim12);
	writel(0x0, &wdtimer->tim34);
	writel(0xa5c60000, &wdtimer->wdtcr);
	writel(0xda7e0000, &wdtimer->wdtcr);
}

#else
static void wd_timer_init(void) { /* nop */ }
#endif

int timer_init(void)
{
	/* We are using timer34 in unchained 32-bit mode, full speed */
	writel(0x0, &timer->tcr);
	writel(0x0, &timer->tgcr);
	writel(0x06 | ((TIM_CLK_DIV - 1) << 8), &timer->tgcr);
	writel(0x0, &timer->tim34);
	writel(TIMER_LOAD_VAL, &timer->prd34);
	writel(2 << 22, &timer->tcr);
	gd->timer_rate_hz = CONFIG_SYS_HZ_CLOCK / TIM_CLK_DIV;
	gd->timer_reset_value = 0;

	wd_timer_init();

	return(0);
}

void reset_timer(void)
{
	gd->timer_reset_value = get_ticks();
}

/*
 * Get the current 64 bit timer tick count
 */
unsigned long long get_ticks(void)
{
	unsigned long now = readl(&timer->tim34);

	/* increment tbu if tbl has rolled over */
	if (now < gd->tbl)
		gd->tbu++;
	gd->tbl = now;

	return (((unsigned long long)gd->tbu) << 32) | gd->tbl;
}

ulong get_timer(ulong base)
{
	unsigned long long timer_diff;

	timer_diff = get_ticks() - gd->timer_reset_value;

	return (timer_diff / (gd->timer_rate_hz / CONFIG_SYS_HZ)) - base;
}

void __udelay(unsigned long usec)
{
	unsigned long long endtime;

	endtime = ((unsigned long long)usec * gd->timer_rate_hz) / 1000000UL;
	endtime += get_ticks();

	while (get_ticks() < endtime)
		;
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ;
}
