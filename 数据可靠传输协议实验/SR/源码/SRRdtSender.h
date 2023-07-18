#ifndef STOP_WAIT_RDT_SENDER_H
#define STOP_WAIT_RDT_SENDER_H
#include "RdtSender.h"
#include "SRConfig.h"
#include <deque>
#include <fstream>
class SRRdtSender :public RdtSender
{
private:
	ofstream f;
	bool waitingState;				// 是否处于等待Ack的状态
	deque<Packet> sndpkt; //缓冲区
	int winSize; //窗口大小
	int maxSeq;
	int nextSeqnum;
public:

	bool getWaitingState();
	bool send(const Message& message);						//发送应用层下来的Message，由NetworkServiceSimulator调用,如果发送方成功地将Message发送到网络层，返回true;如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	void receive(const Packet& ackPkt);						//接受确认Ack，将被NetworkServiceSimulator调用	
	void timeoutHandler(int seqNum);					//Timeout handler，将被NetworkServiceSimulator调用

public:
	SRRdtSender();
	virtual ~SRRdtSender();
};

#endif

