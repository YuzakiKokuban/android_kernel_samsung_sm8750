/**
 * @file stk6d2x_ver.h
 *
 * Copyright (c) 2020, Sensortek.
 * All rights reserved.
 *
 ******************************************************************************/

/*==============================================================================

	Change Log:

	EDIT HISTORY FOR FILE

	Nov 17 2022 STK - 1.0.0
	  - First Draft Version
	  - Basic Function
	  - Flicker 1Hz (1024 data bytes)
	Dec 9 2022 STK - 2.0.0
	  - Add IT Switch and Checking External Clock Exist or Not
	  - Check RID
	  - Add HW/SW Summation
	  - Add Check XFLAG
	  - Integrate Gain Ratio Function
	  - Change FIFO Mode to STA01_STA2_ALS0_ALS1_ALS2
  ============================================================================*/

#ifndef _STK6D2X_VER_H
#define _STK6D2X_VER_H

// 32-bit version number represented as major[31:16].minor[15:8].rev[7:0]
#define STK6D2X_MAJOR        2
#define STK6D2X_MINOR        0
#define STK6D2X_REV          0
#define VERSION_STK6D2X  ((STK6D2X_MAJOR<<16) | (STK6D2X_MINOR<<8) | STK6D2X_REV)
#define DRIVER_VERSION  "2.0.0"

#endif //_STK6D2X_VER_H