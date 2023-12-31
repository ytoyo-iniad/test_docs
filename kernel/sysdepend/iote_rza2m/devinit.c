/*
 *----------------------------------------------------------------------
 *    micro T-Kernel 3.00.06
 *
 *    Copyright (C) 2006-2022 by Ken Sakamura.
 *    This software is distributed under the T-License 2.2.
 *----------------------------------------------------------------------
 *
 *    Released by TRON Forum(http://www.tron.org) at 2022/10.
 *
 *----------------------------------------------------------------------
 */

#include <sys/machine.h>
#ifdef IOTE_RZA2M

/*
 *	devinit.c (RZ/A2M IoT-Engine)
 *	Device-Dependent Initialization
 */

#include <sys/sysdef.h>
#include <tm/tmonitor.h>
#include <tk/device.h>

#include "kernel.h"
#include "sysdepend.h"

/* ------------------------------------------------------------------------ */

/*
 * Initialization before micro T-Kernel starts
 */

EXPORT ER knl_init_device( void )
{
	return E_OK;
}

/* ------------------------------------------------------------------------ */
/*
 * Start processing after T-Kernel starts
 *	Called from the initial task contexts.
 */
EXPORT ER knl_start_device( void )
{
#if USE_SDEV_DRV
	ER	err;

	/* A/D Converter "adca" */
	#if DEVCNF_USE_ADC
		err = dev_init_adc(0);
		if(err < E_OK) return err;
	#endif

	/* I2C unit.3 "iicd" */
	#if DEVCNF_USE_IIC
		err = dev_init_i2c(3);
		if(err < E_OK) return err;
	#endif


	/* Serial ch.4 "sere" */
	#if DEVCNF_USE_SER
		err = dev_init_ser(4);
		if(err < E_OK) return err;
	#endif

#endif
	return E_OK;
}

#if USE_SHUTDOWN
/* ------------------------------------------------------------------------ */
/*
 * System finalization
 *	Called just before system shutdown.
 *	Execute finalization that must be done before system shutdown.
 */
EXPORT ER knl_finish_device( void )
{
	return E_OK;
}

#endif /* USE_SHUTDOWN */

#endif /* IOTE_RZA2M */
