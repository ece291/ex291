/* drvhdr.c -- VBE/AF driver header, used as input for DRVGEN.
   Based on FreeBE/AF stub drvhdr.c.

   $Id: drvhdr.c,v 1.2 2001/03/01 17:09:00 pete Exp $
*/

#include "vbeaf.h"

AF_DRIVER drvhdr =
{
   "VBEAF.DRV",                                             /* Signature */
   0x200,                                                   /* Version */
   0x100,                                                   /* DriverRev */
   "FreeBE/AF EX291 driver " FREEBE_VERSION,                /* OemVendorName */
   "Copyright (C) 2001 Peter Johnson",                      /* OemCopyright */
   NULL,                                                    /* AvailableModes */
   0,                                                       /* TotalMemory */
   0,                                                       /* Attributes */
   0,                                                       /* BankSize */
   0,                                                       /* BankedBasePtr */
   0,                                                       /* LinearSize */
   0,                                                       /* LinearBasePtr */
   0,                                                       /* LinearGranularity */
   NULL,                                                    /* IOPortsTable */
   { NULL, NULL, NULL, NULL },                              /* IOMemoryBase */
   { 0, 0, 0, 0 },                                          /* IOMemoryLen */
   0,                                                       /* LinearStridePad */
   0xFFFF,                                                  /* PCIVendorID */
   0xFFFF,                                                  /* PCIDeviceID */
   0xFFFF,                                                  /* PCISubSysVendorID */
   0xFFFF,                                                  /* PCISubSysID */
   0                                                        /* Checksum */
};

