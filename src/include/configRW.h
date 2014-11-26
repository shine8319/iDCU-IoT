
UINT32 ReadTransducer(UINT8 *name, UINT16 *transducerId );
int ReadDeviceInfo(UINT8 *name, UINT8 *type, UINT8 *value );
void ReadEnvConfig(UINT8 *name, READENV *env );
void ReadNetworkUseConfig(UINT8 *name, UINT8 *lan, UINT8 *wlan );
void WriteNetworkUseConfig(UINT8 *name, UINT8 *lan, UINT8 *wlan );
