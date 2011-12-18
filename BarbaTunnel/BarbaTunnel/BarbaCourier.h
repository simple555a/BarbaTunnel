#pragma once
#include "BarbaSocket.h"
#include "SimpleEvent.h"
#include "SimpleSafeList.h"

#define BarbaCourier_MaxMessageLength  1600

//BarbaCourier
class BarbaCourier
{
protected:
	class Message
	{
	public:
		Message(BYTE* buffer, size_t count)
		{
			memcpy_s(this->Buffer, _countof(this->Buffer), buffer, count); 
			this->Count=count;
		}
		BYTE Buffer[BarbaCourier_MaxMessageLength];
		size_t Count;
	};

public:
	//@maxConnenction number of simultaneous connection for each outgoing and incoming, eg: 2 mean 2 connection for send and 2 connection for receive so the total will be 4
	explicit BarbaCourier(u_short maxConnenction, size_t threadsStackSize=128000);
	virtual void Send(BYTE* buffer, size_t bufferCount);
	virtual void Receive(BYTE* buffer, size_t bufferCount);
	void InitFakeRequests(LPCSTR httpGetTemplate, LPCSTR httpPostTemplate);
	size_t GetSentBytesCount() {return this->SentBytesCount;}
	size_t GetReceiveBytesCount() {return this->ReceiveBytesCount;}
	
	//Call this method to delete object, This method will signal all created thread to finish their job
	//This method will call asynchronously. do not use the object after call it 
	//@return the handle for deleting thread
	HANDLE Delete();

private:
	size_t MaxMessageBuffer;
	size_t SentBytesCount;
	size_t ReceiveBytesCount;
	SimpleEvent SendEvent;
	SimpleSafeList<Message*> Messages;
	static unsigned int __stdcall DeleteThread(void* object);

protected:
	virtual void Dispose();
	bool IsDisposing();
	virtual ~BarbaCourier(void);
	virtual void Send(Message* message, bool highPriority=false);
	void ProcessIncoming(BarbaSocket* barbaSocket);
	void ProcessOutgoing(BarbaSocket* barbaSocket);
	SimpleSafeList<BarbaSocket*> Sockets;
	SimpleSafeList<HANDLE> Threads;
	size_t MaxConnection;
	size_t ThreadsStackSize;
	SimpleEvent DisposeEvent;

	std::string FakeHttpGetTemplate;
	std::string FakeHttpPostTemplate;
};

//BarbaCourierServer
class BarbaCourierServer : public BarbaCourier
{
private:
	//used to pass data to created thread
	struct ServerThreadData
	{
		ServerThreadData(BarbaCourier* courier, BarbaSocket* socket, bool IsOutgoing) { this->Courier=courier; this->Socket=socket; this->IsOutgoing = IsOutgoing; }
		BarbaCourier* Courier;
		BarbaSocket* Socket;
		bool IsOutgoing;
	};

public:
	BarbaCourierServer(u_short maxConnenction);
	//@return false if no new connection accepted and caller should delete the socket
	//@return true if connection accepted, in this case caller should not delete the socket and it will be deleted automatically
	bool AddSocket(BarbaSocket* Socket, bool isOutgoing);

protected:
	virtual ~BarbaCourierServer(void);

private:
	void SendFakeReply(BarbaSocket* barbaSocket, bool postReply);
	static unsigned int __stdcall WorkerThread(void* serverThreadData);
};


//BarbaCourierClient
class BarbaCourierClient : public BarbaCourier
{
private:
	//used to pass data to created thread
	struct ClientThreadData
	{
		ClientThreadData(BarbaCourier* courier, bool isOutgoing) {this->Courier=courier; this->IsOutgoing = isOutgoing;}
		BarbaCourier* Courier;
		bool IsOutgoing;
	};

public:
	void SendFakeRequest(BarbaSocket* barbaSocket, bool isOutgoing);
	bool WaitForFakeReply(BarbaSocket* barbaSocket);
	BarbaCourierClient(DWORD remoteIp, u_short remotePort, u_short maxConnenction);

protected:
	virtual ~BarbaCourierClient();

private:
	DWORD RemoteIp;
	u_short RemotePort;
	static unsigned int __stdcall WorkerThread(void* clientThreadData);
};