
UINT32 ReadTransducer(UINT8 *name, UINT16 *transducerId );
int ReadDeviceInfo(UINT8 *name, UINT8 *type, UINT8 *value );
void ReadEnvConfig(UINT8 *name, READENV *env );
void ReadNetworkUseConfig(UINT8 *name, UINT8 *lan, UINT8 *wlan );
void ReadOptionsConfig(UINT8 *name, UINT8 *lan, UINT8 *wlan );
void WriteNetworkUseConfig(UINT8 *name, UINT8 *lan, UINT8 *wlan );
void WriteOptionsConfig(UINT8 *name, UINT8 *lan, UINT8 *wlan );
void WriteSerialConfig( UINT8 *name, COMPORTINFO_VAR  *comport, CONNECTINFO_VAR *connect );
void WriteTimeSyncConfig( UINT8 *name, TIMESYNCINFO_VAR  *timeSync );
