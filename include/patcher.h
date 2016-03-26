#ifndef PATCHER_H
#define PATCHER_H

#include "reservedarea.h"

typedef struct FuncPattern
{
	u32 Length;
	u32 Loads;
	u32 Stores;
	u32 FCalls;
	u32 Branch;
	u32 Moves;
	u8 *Patch;
	u32 PatchLength;
	char *Name;
	u32 offsetFoundAt;
} FuncPattern;

/* the SDGecko/IDE-EXI patches */
extern u8 hdd_bin[];
extern u32 hdd_bin_size;
extern u8 sd_bin[];
extern u32 sd_bin_size;
extern u8 usbgecko_bin[];
extern u32 usbgecko_bin_size;
extern u8 wkf_bin[];
extern u32 wkf_bin_size;
extern u8 dvd_bin[];
extern u32 dvd_bin_size;

/* SDK patches */
extern u8 DVDCancelAsync[];
extern u32 DVDCancelAsync_length;
extern u8 DVDCancel[];
extern u32 DVDCancel_length;
extern u8 DVDGetDriveStatus[];
extern u32 DVDGetDriveStatus_length;
extern u8 DVDGetCommandBlockStatus[];
extern u32 DVDGetCommandBlockStatus_length;
extern u8 DVDCompareDiskId[];
extern u32 DVDCompareDiskId_length;
extern u8 GXGetYScaleFactorHook[];
extern u32 GXGetYScaleFactorHook_length;
extern u8 GXInitTexObjLODHook[];
extern u32 GXInitTexObjLODHook_length;
extern u8 GXSetProjectionHook[];
extern u32 GXSetProjectionHook_length;
extern u8 GXSetScissorHook[];
extern u32 GXSetScissorHook_length;
extern u8 MTXFrustumHook[];
extern u32 MTXFrustumHook_length;
extern u8 MTXLightFrustumHook[];
extern u32 MTXLightFrustumHook_length;
extern u8 MTXLightPerspectiveHook[];
extern u32 MTXLightPerspectiveHook_length;
extern u8 MTXOrthoHook[];
extern u32 MTXOrthoHook_length;
extern u8 MTXPerspectiveHook[];
extern u32 MTXPerspectiveHook_length;
extern u8 setFbbRegsHook[];
extern u32 setFbbRegsHook_length;
extern u8 VIConfigure240p[];
extern u32 VIConfigure240p_length;
extern u8 VIConfigure288p[];
extern u32 VIConfigure288p_length;
extern u8 VIConfigure480i[];
extern u32 VIConfigure480i_length;
extern u8 VIConfigure480p[];
extern u32 VIConfigure480p_length;
extern u8 VIConfigure576i[];
extern u32 VIConfigure576i_length;
extern u8 VIConfigure576p[];
extern u32 VIConfigure576p_length;
extern u8 VIConfigure960i[];
extern u32 VIConfigure960i_length;
extern u8 VIConfigure1152i[];
extern u32 VIConfigure1152i_length;
extern u8 VIConfigurePanHook[];
extern u32 VIConfigurePanHook_length;
extern u8 VIConfigurePanHookDs[];
extern u32 VIConfigurePanHookDs_length;

#define SWISS_MAGIC 0x53574953 /* "SWIS" */

#define LO_RESERVE 		0x80001000
#define LO_RESERVE_DVD 	0x80001800

/* Function jump locations for the SD/IDE/USBGecko patch */
#define PATCHED_MEMCPY			(LO_RESERVE)
#define CALC_SPEED				(LO_RESERVE | 0x04)
#define STOP_DI_IRQ				(LO_RESERVE | 0x08)
#define READ_TRIGGER_INTERRUPT	(LO_RESERVE | 0x0C)
#define DSP_HANDLER_HOOK		(LO_RESERVE | 0x10)
#define PATCHED_MEMCPY_DBG		(LO_RESERVE | 0x14)

/* Function jump locations for the DVD patch */
#define ENABLE_BACKUP_DISC 		(LO_RESERVE_DVD | 0x00)
#define READ_REAL_OR_PATCHED	(LO_RESERVE_DVD | 0x04)

/* Function jump locations for the WKF/WASP patch */
#define ADJUST_LBA_OFFSET	 	(LO_RESERVE_DVD)

#define READ_PATCHED_ALL 		(0x111)

/* Types of files we may patch */
#define PATCH_DOL		0
#define PATCH_ELF		1
#define PATCH_LOADER	2

/* The device patches for a particular game were written to */
// -1 no device, 0 slot a, 1 slot b.
extern int savePatchDevice;

u32 Patch_DVDLowLevelReadForWKF(void *addr, u32 length, int dataType);
u32 Patch_DVDLowLevelReadForDVD(void *addr, u32 length, int dataType);
u32 Patch_DVDLowLevelRead(void *addr, u32 length, int dataType);
void Patch_VidMode(u8 *data, u32 length, int dataType);
void Patch_WideAspect(u8 *data, u32 length, int dataType);
int Patch_TexFilt(u8 *data, u32 length, int dataType);
int Patch_FontEnc(void *addr, u32 length);
int Patch_Fwrite(void *addr, u32 length);
int Patch_DVDReset(void *addr,u32 length);
void Patch_GameSpecific(void *addr, u32 length, const char* gameID);
u32 Calc_ProperAddress(u8 *data, u32 type, u32 offsetFoundAt);
int Patch_CheatsHook(u8 *data, u32 length, u32 type);
int install_code();


#endif

