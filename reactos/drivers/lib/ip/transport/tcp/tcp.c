/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/tcp.c
 * PURPOSE:     Transmission Control Protocol
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

LONG TCP_IPIdentification = 0;
static BOOLEAN TCPInitialized = FALSE;
static NPAGED_LOOKASIDE_LIST TCPSegmentList;
LIST_ENTRY SignalledConnections;
LIST_ENTRY SleepingThreadsList;
FAST_MUTEX SleepingThreadsLock;
RECURSIVE_MUTEX TCPLock;

static VOID HandleSignalledConnection( PCONNECTION_ENDPOINT Connection,
				       ULONG NewState ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PTCP_COMPLETION_ROUTINE Complete;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    BOOLEAN CompletedOne = FALSE;

    /* Things that can happen when we try the initial connection */
    if( ((NewState & SEL_CONNECT) || (NewState & SEL_FIN)) &&

	!(Connection->State & (SEL_CONNECT | SEL_FIN)) ) {
	while( !IsListEmpty( &Connection->ConnectRequest ) ) {
	    Connection->State |= NewState & (SEL_CONNECT | SEL_FIN);
	    Entry = RemoveHeadList( &Connection->ConnectRequest );
	    Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
	    Complete = Bucket->Request.RequestNotifyObject;
	    TI_DbgPrint(DEBUG_TCP,
			("Completing Connect Request %x\n", Bucket->Request));
	    if( NewState & SEL_FIN ) Status = STATUS_CONNECTION_REFUSED;
	    Complete( Bucket->Request.RequestContext, Status, 0 );
	    /* Frees the bucket allocated in TCPConnect */
	    PoolFreeBuffer( Bucket );
	}
    }

    /* Things that happen after we're connected */
    if( (NewState & SEL_READ) ) {
	TI_DbgPrint(DEBUG_TCP,("Readable: irp list %s\n",
			       IsListEmpty(&Connection->ReceiveRequest) ?
			       "empty" : "nonempty"));

	while( !IsListEmpty( &Connection->ReceiveRequest ) ) {
	    PIRP Irp;
	    OSK_UINT RecvLen = 0, Received = 0;
	    OSK_PCHAR RecvBuffer = 0;
	    PMDL Mdl;
	    NTSTATUS Status;

	    Entry = RemoveHeadList( &Connection->ReceiveRequest );
	    Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
	    Complete = Bucket->Request.RequestNotifyObject;

	    TI_DbgPrint(DEBUG_TCP,
			("Readable, Completing read request %x\n", 
			 Bucket->Request));

	    Irp = Bucket->Request.RequestContext;
	    Mdl = Irp->MdlAddress;

	    TI_DbgPrint(DEBUG_TCP,
			("Getting the user buffer from %x\n", Mdl));

	    NdisQueryBuffer( Mdl, &RecvBuffer, &RecvLen );

	    TI_DbgPrint(DEBUG_TCP,
			("Reading %d bytes to %x\n", RecvLen, RecvBuffer));

	    TI_DbgPrint(DEBUG_TCP, ("Connection: %x\n", Connection));
	    TI_DbgPrint
		(DEBUG_TCP, 
		 ("Connection->SocketContext: %x\n", 
		  Connection->SocketContext));
	    TI_DbgPrint(DEBUG_TCP, ("RecvBuffer: %x\n", RecvBuffer));
	    
	    Status = TCPTranslateError
		( OskitTCPRecv( Connection->SocketContext,
				RecvBuffer,
				RecvLen,
				&Received,
				0 ) );

	    TI_DbgPrint(DEBUG_TCP,("TCP Bytes: %d\n", Received));
	    
	    if( Status == STATUS_SUCCESS ) {
		TI_DbgPrint(DEBUG_TCP,("Received %d bytes with status %x\n",
				       Received, Status));
		
		TI_DbgPrint(DEBUG_TCP,
			    ("Completing Receive Request: %x\n", 
			     Bucket->Request));

		Complete( Bucket->Request.RequestContext,
			  STATUS_SUCCESS, Received );
		CompletedOne = TRUE;
	    } else if( Status == STATUS_PENDING ) {
		InsertHeadList( &Connection->ReceiveRequest,
				&Bucket->Entry );
		break;
	    } else {
		TI_DbgPrint(DEBUG_TCP,
			    ("Completing Receive request: %x %x\n",
			     Bucket->Request, Status));
		Complete( Bucket->Request.RequestContext, Status, 0 );
		CompletedOne = TRUE;
	    }
	}
    }
    if( NewState & SEL_FIN ) {
	TI_DbgPrint(DEBUG_TCP, ("EOF From socket\n"));
	
	while( !IsListEmpty( &Connection->ReceiveRequest ) ) {
	    Entry = RemoveHeadList( &Connection->ReceiveRequest );
	    Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
	    Complete = Bucket->Request.RequestNotifyObject;

	    Complete( Bucket->Request.RequestContext, STATUS_SUCCESS, 0 );
	}
    }

    Connection->Signalled = FALSE;
}

VOID DrainSignals() {
    PCONNECTION_ENDPOINT Connection;
    PLIST_ENTRY ListEntry;

    while( !IsListEmpty( &SignalledConnections ) ) {
	ListEntry = RemoveHeadList( &SignalledConnections );
	Connection = CONTAINING_RECORD( ListEntry, CONNECTION_ENDPOINT,
					SignalList );
	HandleSignalledConnection( Connection, Connection->SignalState );
    }
}

PCONNECTION_ENDPOINT TCPAllocateConnectionEndpoint( PVOID ClientContext ) {
    PCONNECTION_ENDPOINT Connection = 
	ExAllocatePool(NonPagedPool, sizeof(CONNECTION_ENDPOINT));
    if (!Connection)
	return Connection;
    
    TI_DbgPrint(DEBUG_CPOINT, ("Connection point file object allocated at (0x%X).\n", Connection));
    
    RtlZeroMemory(Connection, sizeof(CONNECTION_ENDPOINT));
    
    /* Initialize spin lock that protects the connection endpoint file object */
    TcpipInitializeSpinLock(&Connection->Lock);
    InitializeListHead(&Connection->ConnectRequest);
    InitializeListHead(&Connection->ListenRequest);
    InitializeListHead(&Connection->ReceiveRequest);
    
    /* Save client context pointer */
    Connection->ClientContext = ClientContext;
    
    return Connection;
}

VOID TCPFreeConnectionEndpoint( PCONNECTION_ENDPOINT Connection ) {
    TI_DbgPrint(MAX_TRACE,("FIXME: Cancel all pending requests\n"));
    /* XXX Cancel all pending requests */
    ExFreePool( Connection );
}

NTSTATUS TCPSocket( PCONNECTION_ENDPOINT Connection, 
		    UINT Family, UINT Type, UINT Proto ) {
    NTSTATUS Status;

    TI_DbgPrint(DEBUG_TCP,("Called: Connection %x, Family %d, Type %d, "
			   "Proto %d\n",
			   Connection, Family, Type, Proto));

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );
    Status = TCPTranslateError( OskitTCPSocket( Connection,
						&Connection->SocketContext,
						Family,
						Type,
						Proto ) );

    ASSERT_KM_POINTER(Connection->SocketContext);

    TI_DbgPrint(DEBUG_TCP,("Connection->SocketContext %x\n",
			   Connection->SocketContext));

    TcpipRecursiveMutexLeave( &TCPLock );

    return Status;
}

VOID TCPReceive(PIP_INTERFACE Interface, PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives and queues TCP data
 * ARGUMENTS:
 *     IPPacket = Pointer to an IP packet that was received
 * NOTES:
 *     This is the low level interface for receiving TCP data
 */
{
    TI_DbgPrint(DEBUG_TCP,("Sending packet %d (%d) to oskit\n", 
			   IPPacket->TotalSize,
			   IPPacket->HeaderSize));

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    OskitTCPReceiveDatagram( IPPacket->Header, 
			     IPPacket->TotalSize, 
			     IPPacket->HeaderSize );

    DrainSignals();

    TcpipRecursiveMutexLeave( &TCPLock );
}

/* event.c */
int TCPSocketState( void *ClientData,
		    void *WhichSocket,
		    void *WhichConnection,
		    OSK_UINT NewState );

int TCPPacketSend( void *ClientData,
		   OSK_PCHAR Data,
		   OSK_UINT Len );

POSK_IFADDR TCPFindInterface( void *ClientData,
			      OSK_UINT AddrType,
			      OSK_UINT FindType,
			      OSK_SOCKADDR *ReqAddr );

void *TCPMalloc( void *ClientData,
		 OSK_UINT bytes, OSK_PCHAR file, OSK_UINT line );
void TCPFree( void *ClientData,
	      void *data, OSK_PCHAR file, OSK_UINT line );

int TCPSleep( void *ClientData, void *token, int priority, char *msg,
	      int tmio );

void TCPWakeup( void *ClientData, void *token );

OSKITTCP_EVENT_HANDLERS EventHandlers = {
    NULL,             /* Client Data */
    TCPSocketState,   /* SocketState */
    TCPPacketSend,    /* PacketSend */
    TCPFindInterface, /* FindInterface */
    TCPMalloc,        /* Malloc */
    TCPFree,          /* Free */
    TCPSleep,         /* Sleep */
    TCPWakeup         /* Wakeup */
};

NTSTATUS TCPStartup(VOID)
/*
 * FUNCTION: Initializes the TCP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    TcpipRecursiveMutexInit( &TCPLock );
    ExInitializeFastMutex( &SleepingThreadsLock );
    InitializeListHead( &SleepingThreadsList );    
    InitializeListHead( &SignalledConnections );

    RegisterOskitTCPEventHandlers( &EventHandlers );
    InitOskitTCP();
    
    /* Register this protocol with IP layer */
    IPRegisterProtocol(IPPROTO_TCP, TCPReceive);
    
    ExInitializeNPagedLookasideList(
	&TCPSegmentList,                /* Lookaside list */
	NULL,                           /* Allocate routine */
	NULL,                           /* Free routine */
	0,                              /* Flags */
	sizeof(TCP_SEGMENT),            /* Size of each entry */
	TAG('T','C','P','S'),           /* Tag */
	0);                             /* Depth */
    
    TCPInitialized = TRUE;
    
    return STATUS_SUCCESS;
}


NTSTATUS TCPShutdown(VOID)
/*
 * FUNCTION: Shuts down the TCP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    if (!TCPInitialized)
	return STATUS_SUCCESS;
    
    /* Deregister this protocol with IP layer */
    IPRegisterProtocol(IPPROTO_TCP, NULL);
    
    ExDeleteNPagedLookasideList(&TCPSegmentList);
    
    TCPInitialized = FALSE;

    DeinitOskitTCP();

    return STATUS_SUCCESS;
}

NTSTATUS TCPTranslateError( int OskitError ) {
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    switch( OskitError ) {
    case 0: Status = STATUS_SUCCESS; break;
    case OSK_EADDRNOTAVAIL:
    case OSK_EAFNOSUPPORT: Status = STATUS_INVALID_CONNECTION; break;
    case OSK_ECONNREFUSED:
    case OSK_ECONNRESET: Status = STATUS_REMOTE_NOT_LISTENING; break;
    case OSK_EINPROGRESS:
    case OSK_EAGAIN: Status = STATUS_PENDING; break;
    default: Status = STATUS_INVALID_CONNECTION; break;
    }

    TI_DbgPrint(DEBUG_TCP,("Error %d -> %x\n", OskitError, Status));
    return Status;
}

#if 0
NTSTATUS TCPBind
( PCONNECTION_ENDPOINT Connection,
  PTDI_CONNECTION_INFORMATION ConnInfo ) {
    NTSTATUS Status;
    SOCKADDR_IN AddressToConnect;
    PIP_ADDRESS LocalAddress;
    USHORT LocalPort;

    TI_DbgPrint(DEBUG_TCP,("Called\n"));

    Status = AddrBuildAddress
	((PTA_ADDRESS)ConnInfo->LocalAddress,
	 &LocalAddress,
	 &LocalPort);

    AddressToBind.sin_family = AF_INET;
    memcpy( &AddressToBind.sin_addr, 
	    &LocalAddress->Address.IPv4Address,
	    sizeof(AddressToBind.sin_addr) );
    AddressToBind.sin_port = LocalPort;

    Status = OskitTCPBind( Connection->SocketContext,
			   Connection,
			   &AddressToBind, 
			   sizeof(AddressToBind));

    TI_DbgPrint(DEBUG_TCP,("Leaving %x\n", Status));

    return Status;
}
#endif

NTSTATUS TCPConnect
( PCONNECTION_ENDPOINT Connection,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context ) {
    NTSTATUS Status;
    SOCKADDR_IN AddressToConnect = { 0 }, AddressToBind = { 0 };
    IP_ADDRESS RemoteAddress;
    USHORT RemotePort;
    PTDI_BUCKET Bucket;

    DbgPrint("TCPConnect: Called\n");

    Bucket = ExAllocatePool( NonPagedPool, sizeof(*Bucket) );
    if( !Bucket ) return STATUS_NO_MEMORY;

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    /* Freed in TCPSocketState */
    Bucket->Request.RequestNotifyObject = (PVOID)Complete;
    Bucket->Request.RequestContext = Context;

    InsertHeadList( &Connection->ConnectRequest, &Bucket->Entry );

    Status = AddrBuildAddress
	((PTRANSPORT_ADDRESS)ConnInfo->RemoteAddress,
	 &RemoteAddress,
	 &RemotePort);

    DbgPrint("Connecting to address %x:%x\n",
	     RemoteAddress.Address.IPv4Address,
	     RemotePort);

    if (!NT_SUCCESS(Status)) {
	TI_DbgPrint(DEBUG_TCP, ("Could not AddrBuildAddress in TCPConnect\n"));
	return Status;
    }
    
    AddressToConnect.sin_family = AF_INET;
    AddressToBind = AddressToConnect;

    OskitTCPBind( Connection->SocketContext,
		  Connection,
		  &AddressToBind,
		  sizeof(AddressToBind) );

    memcpy( &AddressToConnect.sin_addr, 
	    &RemoteAddress.Address.IPv4Address,
	    sizeof(AddressToConnect.sin_addr) );
    AddressToConnect.sin_port = RemotePort;

    Status = TCPTranslateError
	( OskitTCPConnect( Connection->SocketContext,
			   Connection,
			   &AddressToConnect, 
			   sizeof(AddressToConnect) ) );

    TcpipRecursiveMutexLeave( &TCPLock );
    
    if( Status == OSK_EINPROGRESS || Status == STATUS_SUCCESS ) 
	return STATUS_PENDING;
    else
	return Status;
}

NTSTATUS TCPDisconnect
( PCONNECTION_ENDPOINT Connection,
  UINT Flags,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PTDI_CONNECTION_INFORMATION ReturnInfo,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context ) {
    NTSTATUS Status;
    
    TI_DbgPrint(DEBUG_TCP,("started\n"));

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    switch( Flags & (TDI_DISCONNECT_ABORT | TDI_DISCONNECT_RELEASE) ) {
    case 0:
    case TDI_DISCONNECT_ABORT:
	Flags = 0;
	break;

    case TDI_DISCONNECT_ABORT | TDI_DISCONNECT_RELEASE:
	Flags = 2;
	break;

    case TDI_DISCONNECT_RELEASE:
	Flags = 1;
	break;
    }

    Status = TCPTranslateError
	( OskitTCPShutdown( Connection->SocketContext, Flags ) );

    TcpipRecursiveMutexLeave( &TCPLock );
    
    TI_DbgPrint(DEBUG_TCP,("finished %x\n", Status));

    return Status;
}

NTSTATUS TCPClose
( PCONNECTION_ENDPOINT Connection ) {
    NTSTATUS Status;
    
    TI_DbgPrint(DEBUG_TCP,("TCPClose started\n"));

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    Status = TCPTranslateError( OskitTCPClose( Connection->SocketContext ) );

    if( Connection->Signalled ) 
	RemoveEntryList( &Connection->SignalList );

    TcpipRecursiveMutexLeave( &TCPLock );
    
    TI_DbgPrint(DEBUG_TCP,("TCPClose finished %x\n", Status));

    return Status;
}

NTSTATUS TCPListen
( PCONNECTION_ENDPOINT Connection,
  UINT Backlog, 
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context) {
   NTSTATUS Status;

   TI_DbgPrint(DEBUG_TCP,("TCPListen started\n"));

   TI_DbgPrint(DEBUG_TCP,("Connection->SocketContext %x\n",
     Connection->SocketContext));

   ASSERT(Connection);
   ASSERT_KM_POINTER(Connection->SocketContext);

   TcpipRecursiveMutexEnter( &TCPLock, TRUE );
   
   Status =  TCPTranslateError( OskitTCPListen( Connection->SocketContext,
						Backlog ) );
   
   TcpipRecursiveMutexLeave( &TCPLock );

   TI_DbgPrint(DEBUG_TCP,("TCPListen finished %x\n", Status));
   
   return Status;
}

NTSTATUS TCPAccept
( PTDI_REQUEST Request,
  VOID **NewSocketContext ) {
   NTSTATUS Status;

   TI_DbgPrint(DEBUG_TCP,("TCPAccept started\n"));
   Status = STATUS_UNSUCCESSFUL;
   TI_DbgPrint(DEBUG_TCP,("TCPAccept finished %x\n", Status));
   return Status;
}

NTSTATUS TCPReceiveData
( PCONNECTION_ENDPOINT Connection,
  PNDIS_BUFFER Buffer,
  ULONG ReceiveLength,
  PULONG BytesReceived,
  ULONG ReceiveFlags,
  PTCP_COMPLETION_ROUTINE Complete,
  PVOID Context ) {
    PCHAR DataBuffer;
    UINT DataLen, Received = 0;
    NTSTATUS Status;
    PTDI_BUCKET Bucket;

    TI_DbgPrint(DEBUG_TCP,("Called for %d bytes\n", ReceiveLength));

    ASSERT_KM_POINTER(Connection->SocketContext);

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    NdisQueryBuffer( Buffer, &DataBuffer, &DataLen );

    TI_DbgPrint(DEBUG_TCP,("TCP>|< Got an MDL %x (%x:%d)\n", Buffer, DataBuffer, DataLen));

    Status = TCPTranslateError
	( OskitTCPRecv
	  ( Connection->SocketContext,
	    DataBuffer,
	    DataLen,
	    &Received,
	    ReceiveFlags ) );
    
    TI_DbgPrint(DEBUG_TCP,("OskitTCPReceive: %x, %d\n", Status, Received));

    /* Keep this request around ... there was no data yet */
    if( Status == STATUS_PENDING ) {
	/* Freed in TCPSocketState */
	Bucket = ExAllocatePool( NonPagedPool, sizeof(*Bucket) );
	if( !Bucket ) {
	    TI_DbgPrint(DEBUG_TCP,("Failed to allocate bucket\n"));
	    TcpipRecursiveMutexLeave( &TCPLock );
	    return STATUS_NO_MEMORY;
	}
	
	Bucket->Request.RequestNotifyObject = Complete;
	Bucket->Request.RequestContext = Context;
	*BytesReceived = 0;

	InsertHeadList( &Connection->ReceiveRequest, &Bucket->Entry );
	Status = STATUS_PENDING;
	TI_DbgPrint(DEBUG_TCP,("Queued read irp\n"));
    } else {
	TI_DbgPrint(DEBUG_TCP,("Got status %x, bytes %d\n", Status, Received));
	*BytesReceived = Received;
    }

    TcpipRecursiveMutexLeave( &TCPLock );

    TI_DbgPrint(DEBUG_TCP,("Status %x\n", Status));

    return Status;
}

NTSTATUS TCPSendData
( PCONNECTION_ENDPOINT Connection,
  PCHAR BufferData,
  ULONG PacketSize,
  PULONG DataUsed,
  ULONG Flags) {
    NTSTATUS Status;

    ASSERT_KM_POINTER(Connection->SocketContext);

    TcpipRecursiveMutexEnter( &TCPLock, TRUE );

    TI_DbgPrint(DEBUG_TCP,("Connection = %x\n", Connection));
    TI_DbgPrint(DEBUG_TCP,("Connection->SocketContext = %x\n",
			   Connection->SocketContext));

    Status = OskitTCPSend( Connection->SocketContext, 
			   BufferData, PacketSize, (PUINT)DataUsed, 0 );

    TcpipRecursiveMutexLeave( &TCPLock );

    return Status;
}

VOID TCPTimeout(VOID) { 
    static int Times = 0;
    TcpipRecursiveMutexEnter( &TCPLock, TRUE );
    if( (Times++ % 5) == 0 ) {
	TimerOskitTCP();
    }
    DrainSignals();
    TcpipRecursiveMutexLeave( &TCPLock );
}

/* EOF */
