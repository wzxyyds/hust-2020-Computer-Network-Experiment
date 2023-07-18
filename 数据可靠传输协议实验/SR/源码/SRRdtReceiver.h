#ifndef STOP_WAIT_RDT_RECEIVER_H
#define STOP_WAIT_RDT_RECEIVER_H
#include "RdtReceiver.h"
#include "SRConfig.h"
#include <deque>
#include <fstream>
class SRRdtReceiver :public RdtReceiver
{
private:
	ofstream f;
	bool isInWin(int seq);
	Packet lastAckPkt;				//上次发送的确认报文
	deque<Packet> rcvPkt;
	int maxSeq;
public:
	SRRdtReceiver();
	virtual ~SRRdtReceiver();

public:
	
	void receive(const Packet &packet);	//接收报文，将被NetworkService调用
};

#endif

