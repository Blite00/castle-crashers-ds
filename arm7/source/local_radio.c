/* Local multiplayer uses the NTR radio driver on both DS and DSi.
 * Link-time wrappers keep the unused TWL infrastructure driver (ath6k/WPA)
 * out of this test core. ARM9 must retain INIT_ONLY | WIFI_LOCAL_ONLY.
 * These signatures match BlocksDS DSWifi's internal setup/update functions.
 */
#ifdef LOCAL_RADIO_ONLY
void Wifi_NTR_Init(void);
void Wifi_NTR_Deinit(void);
void Wifi_NTR_Start(void);
void Wifi_NTR_Stop(void);
void Wifi_NTR_Update(void);
void Wifi_NTR_SetupFilterMode(int mode);
void __wrap_Wifi_TWL_Init(void){Wifi_NTR_Init();}
void __wrap_Wifi_TWL_Deinit(void){Wifi_NTR_Deinit();}
void __wrap_Wifi_TWL_Start(void){Wifi_NTR_Start();}
void __wrap_Wifi_TWL_Stop(void){Wifi_NTR_Stop();}
void __wrap_Wifi_TWL_Update(void){Wifi_NTR_Update();}
void __wrap_Wifi_TWL_SetupFilterMode(int mode){Wifi_NTR_SetupFilterMode(mode);}
#endif
