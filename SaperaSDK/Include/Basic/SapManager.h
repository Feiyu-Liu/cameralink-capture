#ifndef _SAPMANAGER_H_
#define _SAPMANAGER_H_

//	SapManager.h : header file
//

#include "SapClassBasicDef.h"
#include <stdexcept>

//
// SapLocation class declaration
//
class SAPCLASSBASIC_CLASS SapLocation
{
public:
   // Common server indices
   enum ServerIndex
   {
      ServerUnknown = -1,
      ServerSystem = 0
   };

   // Common resource indices
   enum ResourceIndex
   {
      ResourceUnknown = -1
   };

public:
	// Constructor/Destructor
	SapLocation();
	SapLocation(int serverIndex, int resourceIndex = 0);
	SapLocation(const char *serverName, int resourceIndex = 0);
	SapLocation(int serverIndex, const char *serverName, int resourceIndex);
	SapLocation(const SapLocation &loc);
	SapLocation(const SapLocation &loc, int resourceIndex);

	virtual ~SapLocation() {}

	int GetServerIndex() const { return m_ServerIndex; }
	const char *GetServerName() const { return m_ServerName; }
	int GetResourceIndex() const { return m_ResourceIndex; }
	BOOL IsUnknown() const { return m_ServerIndex < 0 && !strlen(m_ServerName); }

   // Obsolete method
	BOOL IsNull() const { return IsUnknown(); }

protected:
	int m_ServerIndex;
	char m_ServerName[CORSERVER_MAX_STRLEN];
	int m_ResourceIndex;
};


// Set current function name for error messages
#define SAP_FUNCNAME(funcName) \
   static const char *sapFuncName = funcName;

// Standard exception handling block. Use together with SAP_FUNCNAME and a try-block.
#define SAP_CATCH                                                                              \
   catch (const std::exception & e)                                                            \
   {                                                                                           \
      SapManager::DisplayMessageAndSave(sapFuncName, e.what(), __FILE__, __LINE__);            \
   }                                                                                           \
   catch (...)                                                                                 \
   {                                                                                           \
      SapManager::DisplayMessageAndSave(sapFuncName, "unknown exception", __FILE__, __LINE__); \
   }

// Check for an error condition with possible arguments for formatting the error message
#define SAP_CHECK(errorCondition, errorMsg, statement)                  \
   {                                                                       \
      if ((errorCondition))                                                \
      {                                                                    \
         if (strlen(errorMsg) != 0)                                           \
            SapManager::DisplayMessageAndSave(sapFuncName, errorMsg, __FILE__, __LINE__); \
         statement;                                                        \
      }                                                                    \
   }

// Signal an error condition
#define SAP_ERROR(errorMsg, statement)                                  \
   {                                                                    \
      if (strlen(errorMsg) != 0)                                           \
         SapManager::DisplayMessageAndSave(sapFuncName, errorMsg, __FILE__, __LINE__); \
      statement;                                                        \
   }

#define SAP_ERROR_SHORT(errorMsg)                                       \
   {                                                                    \
      if (strlen(errorMsg) != 0)                                           \
         SapManager::DisplayMessageAndSave(sapFuncName, errorMsg, __FILE__, __LINE__); \
   }

#define SAP_ERROR_NOSAVE(errorMsg, statement)                                  \
   {                                                                    \
      if (strlen(errorMsg) != 0)                                           \
         SapManager::DisplayMessage(sapFuncName, errorMsg, __FILE__, __LINE__); \
      statement;                                                        \
   }

// Valid only for classes that implement the Destroy() method
#define SAP_DESTROY(errorMsg, statement)                                \
   {                                                                    \
      if (strlen(errorMsg) != 0)                                           \
         SapManager::DisplayMessageAndSave(sapFuncName, errorMsg, __FILE__, __LINE__); \
      Destroy();                                                        \
      statement;                                                        \
   }

#define SAP_DESTROY_NOSAVE(errorMsg, statement)                                \
   {                                                                    \
      if (strlen(errorMsg) != 0)                                           \
         SapManager::DisplayMessage(sapFuncName, errorMsg, __FILE__, __LINE__); \
      Destroy();                                                        \
      statement;                                                        \
   }

// Show a message for the current function with one possible arguments for formatting
#define SAP_MESSAGE(message)                                    \
   {                                                            \
      if (strlen(message) != 0)                                    \
         SapManager::DisplayMessage(sapFuncName, message, __FILE__, __LINE__); \
   }


// Forward declarations
class SAPCLASSBASIC_CLASS SapManCallbackInfo;
class SAPCLASSBASIC_CLASS SapManVersionInfo;
class SAPCLASSBASIC_CLASS SapInterface;
class SAPCLASSBASIC_CLASS SapResourceInfo;
typedef void (*SapManCallback)(SapManCallbackInfo *);

//
// SapManager class declaration
//

class SAPCLASSBASIC_CLASS SapManager
{
public:
   // Server types
   typedef int ServerType;    // For compatibility with old Sapera++ application code
   enum _ServerType
   {
      ServerNone         = 0,
      ServerSystem       = CORPORT_SYSTEM,

      ServerCobra        = CORPORT_COBRA,
      ServerViperRgb     = CORPORT_VIPERRGB,
      ServerViperDigital = CORPORT_VIPERDIG,
      ServerViperQuad    = CORPORT_VIPERQUAD,
      ServerViperCamLink = CORPORT_VIPERCAMLINK,
      ServerBanditII     = CORPORT_BANDITII,
      ServerBandit3MV    = CORPORT_NEXUS,
      ServerBandit3CV    = CORPORT_NEXUSCV,
      ServerX64CL        = CORPORT_X64,
      ServerX64LVDS      = CORPORT_X64LVDS,
      ServerX64NS        = CORPORT_X64NS,
      ServerX64Analog    = CORPORT_X64AN,       // Obsolete, use ServerX64ANQuad instead
      ServerX64ANQuad    = CORPORT_X64AN,
      ServerX64AN2       = CORPORT_X64AN2,
      ServerX64ANLX1     = CORPORT_X64ANLX1,
      ServerX64CLiPRO    = CORPORT_X64IPRO,
      ServerX64CLiPROe   = CORPORT_X64IPROE,    // Obsolete, use ServerX64CLExpress instead
      ServerX64CLExpress = CORPORT_X64IPROE,
      ServerX64CLGigE    = CORPORT_X64INET,
      ServerPC2Vision    = CORPORT_PC2V,
      ServerPC2Comp      = CORPORT_PC2COMP,
      ServerPC2CamLink   = CORPORT_PC2C,
      ServerMamba        = CORPORT_MAMBA,
      ServerAnaconda     = CORPORT_ANACONDA,    // Obsolete, use ServerAnacondaCL instead
      ServerAnacondaCL   = CORPORT_ANACONDA,
      ServerAnacondaLVDS = CORPORT_ANACONDALVDS,
      ServerXriCL        = CORPORT_XRICL,
      ServerXriLVDS      = CORPORT_XRILVDS,

      ServerX64CLPX4       = CORPORT_X64IPROE4X,      // Obsolete, use ServerXceleraCLPX4 instead
      ServerXceleraCLPX4   = CORPORT_X64IPROE4X,
      ServerX64CLLX4       = CORPORT_XCELERAL4X,      // Obsolete, use ServerXceleraCLLX4 instead
      ServerXceleraCLLX4   = CORPORT_XCELERAL4X,
      ServerX64CLVX4       = CORPORT_XCELERAV4XCL,    // Obsolete, use ServerXceleraCLVX4 instead
      ServerXceleraCLVX4   = CORPORT_XCELERAV4XCL,
      ServerX64LVDSPX4     = CORPORT_XCELERAPX4LVDS,  // Obsolete, use ServerXceleraLVDSPX4 instead
      ServerXceleraLVDSPX4 = CORPORT_XCELERAPX4LVDS,
      ServerX64LVDSVX4     = CORPORT_XCELERAV4XLVDS,  // Obsolete, use ServerXceleraLVDSVX4 instead
      ServerXceleraLVDSVX4 = CORPORT_XCELERAV4XLVDS,
      ServerX64XRICL       = CORPORT_XCELERAXRICL,    // Obsolete, use ServerXceleraXRICL instead
      ServerXceleraXRICL   = CORPORT_XCELERAXRICL,
      ServerX64XRILVDS     = CORPORT_XCELERAXRILVDS,  // Obsolete, use ServerXceleraXRILVDS instead
      ServerXceleraXRILVDS = CORPORT_XCELERAXRILVDS,
      ServerXceleraCLLX1   = CORPORT_XCELERALX1CL,
      ServerXceleraHSPX8   = CORPORT_XCELERAHSPX8,
      ServerXceleraCLPPX8  = CORPORT_XCELERACLpPX8,
      ServerXceleraCLHSPX8 = CORPORT_XCELERACLHSPX8,

      ServerXtiumCLHSPX8 = CORPORT_XTIUMCLHSPX8,
      ServerXtiumCLPX4   = CORPORT_XTIUMCLPX4,
      ServerXtiumCXPPX8  = CORPORT_XTIUMCXPPX8,
      ServerXtiumCLHSPX4 = CORPORT_XTIUMCLHSPX4,
      ServerXtiumSFPMPX4 = CORPORT_XTIUMSFPMPX4,
      ServerXtiumCLMX4   = CORPORT_XTIUMCLMX4,
      ServerXtiumCLCZM   = CORPORT_XTIUMCLCZM,

      ServerXtium2CLHSPX8   = CORPORT_XTIUM2CLHSPX8,
      ServerXtium2CLHSFX8   = CORPORT_XTIUM2SFPXPX8,
      ServerXtium2CLHSFX8LC = CORPORT_XTIUM2CLHSFX8LC,
      ServerXtium2CLHSPX8LC = CORPORT_XTIUM2CLHSPX8LC,
      ServerXtium2CXPPX8    = CORPORT_XTIUM2CXPPX8,
      ServerXtium2CLMX4    = CORPORT_XTIUM2CLMX4,
      ServerXtium2CLVX8    = CORPORT_XTIUM2CLVX8,
      ServerXtium2GIGEPX8    = CORPORT_XTIUM2GIGEPX8,

      ServerGenie      = CORPORT_CAM_GIGE,   // Obsolete, use ServerGigEVision instead
      ServerGigEVision = CORPORT_CAM_GIGE,
      ServerCameraLink = CORPORT_CLPROTOCOL,
      ServerUSB3Vision = CORPORT_CAM_U3V,
   };

   // Resource types
   typedef int ResType;    // For compatibility with old Sapera++ application code
   enum _ResType
   {
      ResourceAcq = 0,
      ResourceDisplay,
      ResourceObsolete2,
      ResourcePixPro,
      ResourceRtPro = ResourcePixPro,
      ResourceObsolete1,
      ResourceGraphic,
      ResourceGio,
      ResourceObsolete3,
      ResourceAcqDevice,
      ResourceLast = ResourceAcqDevice
   };

   // detection server types
   typedef int DetectServerType;
   enum _DetectServerType
   {
      DetectServerGenCP = CORMAN_VAL_DETECTION_SERVER_TYPE_GENCP,
      DetectServerAll   = CORMAN_VAL_DETECTION_SERVER_TYPE_ALL
   };

   // Sapera error return codes
   // To be completed later if necessary
   enum StatusCode
   {
      StatusOk = CORSTATUS_OK,
      StatusInvalidHandle = CORSTATUS_INVALID_HANDLE,
      StatusIncompatibleAcq = CORSTATUS_INCOMPATIBLE_ACQ	,
      StatusIncompatibleBuffer = CORSTATUS_INCOMPATIBLE_BUFFER,
      StatusIncompatibleCab = CORSTATUS_INCOMPATIBLE_CAB	,
      StatusIncompatibleCam = CORSTATUS_INCOMPATIBLE_CAM,
      StatusIncompatibleDisplay = CORSTATUS_INCOMPATIBLE_DISPLAY,
      StatusIncompatibleGraphic = CORSTATUS_INCOMPATIBLE_GRAPHIC,
      StatusIncompatibleKernel = CORSTATUS_INCOMPATIBLE_KERNEL	,
      StatusIncompatibleLut = CORSTATUS_INCOMPATIBLE_LUT	,
      StatusIncompatibleManager = CORSTATUS_INCOMPATIBLE_MANAGER	,
      StatusIncompatiblePro = CORSTATUS_INCOMPATIBLE_PRO	,
      StatusIncompatibleVic = CORSTATUS_INCOMPATIBLE_VIC	,
      StatusIncompatibleView = CORSTATUS_INCOMPATIBLE_VIEW,
      StatusIncompatibleXfer = CORSTATUS_INCOMPATIBLE_XFER,
      StatusIncompatibleString = CORSTATUS_INCOMPATIBLE_STRING	,
      StatusIncompatibleObject = CORSTATUS_INCOMPATIBLE_OBJECT	,
      StatusIncompatibleArray = CORSTATUS_INCOMPATIBLE_ARRAY	,	
      StatusIncompatibleStream = CORSTATUS_INCOMPATIBLE_STREAM	,	
      StatusIncompatibleFile = CORSTATUS_INCOMPATIBLE_FILE		,
      StatusCapInvalid = CORSTATUS_CAP_INVALID				,
      StatusCapNotAvailable = CORSTATUS_CAP_NOT_AVAILABLE,		
      StatusPrmInvalid = CORSTATUS_PRM_INVALID				,
      StatusPrmNotAvailable = CORSTATUS_PRM_NOT_AVAILABLE,		
      StatusPrmOutOfRange = CORSTATUS_PRM_OUT_OF_RANGE	,		
      StatusPrmInvalidValue = CORSTATUS_PRM_INVALID_VALUE,		
      StatusPrmReadOnly = CORSTATUS_PRM_READ_ONLY			,	
      StatusPrmMutuallyExclusive = CORSTATUS_PRM_MUTUALLY_EXCLUSIVE	,
      StatusPrmWriteOnly = CORSTATUS_PRM_WRITE_ONLY      ,
      StatusArgInvalid = CORSTATUS_ARG_INVALID				,
      StatusArgOutOfRange = CORSTATUS_ARG_OUT_OF_RANGE	,	
      StatusArgIncompatible = CORSTATUS_ARG_INCOMPATIBLE	,		
      StatusArgInvalidValue = CORSTATUS_ARG_INVALID_VALUE,		
      StatusArgNull = CORSTATUS_ARG_NULL					    , 
      StatusFileOptionsError = CORSTATUS_FILE_OPTIONS_ERROR,     
      StatusFileOpenModeInvalid = CORSTATUS_FILE_OPEN_MODE_INVALID	,
      StatusFileSeekError = CORSTATUS_FILE_SEEK_ERROR			,
      StatusFileCreateError = CORSTATUS_FILE_CREATE_ERROR	,	
      StatusFileOpenError = CORSTATUS_FILE_OPEN_ERROR			,
      StatusFileReadError = CORSTATUS_FILE_READ_ERROR			,
      StatusFileWriteError = CORSTATUS_FILE_WRITE_ERROR		,	
      StatusFileCloseError = CORSTATUS_FILE_CLOSE_ERROR		,	
      StatusFileFormatUnkown = CORSTATUS_FILE_FORMAT_UNKNOWN ,	
      StatusFileFieldValueNotSupported = CORSTATUS_FILE_FIELD_VALUE_NOT_SUPPORTED ,
      StatusFileGetFieldError = CORSTATUS_FILE_GET_FIELD_ERROR	,
      StatusFileReadOnly = CORSTATUS_FILE_READ_ONLY			,
      StatusFileWriteOnly = CORSTATUS_FILE_WRITE_ONLY			 ,     
      StatusNotImplemented = CORSTATUS_NOT_IMPLEMENTED		,	
      StatusNoMemory = CORSTATUS_NO_MEMORY					,
      StatusClippingOccurred = CORSTATUS_CLIPPING_OCCURED,			
      StatusHardwareError = CORSTATUS_HARDWARE_ERROR		,	
      StatusServiceNotAvailable = CORSTATUS_SERVICE_NOT_AVAILABLE	,
      StatusNotAccessible = CORSTATUS_NOT_ACCESSIBLE			,
      StatusNotAvailable = CORSTATUS_NOT_AVAILABLE				,
      StatusRoutingNotImplemented = CORSTATUS_ROUTING_NOT_IMPLEMENTED,
      StatusRoutingNotAvailable = CORSTATUS_ROUTING_NOT_AVAILABLE	,
      StatusRoutingInUse = CORSTATUS_ROUTING_IN_USE			,
      StatusIncompatibleSize = CORSTATUS_INCOMPATIBLE_SIZE	,	
      StatusIncompatibleFormat = CORSTATUS_INCOMPATIBLE_FORMAT		,
      StatusIncompatibleLocation = CORSTATUS_INCOMPATIBLE_LOCATION,	
      StatusResourceInUse = CORSTATUS_RESOURCE_IN_USE			,
      StatusResourceLinked = CORSTATUS_RESOURCE_LINKED		,	
      StatusSoftwareError = CORSTATUS_SOFTWARE_ERROR			,
      StatusParametersLocked = CORSTATUS_PARAMETERS_LOCKED	,	
      StatusXferNotConnected = CORSTATUS_XFER_NOT_CONNECTED ,	     
      StatusXferEmptyList = CORSTATUS_XFER_EMPTY_LIST		   ,
      StatusXferCantCycle = CORSTATUS_XFER_CANT_CYCLE  		,
      StatusRoutingNotSpecified = CORSTATUS_ROUTING_NOT_SPECIFIED	,
      StatusTransferInProgress = CORSTATUS_TRANSFER_IN_PROGRESS	,
      StatusApiNotLocked = CORSTATUS_API_NOT_LOCKED			,
      StatusServerNotFound = CORSTATUS_SERVER_NOT_FOUND		,	
      StatusCannotSignalEvent = CORSTATUS_CANNOT_SIGNAL_EVENT,		
      StatusNoMessage = CORSTATUS_NO_MESSAGE					,
      StatusTimeOut = CORSTATUS_TIMEOUT						,
      StatusInvalidAlignment = CORSTATUS_INVALID_ALIGNMENT,		
      StatusDdraw256Colors = CORSTATUS_DDRAW_256_COLORS	,		
      StatusPciIoError = CORSTATUS_PCI_IO_ERROR				,
      StatusPciCannotAccessDevice = CORSTATUS_PCI_CANNOT_ACCESS_DEVICE,
      StatusEventCreateError = CORSTATUS_EVENT_CREATE_ERROR       ,
      StatusBoardNotReady = CORSTATUS_BOARD_NOT_READY			,
      StatusXferMaxSize = CORSTATUS_XFER_MAX_SIZE            ,
      StatusProcessingError = CORSTATUS_PROCESSING_ERROR	     ,   
      StatusResourceLocked = CORSTATUS_RESOURCE_LOCKED         , 
      StatusNoMessagingMemory = CORSTATUS_NO_MESSAGING_MEMORY	,	  
      StatusDdrawNotAvailable = CORSTATUS_DDRAW_NOT_AVAILABLE   ,   
      StatusDdrawError = CORSTATUS_DDRAW_ERROR              ,
      StatusResourceNotLocked = CORSTATUS_RESOURCE_NOT_LOCKED,      
      StatusDiskOnChipError = CORSTATUS_DISK_ON_CHIP_ERROR    ,   
      StatusAuthentificationFailed = CORSTATUS_AUTHENTIFICATION_FAILED  ,
      StatusInsufficientBandwidth = CORSTATUS_INSUFFICIENT_BANDWIDTH   ,
      StatusFileTellError = CORSTATUS_FILE_TELL_ERROR			,
      StatusMaxProcessExceeded = CORSTATUS_MAX_PROCESS_EXCEEDED     ,
      StatusXferCountMultSrcFrameCount = CORSTATUS_XFER_COUNT_MULT_SRC_FRAME_COUNT  ,
      StatusAcqConnectedToXfer = CORSTATUS_ACQ_CONNECTED_TO_XFER		,
      StatusInsufficientBoardMemory = CORSTATUS_INSUFFICIENT_BOARD_MEMORY ,
      StatusInsufficientResources = CORSTATUS_INSUFFICIENT_RESOURCES,
      StatusMissingResource = CORSTATUS_MISSING_RESOURCE,
      StatusNoDeviceFound = CORSTATUS_NO_DEVICE_FOUND,
      StatusResourceNotConnected = CORSTATUS_RESOURCE_NOT_CONNECTED,
      StatusServerDatabaseFull = CORSTATUS_SERVER_DATABASE_FULL,
      StatusDeviceNotConnected = CORSTATUS_DEVICE_NOT_CONNECTED,
      StatusResourceAccess = CORSTATUS_RESOURCE_ACCESS,
      StatusDeviceNotResponding = CORSTATUS_DEVICE_NOT_RESPONDING,
      StatusDataInvalid = CORSTATUS_DATA_INVALID,
      StatusResourceRead = CORSTATUS_RESOURCE_READ,
      StatusResourceWrite = CORSTATUS_RESOURCE_WRITE,
      StatusConnectionDropped = CORSTATUS_CONNECTION_DROPPED,
      StatusEvaluationPeriodExpired = CORSTATUS_EVALUATION_PERIOD_EXPIRED,
      StatusExternalPowerNoPresent = CORSTATUS_EXTERNAL_POWER_NO_PRESENT,
      StatusCameraPowerError = CORSTATUS_CAMERA_POWER_ERROR,
      StatusRebootRequired = CORSTATUS_REBOOT_REQUIRED,
      StatusIncompatibleFirmware = CORSTATUS_INCOMPATIBLE_FIRMWARE,
      StatusIncompatibleServer = CORSTATUS_INCOMPATIBLE_SERVER,
      StatusDeviceContextualError = CORSTATUS_DEVICE_CONTEXTUAL_ERROR,
      StatusNoCameraConfigSaved = CORSTATUS_NO_CAMERA_CONFIG_SAVED,
   };

   // Status reporting modes
   typedef int StatusMode;    // For compatibility with old Sapera++ application code
   enum _StatusMode
   {
      StatusNotify = 0,
      StatusLog,
      StatusDebug,
      StatusCustom,
      StatusCallback
   };

   // Manager event types
   typedef int EventType;
   enum _EventType
   {
      EventNone                = 0,
      EventServerNew           = CORMAN_VAL_EVENT_TYPE_SERVER_NEW,
      EventServerDisconnected  = CORMAN_VAL_EVENT_TYPE_SERVER_DISCONNECTED,
      EventServerConnected     = CORMAN_VAL_EVENT_TYPE_SERVER_CONNECTED,
      EventServerDatabaseFull  = CORMAN_VAL_EVENT_TYPE_SERVER_DATABASE_FULL,
      EventResourceInfoChanged = CORMAN_VAL_EVENT_TYPE_RESOURCE_INFO_CHANGED,
      EventServerFile          = CORMAN_VAL_EVENT_TYPE_SERVER_FILE,

      // obsolete, use EventServerDisconnected instead
      EventServerNotAccessible = CORMAN_VAL_EVENT_TYPE_SERVER_NOT_ACCESSIBLE,
      // obsolete, use EventServerConnected instead
      EventServerAccessible    = CORMAN_VAL_EVENT_TYPE_SERVER_ACCESSIBLE,
   };

   // Various maximum values
   enum MiscValues
   {
      MaxServers = CORSERVER_MAX_NUMBER,
      MaxResourceTypes = ResourceLast + 1,
      MaxResources = 16,
      MaxLabelSize = 64,
      LockTimeout = 10000,
   };

public:
	// Constructor/Destructor
	SapManager();
	virtual ~SapManager();

   // Explicitly initialize and terminate access to Sapera LT libraries
   static BOOL Open();
   static BOOL Close();

   // Version and licensing information
   static BOOL GetVersionInfo(SapManVersionInfo *pVersionInfo);

	// Server-access methods
	static int GetServerCount();
	static int GetServerCount(ResType resourceType);

	static int GetServerIndex(const char *serverName);
	static int GetServerIndex(SapLocation loc);
	static int GetServerIndex(const char *deviceUserId, int reserved);

   // Note: nameSize argument (formerly bufSize) is now obsolete
	static BOOL GetServerName(int serverIndex, char *serverName, int nameSize = CORSERVER_MAX_STRLEN);
	static BOOL GetServerName(SapLocation loc, char *serverName, int nameSize = CORSERVER_MAX_STRLEN);
	static BOOL GetServerName(const char *deviceUserId, char *serverName, int nameSize = CORSERVER_MAX_STRLEN);
	static BOOL GetServerName(int serverIndex, ResType resourceType, char *serverName);

	static BOOL GetServerHandle(int serverIndex, PCORSERVER pServer);
	static BOOL GetServerHandle(const char *serverName, PCORSERVER pServer);
	static BOOL GetServerHandle(SapLocation loc, PCORSERVER pServer);
	static BOOL GetServerHandle(const char *deviceUserId, PCORSERVER pServer, int reserved);

	static ServerType GetServerType(int serverIndex);
	static ServerType GetServerType(const char *serverName);
	static ServerType GetServerType(SapLocation loc);

   static BOOL IsServerAccessible(int serverIndex);
   static BOOL IsServerAccessible(const char *serverName);
   static BOOL IsServerAccessible(SapLocation loc);

   static BOOL WriteFile(int serverIndex, const char *localFilePath, int deviceFileIndex = -1);
   static BOOL WriteFile(const char* serverName, const char *localFilePath, int deviceFileIndex = -1);
   static BOOL WriteFile(SapLocation loc, const char *localFilePath, int deviceFileIndex = -1);

	static BOOL GetServerSerialNumber(int serverIndex, char *serialNumber);
	static BOOL GetServerSerialNumber(const char *serverName, char *serialNumber);
	static BOOL GetServerSerialNumber(SapLocation loc, char *serialNumber);

   static BOOL ResetServer(int serverIndex, BOOL wait = TRUE, SapManCallback pCallback = NULL, void *pContext = NULL);
   static BOOL ResetServer(const char *serverName, BOOL wait = TRUE, SapManCallback pCallback = NULL, void *pContext = NULL);
   static BOOL ResetServer(SapLocation loc, BOOL wait = TRUE, SapManCallback pCallback = NULL, void *pContext = NULL);

   static BOOL DetectAllServers(DetectServerType type = DetectServerAll);

   static BOOL GetDeviceInfoValueSize(int serverIndex, const char *infoValueName, int *infoValueSize);
   static BOOL GetDeviceInfoValueSize(const char *serverName, const char *infoValueName, int *infoValueSize);
   static BOOL GetDeviceInfoValueSize(SapLocation loc, const char *infoValueName, int *infoValueSize);

   static BOOL ReadDeviceInfoValue(int serverIndex, const char *infoValueName, char *infoValue, int infoValueSize);
   static BOOL ReadDeviceInfoValue(const char *serverName, const char *infoValueName, char *infoValue, int infoValueSize);
   static BOOL ReadDeviceInfoValue(SapLocation loc, const char *infoValueName, char *infoValue, int infoValueSize);

   static BOOL GetSerialPortCount(int serverIndex, int *serialCount);
   static BOOL GetSerialPortCount(const char *serverName, int *serialCount);
   static BOOL GetSerialPortCount(SapLocation loc, int *serialCount);

   static BOOL SetSerialPortComIndex(int serverIndex, int serialIndex, int comIndex);
   static BOOL SetSerialPortComIndex(const char *serverName, int serialIndex, int comIndex);
   static BOOL SetSerialPortComIndex(SapLocation loc, int serialIndex, int comIndex);

   static BOOL GetSerialPortComIndex(int serverIndex, int serialIndex, int *comIndex);
   static BOOL GetSerialPortComIndex(const char *serverName, int serialIndex, int *comIndex);
   static BOOL GetSerialPortComIndex(SapLocation loc, int serialIndex, int *comIndex);

	// Resource-access methods
	static int GetResourceCount(int serverIndex, ResType resourceType);
	static int GetResourceCount(const char *serverName, ResType resourceType);
	static int GetResourceCount(SapLocation loc, ResType resourceType);

   // Note: nameSize argument (formerly bufSize) is now obsolete
	static BOOL GetResourceName(int serverIndex, ResType resourceType, int resourceIndex, char *resourceName, int nameSize = MaxLabelSize);
	static BOOL GetResourceName(const char *serverName, ResType resourceType, int resourceIndex, char *resourceName, int nameSize = MaxLabelSize);
	static BOOL GetResourceName(SapLocation loc, ResType resourceType, char *resourceName, int nameSize = MaxLabelSize);

   static int GetResourceIndex(int serverIndex, ResType resourceType, const char *resourceName);
   static int GetResourceIndex(const char *serverName, ResType resourceType, const char *resourceName);

	static BOOL IsResourceAvailable(int serverIndex, ResType resourceType, int resourceIndex);
	static BOOL IsResourceAvailable(const char *serverName, ResType resourceType, int resourceIndex);
	static BOOL IsResourceAvailable(SapLocation loc, ResType resourceType);

   static BOOL GetInstallDirectory(int serverIndex, char *installDir);
   static BOOL GetInstallDirectory(const char *serverName, char *installDir);
   static BOOL GetInstallDirectory(SapLocation loc, char *installDir);

   // Server callback functionality
   static BOOL RegisterServerCallback(EventType eventType, SapManCallback callback, void *context = NULL);
   static BOOL UnregisterServerCallback(void);
   static EventType GetServerEventType() { return m_ServerEventType; }

	// Utility methods
#if COR_LINUX
   static BOOL IsSystemLocation() { return TRUE; }
#else
   static BOOL IsSystemLocation() { return CorManIsSystemHandle(CorManGetLocalServer()); }
#endif
   static BOOL IsSystemLocation(SapLocation loc);
	static BOOL IsSameServer(SapLocation loc1, SapLocation loc2);
	static BOOL IsSameLocation(SapLocation loc1, SapLocation loc2);
   static int GetPixelDepthMin(SapFormat format);
   static int GetPixelDepthMax(SapFormat format);
   static SapFormatType GetFormatType(SapFormat format);
	static BOOL GetStringFromFormat( SapFormat format, char* txtFormat);
   static int GetCommandTimeout();
   static void SetCommandTimeout(int commandTimeout);
   static int GetResetTimeout() { return m_ResetTimeout; }
   static void SetResetTimeout(int resetTimeout) { m_ResetTimeout = resetTimeout; }

	// Status reporting methods
	static StatusMode GetDisplayStatusMode() { return m_DisplayStatusMode; }
	static BOOL SetDisplayStatusMode(StatusMode mode, SapManCallback pCallback = NULL, void *pContext = NULL);
	static SAPSTATUS DisplayStatus(const char *functionName, SAPSTATUS status);
	static void DisplayMessage(const char *message, const char *fileName = NULL, int lineNumber = 0, ...);
   static void DisplayMessageAndSave(const char *funcName, const char *message, const char *fileName, int lineNumber);
   static void GetLastStatus(SAPSTATUS *pLastStatus);
   static const char *GetLastStatus();

   static BOOL IsStatusOk(const char *funcName, SAPSTATUS status) { return DisplayStatus(funcName, status) == StatusOk; }

   // Check if all handles were created successfully  (derived classes only)
	operator BOOL() const {return m_bInitOK;}

   // Internal usage only
   static BOOL InternalCommand(int serverIndex, int command, void *inData, int inDataSize, void *outData, int outDataSize);
   static BOOL InternalCommand(const char* serverName, int command, void *inData, int inDataSize, void *outData, int outDataSize);

   static BOOL ControlCommand(int serverIndex, int command, void *inData, int inDataSize, void *outData, int outDataSize);
   static BOOL ControlCommand(const char* serverName, int command, void *inData, int inDataSize, void *outData, int outDataSize);

protected:
	BOOL m_bInitOK;            // TRUE if all handles were created successfully (derived classes only)

protected:
   static BOOL Open(BOOL isUserCall);
   static BOOL Close(BOOL isUserCall);
	static ServerType GetServerType(CORSERVER hServer);
	static BOOL GetServerSerialNumber(CORSERVER hServer, char *serialNumber);
	static BOOL SetResourceCount(int serverIndex, ResType resourceType);
	static BOOL SetResourceName(int serverIndex, ResType resourceType, int resourceIndex);
	static BOOL SetResourceName(SapLocation loc, ResType resourceType, CORHANDLE handle);
   static void DisplayMessage(const char *funcName, const char *message, const char *fileName, int lineNumber);
	static void DisplayMessageAndSave(char *message, const char *fileName, int lineNumber);
	static const char *FormatMessage(const char *formatStr, ...);
	static UINT WINAPI ResetThreadProc(LPVOID lpParameter);
	static BOOL GetInstallDirectory(CORSERVER hServer, char *installDir);
   static SAPSTATUS CCONV OnServerCallback(void *context, COREVENTINFO eventInfo);

protected:
	static SapInterface m_Interface;
	static SapResourceInfo m_ResourceInfo[MaxResourceTypes];
	static StatusMode m_DisplayStatusMode;

   static int m_ResetIndex;
   static int m_ResetTimeout;

   static SapManCallback m_ResetCallback;
   static void *m_ResetContext;

   static SapManCallback m_ErrorCallback;
   static void *m_ErrorContext;

   static EventType m_ServerEventType;
   static SapManCallback m_ServerCallback;
   static void *m_ServerContext;

   // Tracks usage of Open and Close methods from application code
   static BOOL m_UserOpenDone;

   // Tracks automatic usage of Open and Close through the SapManager constructor
   static int m_InternalOpenCount;
};

// Check status returned by a Sapera method; convert bad status into exception.
inline void SapTryCall(BOOL result)
{
   if (!result)
      throw std::runtime_error(SapManager::GetLastStatus());
}

//
// SapManVersionInfo class declaration
//
class SAPCLASSBASIC_CLASS SapManVersionInfo
{
public:
   enum LicenseType
   {
      Runtime = CORMAN_VAL_SAPERA_LICENSE_RUNTIME,
      Evaluation = CORMAN_VAL_SAPERA_LICENSE_EVAL,
      FullSDK = CORMAN_VAL_SAPERA_LICENSE_SDK
   };

public:
   SapManVersionInfo()
   {
      m_Major = 0;
      m_Minor = 0;
      m_Revision = 0;
      m_Build = 0;
      m_LicenseType = Runtime;
      m_EvalDaysRemaining = 0;
   }

   int GetMajor()               const { return m_Major; }
   int GetMinor()               const { return m_Minor; }
   int GetRevision()            const { return m_Revision; }
   int GetBuild()               const { return m_Build; }
   LicenseType GetLicenseType() const { return m_LicenseType; }
   int GetEvalDaysRemaining()   const { return m_EvalDaysRemaining; }

   void SetMajor(int major)       { m_Major = major; }
   void SetMinor(int minor)       { m_Minor = minor; }
   void SetRevision(int revision) { m_Revision = revision; }
   void SetBuild(int build)       { m_Build = build; }
   void SetLicenseType(LicenseType licenseType)     { m_LicenseType = licenseType; }
   void SetEvalDaysRemaining(int evalDaysRemaining) { m_EvalDaysRemaining = evalDaysRemaining; }

protected:
   int m_Major;
   int m_Minor;
   int m_Revision;
   int m_Build;
   LicenseType m_LicenseType;
   int m_EvalDaysRemaining;
};


//
// SapManCallbackInfo class declaration
//
class SAPCLASSBASIC_CLASS SapManCallbackInfo
{
public:
   enum MaxValues
   {
      MaxMessageSize = 255
   };

public:
   SapManCallbackInfo(int serverIndex, void *context)
   {
      Construct(SapManager::EventNone, serverIndex, 0, context, SapManager::StatusOk, NULL,0);
   }
   SapManCallbackInfo(SapManager::EventType eventType, int serverIndex, void *context)
   {
      Construct(eventType, serverIndex, 0, context, SapManager::StatusOk, NULL,0);
   }
   SapManCallbackInfo(SapManager::EventType eventType, int serverIndex, int resourceIndex, void *context)
   {
      Construct(eventType, serverIndex, resourceIndex, context, SapManager::StatusOk, NULL,0);
   }
   SapManCallbackInfo(SAPSTATUS errorValue, const char *errorMessage, void *context)
   {
      Construct(SapManager::EventNone, 0, 0, context, errorValue, errorMessage,0);
   }
   SapManCallbackInfo(SapManager::EventType eventType, int serverIndex, void *context, int filePercentProgress)
   {
      Construct(eventType, serverIndex, 0, context, SapManager::StatusOk, NULL, filePercentProgress);
   }
   ~SapManCallbackInfo() {}

	SapManager::EventType GetEventType() const { return m_EventType; }
	int GetServerIndex()          const { return m_ServerIndex; }
	int GetResourceIndex()        const { return m_ResourceIndex; }
   void *GetContext()            const { return m_Context; }
   SAPSTATUS GetErrorValue()     const { return m_ErrorValue; }
   const char *GetErrorMessage() const { return m_ErrorMessage; }
   int GetFilePercentProgress()  const { return m_FilePercentProgress; }
protected:
   void Construct(SapManager::EventType eventType, int serverIndex, int resourceIndex, void *context, SAPSTATUS errorValue, const char *errorMessage, int percentProgress);

protected:
   SapManager::EventType m_EventType;
	int m_ServerIndex;
	int m_ResourceIndex;
   void *m_Context;
   SAPSTATUS m_ErrorValue;
   char m_ErrorMessage[MaxMessageSize + 1];
   int m_FilePercentProgress;
};


//
// SapInterface class declaration
//
typedef SAPSTATUS (__stdcall *PGETCOUNT)( CORSERVER server, PUINT32 count);
typedef SAPSTATUS (__stdcall *PGETHANDLE)( CORSERVER server, UINT32 instance, CORHANDLE *pHandle);
typedef SAPSTATUS (__stdcall *PGETHANDLEREADONLY)( CORSERVER server, UINT32 instance, CORHANDLE *pHandle);
typedef SAPSTATUS (__stdcall *PRELEASE)( CORHANDLE handle);
typedef SAPSTATUS (__stdcall *PGETPRM)( CORHANDLE handle, UINT32 param, void *value);

class SAPCLASSBASIC_CLASS SapInterface
{
public:
	static PGETCOUNT m_GetCount[SapManager::MaxResourceTypes];
	static PGETHANDLE m_GetHandle[SapManager::MaxResourceTypes];
	static PGETHANDLEREADONLY m_GetHandleReadOnly[SapManager::MaxResourceTypes];
	static PRELEASE m_Release[SapManager::MaxResourceTypes];
	static PGETPRM m_GetPrm[SapManager::MaxResourceTypes];
	static int m_LabelPrm[SapManager::MaxResourceTypes];
};


//
// SapResourceInfo class declaration
//
class SAPCLASSBASIC_CLASS SapResourceInfo
{
public:
   SapResourceInfo();
   ~SapResourceInfo();

   int m_Counts[SapManager::MaxServers];
   char m_Labels[SapManager::MaxServers][SapManager::MaxResources][SapManager::MaxLabelSize];
   BOOL m_IsLabelInitOk[SapManager::MaxServers][SapManager::MaxResources];
};

#endif	// _SAPMANAGER_H_
