
#define SENSING_VALUE_REPORT_HEAD_SIZE	18
enum {
	eGateNode_Registration	    = 0x01,
	eGateNode_Deregistration	    = 0x02,
	ePAN_Registration	    = 0x03,
	ePAN_Deregistration	    = 0x04,
	eSensorNode_Registration	    = 0x05,
	eSensorNode_Deregistration   = 0x06,
	eTransducer_Registration	    = 0x07,
	eTransducer_Deregistration   = 0x08,

	eSensing_Value_Report	    = 0x09,
	eMonitoring_Command	    = 0x10,
	eMonitoring_Report	    = 0x11,
	ePush_Control		    = 0x12,
	eUser_Defined_Message	    = 0x13,
	eStored_Command_Request	    = 0x14,
	eStored_Command_Response	    = 0x15,

	eInstant_Command		    = 0x20,
	eContinuous_Command	    = 0x21,

	eACK			    = 0xff,
};


typedef struct Struct_GateNode {
    UINT16  Message_Id;
    UINT16  Length;
    UINT16  GateNode_Id;
} __attribute__ ((__packed__)) Struct_GateNode;

typedef struct Struct_PAN {
    UINT16  Message_Id;
    UINT16  Length;
    UINT16  GateNode_Id;
    UINT16  PAN_Id;
} __attribute__ ((__packed__)) Struct_PAN;

typedef struct Struct_SensorNode {
    UINT16  Message_Id;
    UINT16  Length;
    UINT16  GateNode_Id;
    UINT16  PAN_Id;
    UINT16  SensorNode_Id;
} __attribute__ ((__packed__)) Struct_SensorNode;

typedef struct Struct_Transducer {
    UINT16  Message_Id;
    UINT16  Length;
    UINT16  GateNode_Id;
    UINT16  PAN_Id;
    UINT16  SensorNode_Id;
    UINT16  Transducer_Id;
} __attribute__ ((__packed__)) Struct_Transducer;


typedef struct Struct_GateNode_Registration {
    Struct_GateNode GateNode;
    UINT8	    Manufacturer[20];
    UINT8	    ProducNo[20];
    INT32	    Longitude;
    INT32	    Latitude;
    INT32	    Altitude;
} __attribute__ ((__packed__)) Struct_GateNode_Registration;

typedef struct Struct_PAN_Registration {
    Struct_PAN	    PAN;
    UINT8	    PAN_Channel;
    UINT8	    Manufacturer[20];
    UINT8	    ProducNo[20];
    UINT8	    PAN_Topology;
    UINT8	    PAN_Protocol_Stack;
} __attribute__ ((__packed__)) Struct_PAN_Registration;

typedef struct Struct_SensorNode_Registration {
    Struct_SensorNode	SensorNode;
    UINT8		Global_Id[12];
    UINT8		Monitoring_Mode;
    INT32		Monitoring_Period;
    UINT8		Manufacturer[20];
    UINT8		ProducNo[20];
    INT32		Longitude;
    INT32		Latitude;
    INT32		Altitude;
} __attribute__ ((__packed__)) Struct_SensorNode_Registration;

typedef struct Struct_Transducer_Registration {
    Struct_Transducer	Transducer;
    UINT8		Manufacturer[20];
    UINT8		ProducNo[20];
    float		Min;
    float		Max;
    float		Offset;
    float		Level;
    UINT8		TransducerType;
    UINT8		DataType;
    UINT8		DataUnitLength;
    UINT8		DataUnit[32];
} __attribute__ ((__packed__)) Struct_Transducer_Registration;

typedef struct Struct_TransducerInfo {
    UINT16  Transducer_Id;
    UINT32  Sensing_Time;
    UINT8   Data_Length;
 
} __attribute__ ((__packed__)) Struct_TransducerInfo;

typedef struct Struct_TR51{
    Struct_TransducerInfo TransducerInfo;
    UINT8   Sensing_Value;
} __attribute__ ((__packed__)) Struct_TR51;


typedef struct Struct_TR52{
    Struct_TransducerInfo TransducerInfo;
    UINT32   Sensing_Value;
} __attribute__ ((__packed__)) Struct_TR52;

typedef struct Struct_ACK{
    UINT16  Message_Id;
    UINT16  Length;
    UINT32  Command_Id;
    UINT16  GateNode_Id;
    UINT16  PAN_Id;
    UINT16  SensorNode_Id;
    UINT16  Transducer_Id;
    UINT8  Result_Code;
  } __attribute__ ((__packed__)) Struct_ACK;


typedef struct Struct_SensingValueReport {
    UINT16  Message_Id;
    UINT16  Length;
    UINT32  Command_Id;
    UINT16  GateNode_Id;
    UINT16  PAN_Id;
    UINT16  SensorNode_Id;
    UINT32  Transfer_Time;
    UINT8   Transducer_Info[1024];
    /*
    UINT8   Transducer_Count;
    Struct_TR51	TR51;
    Struct_TR52	TR52;
    */
  } __attribute__ ((__packed__)) Struct_SensingValueReport;

typedef struct Struct_UserDefinedMessage {
    Struct_SensorNode SensorNode_Info;
    UINT32  Message_Length;
    UINT8   Message[1024];
} __attribute__ ((__packed__)) Struct_UserDefinedMessage;


