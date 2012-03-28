// kommando knoten  für zdsp1 hardware server
// es werden alle kommando knoten für zhserver nodes,
// scpi nodes, commands und , queries für den 
// bearbeitenden kommando interpreter definiert

#include "scpi.h" // für scpi knoten
#include "dsp1scpi.h" // scpi erweiterung

// die vollständige scpi kommando liste

cNodeSCPI* System;
                   cNodeSCPI* SystemVersion;
		           cNodeSCPI* SystemVersionServer;
		           cNodeSCPI* SystemVersionDevice;	   
	     cNodeSCPI* SystemDebug;	   
                   cNodeSCPI* SystemCommunication;
		          cNodeSCPI* SystemCommunicationDeviceNode;
		          cNodeSCPI* SystemCommunicationEncryption;	  
	     cNodeSCPI* SystemDsp;
	                        cNodeSCPI* SystemDspReset;
	                        cNodeSCPI* SystemDspBoot;
				cNodeSCPI* SystemDspBootPath;
				cNodeSCPI* SystemDspBootExecute;
		          cNodeSCPI* SystemDspSampling;	
		          cNodeSCPI* SystemDspMaxima;
			              cNodeSCPI* SystemDspMaximaReset;
		          cNodeSCPI* SystemDspTrigger;
			              cNodeSCPI* SystemDspTriggerIntList;
				                   cNodeSCPI* SystemDspTriggerALL;
			          	                   cNodeSCPI* SystemDspTriggerHKSK;
		          cNodeSCPI* SystemDspEN61850;
			                                 cNodeSCPI* SystemDspEN61850SyncLostCount;
					     cNodeSCPI* SystemDspEN61850DataCount;
					     cNodeSCPI* SystemDspEN61850Mac;
					                         cNodeSCPI* SystemDspEN61850MacSAdress;
						           cNodeSCPI* SystemDspEN61850MacDAdress;
					     cNodeSCPI* SystemDspEN61850PriorityTagged;		
					     cNodeSCPI* SystemDspEN61850EthTypeAppId;
					     cNodeSCPI* SystemDspEN61850EthSync;		   
		          cNodeSCPI* SystemDspCommand;
			              cNodeSCPI* SystemDspCommandStat;
	     cNodeSCPI* SystemSerNr;
	     
	                        	     
cNodeSCPI* Status;
                   cNodeSCPI* StatusDevice;
	     cNodeSCPI* StatusDsp;	   
		          cNodeSCPI* StatusDspLoad;	   
	                                           cNodeSCPI* StatusDspLoadActual;
		                             cNodeSCPI* StatusDspLoadMaximum;
			                                  cNodeSCPI* StatusDspLoadMaximumReset;
	     
cNodeSCPI* MeasureNode;
                   cNodeSCPI* MeasureList;
		          cNodeSCPI* MeasureListCycList;
		          cNodeSCPI* MeasureListIntList;
		          cNodeSCPI* MeasureListRavList;
		          cNodeSCPI* MeasureListSet;
		          cNodeSCPI* MeasureListClear;	  
	     cNodeSCPI* MeasureInitiate; 
	     cNodeSCPI* MeasureFetch;
	     
	     
cNodeSCPI* Memory;	     
	     cNodeSCPI* MemoryRead;
	     cNodeSCPI* MemoryWrite;
	     
	     
// cNodeScpi (QString,tNodeSpec,cNode*,cNode*,SCPICmdType,SCPICmdType); 
// konstruktor, sNodeName, nNodedef, pNextNode, pNewLevelNode, Cmd, Query				
// konstruktor, psNodeNames,psNode2Set, nNodedef, pNextNode, pNewLevelNode, Cmd, Query
//cNodeZHServer::cNodeZHServer(QStringList* sl,QString* s,tNodeSpec ns,cNode* n1,cNode* n2,SCPICmdType,SCPICmdType)
	       
cNode* InitCmdTree() {
    
    // implementiertes memory modell
    
    MemoryWrite=new cNodeSCPI("WRITE",isCommand,NULL,NULL,DspMemoryWrite,nixCmd);	     
    MemoryRead=new cNodeSCPI("READ",isCommand,MemoryWrite,NULL,DspMemoryRead,nixCmd);	     
    Memory=new cNodeSCPI("MEMORY",isNode,NULL,MemoryRead,nixCmd,nixCmd);	     

    // implementiertes measure modell

    MeasureFetch=new cNodeSCPI("FETCH",isCommand,NULL,NULL,Fetch,nixCmd);    
    MeasureInitiate=new cNodeSCPI("INITIATE",isCommand,MeasureFetch,NULL,Initiate,nixCmd);
    MeasureListClear=new cNodeSCPI("CLEAR",isCommand,NULL,NULL,UnloadCmdList,nixCmd);
    MeasureListSet=new cNodeSCPI("SET",isCommand,MeasureListClear,NULL,LoadCmdList,nixCmd);
    MeasureListRavList=new cNodeSCPI("RAVLIST",isQuery | isCommand,MeasureListSet,NULL,SetRavList,GetRavList);
    MeasureListIntList=new cNodeSCPI("INTLIST",isQuery | isCommand,MeasureListRavList,NULL,SetCmdIntList,GetCmdIntList);
    MeasureListCycList=new cNodeSCPI("CYCLIST",isQuery | isCommand,MeasureListIntList,NULL,SetCmdList,GetCmdList);
    MeasureList=new cNodeSCPI("LIST",isNode,MeasureInitiate,MeasureListCycList,nixCmd,nixCmd);
    MeasureNode=new cNodeSCPI("MEASURE",isNode | isCommand,Memory,MeasureList,Measure,nixCmd);	     
    // implementiertes status modell
    
    StatusDspLoadMaximumReset=new cNodeSCPI("RESET",isCommand,NULL,NULL,ResetDeviceLoadMax,nixCmd);
    StatusDspLoadMaximum=new cNodeSCPI("MAXIMUM",isNode | isQuery,NULL,StatusDspLoadMaximumReset,nixCmd,GetDeviceLoadMax);	
    StatusDspLoadActual=new cNodeSCPI("ACTUAL",isQuery,StatusDspLoadMaximum,NULL,nixCmd,GetDeviceLoadAct);
    StatusDspLoad=new cNodeSCPI("LOAD",isNode,NULL,StatusDspLoadActual,nixCmd,nixCmd);
    StatusDsp=new cNodeSCPI("DSP",isNode | isQuery,NULL,StatusDspLoad,nixCmd,GetDspStatus);
    StatusDevice=new cNodeSCPI("DEVICE",isQuery,StatusDsp,NULL,nixCmd,GetDeviceStatus);
    Status=new cNodeSCPI("STATUS",isNode,MeasureNode,StatusDevice,nixCmd,nixCmd);
    
    // implementiertes system modell
  
    SystemSerNr=new cNodeSCPI("SERNR",isQuery,NULL,NULL,nixCmd,GetPCBSerialNumber);
    SystemDspCommandStat=new cNodeSCPI("STAT",isQuery | isCommand,NULL,NULL,SetDspCommandStat,GetDspCommandStat);      
    SystemDspCommand=new cNodeSCPI("COMMAND",isNode,NULL,SystemDspCommandStat,nixCmd,nixCmd);
    
    
    SystemDspEN61850EthSync=new cNodeSCPI("ETHSYNC",isQuery | isCommand,NULL,NULL,SetEN61850EthSync,GetEN61850EthSync);
    SystemDspEN61850EthTypeAppId=new cNodeSCPI("ETHTYPEAPPID",isQuery | isCommand,SystemDspEN61850EthSync,NULL,SetEN61850EthTypeAppId,GetEN61850EthTypeAppId);
    SystemDspEN61850PriorityTagged=new cNodeSCPI("PRIORITYTAGGED",isQuery | isCommand,SystemDspEN61850EthTypeAppId,NULL,SetEN61850PriorityTagged,GetEN61850PriorityTagged);
    SystemDspEN61850MacDAdress=new cNodeSCPI("DADRESS",isQuery | isCommand,NULL,NULL,SetEN61850DestAdr,GetEN61850DestAdr);
    SystemDspEN61850MacSAdress=new cNodeSCPI("SADRESS",isQuery | isCommand,SystemDspEN61850MacDAdress,NULL,SetEN61850SourceAdr,GetEN61850SourceAdr);
    SystemDspEN61850Mac=new cNodeSCPI("MAC",isNode,SystemDspEN61850PriorityTagged,SystemDspEN61850MacSAdress,nixCmd,nixCmd);
    SystemDspEN61850DataCount=new cNodeSCPI("DATCOUNT",isQuery | isCommand,SystemDspEN61850Mac,NULL,SetEN61850DataCount,GetEN61850DataCount);
    SystemDspEN61850SyncLostCount=new cNodeSCPI("SNLCOUNT",isQuery | isCommand,SystemDspEN61850DataCount,NULL,SetEN61850SyncLostCount,GetEN61850SyncLostCount);
    SystemDspEN61850=new cNodeSCPI("EN61850",isNode,SystemDspCommand,SystemDspEN61850SyncLostCount,nixCmd,nixCmd);
    SystemDspTriggerHKSK=new cNodeSCPI("HKSK",isCommand,NULL,NULL,TriggerIntListHKSK,nixCmd);	      
    SystemDspTriggerALL=new cNodeSCPI("ALL",isCommand,SystemDspTriggerHKSK,NULL,TriggerIntListALL,nixCmd);
    SystemDspTriggerIntList=new cNodeSCPI("INTLIST",isNode,NULL,SystemDspTriggerALL,nixCmd,nixCmd);
    SystemDspTrigger=new cNodeSCPI("TRIGGER",isNode,SystemDspEN61850,SystemDspTriggerIntList,nixCmd,nixCmd);	  
    SystemDspMaximaReset=new cNodeSCPI("RESET",isCommand,NULL,NULL,ResetMaxima,nixCmd);
    SystemDspMaxima=new cNodeSCPI("MAXIMA",isNode,SystemDspTrigger,SystemDspMaximaReset,nixCmd,nixCmd);
    SystemDspSampling=new cNodeSCPI("SAMPLING",isQuery | isCommand,SystemDspMaxima,NULL,SetSamplingSystem,GetSamplingSystem);
    SystemDspBootExecute=new cNodeSCPI("EXECUTE",isCommand,NULL,NULL,BootDsp,nixCmd);
    SystemDspBootPath=new cNodeSCPI("PATH",isQuery | isCommand,SystemDspBootExecute,NULL,SetDspBootPath,GetDspBootPath);
    SystemDspBoot=new cNodeSCPI("BOOT",isNode,SystemDspSampling,SystemDspBootPath,nixCmd,nixCmd);
    SystemDspReset=new cNodeSCPI("RESET",isCommand,SystemDspBoot,NULL,ResetDsp,nixCmd);
    SystemDsp=new cNodeSCPI("DSP",isNode,SystemSerNr,SystemDspReset,nixCmd,nixCmd);
    SystemCommunicationEncryption=new cNodeSCPI("ENCRYPTION",isQuery | isCommand,NULL,NULL,SetCommEncryption,GetCommEncryption);
    SystemCommunicationDeviceNode=new cNodeSCPI("DEVNODE",isQuery,SystemCommunicationEncryption,NULL,nixCmd,GetDspDeviceNode);
    SystemCommunication=new cNodeSCPI("COMMUNICATION",isNode,SystemDsp,SystemCommunicationDeviceNode,nixCmd,nixCmd);
    SystemDebug=new cNodeSCPI("DEBUG",isQuery | isCommand,SystemCommunication,NULL,eSetDebugLevel,GetDebugLevel);
    SystemVersionDevice=new cNodeSCPI("DEVICE",isQuery,NULL,NULL,nixCmd,GetDeviceVersion);
    SystemVersionServer=new cNodeSCPI("SERVER",isQuery,SystemVersionDevice,NULL,nixCmd,GetServerVersion);  
    SystemVersion=new cNodeSCPI("VERSION",isNode,SystemDebug,SystemVersionServer,nixCmd,nixCmd);	         
    System=new cNodeSCPI("SYSTEM",isNode,Status,SystemVersion,nixCmd,nixCmd);
    return (System);  
}
