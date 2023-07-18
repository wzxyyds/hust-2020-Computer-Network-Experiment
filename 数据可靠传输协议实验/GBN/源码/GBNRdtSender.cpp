#include "stdafx.h"
#include "Global.h"
#include "GBNRdtSender.h"

#define WIN_SIZE 4
#define SEQ_BIT 3

GBNRdtSender::GBNRdtSender() : nextSeqnum(0), waitingState(false), winSize(WIN_SIZE), maxSeq(1 << SEQ_BIT), base(0)
{
	f.open("D:\\hust\\大三\\计网\\实验二\\Exp2\\Exp2\\GBN\\senderWindow.txt");
}


GBNRdtSender::~GBNRdtSender()
{
	f.close();
}



bool GBNRdtSender::getWaitingState() {
	return waitingState;
}




bool GBNRdtSender::send(const Message& message) {
	if (this->waitingState) { //发送方处于等待确认状态
		return false;
	}
	Packet packetWaitingACK;
	packetWaitingACK.acknum = -1; //忽略该字段
	packetWaitingACK.seqnum = this->nextSeqnum;
	packetWaitingACK.checksum = 0;
	memcpy(packetWaitingACK.payload, message.data, sizeof(message.data));
	packetWaitingACK.checksum = pUtils->calculateCheckSum(packetWaitingACK);
	this->sndpkt.push_back(packetWaitingACK);
	pUtils->printPacket("发送方发送报文", this->sndpkt.back());
	if (base == nextSeqnum)
		pns->startTimer(SENDER, Configuration::TIME_OUT, 0);			//启动发送方定时器
	pns->sendToNetworkLayer(RECEIVER, this->sndpkt.back());								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	if (this->sndpkt.size() >= this->winSize) {
		this->waitingState = true;																	//进入等待状态
	}
	this->nextSeqnum = (this->nextSeqnum + 1) % this->maxSeq;
	return true;
}

void GBNRdtSender::receive(const Packet& ackPkt) {
	if (!this->sndpkt.empty()) {
		//检查校验和是否正确
		int checkSum = pUtils->calculateCheckSum(ackPkt);

		//如果校验和正确，并且确认序号=发送方已发送并等待确认的数据包序号
		if (checkSum == ackPkt.checksum) {
			pns->stopTimer(SENDER, 0);		//关闭定时器
			this->base = (ackPkt.acknum + 1) % this->maxSeq;
			pUtils->printPacket("发送方正确收到确认", ackPkt);
			if (base != nextSeqnum) {
				pns->startTimer(SENDER, Configuration::TIME_OUT, 0);			//启动发送方定时器
			}
			while (!sndpkt.empty() && sndpkt.front().seqnum != base) {
				cout << "滑动前窗口中报文序号为：";
				f << "滑动前窗口中报文序号为：";
				for (auto p : sndpkt) {
					f << p.seqnum << " ";
					cout << p.seqnum << " ";
				}
				cout << endl;
				f << endl;
				int newbase = (sndpkt.front().seqnum + 1) % maxSeq;
				this->sndpkt.pop_front();
				cout << "滑动后窗口中报文序号为：";
				f << "滑动后窗口中报文序号为：";
				for (auto p : sndpkt) {
					f << p.seqnum << " ";
					cout << p.seqnum << " ";
				}
				cout << "\n" << endl;
				f << "\n" << endl;
			}
			if (this->sndpkt.size() < this->winSize) {
				this->waitingState = false;																	//进入等待状态
			}
		}
	}
}

void GBNRdtSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	for (Packet& p : this->sndpkt) {
		pUtils->printPacket("发送方定时器时间到，重发上次发送的报文", p);
		pns->sendToNetworkLayer(RECEIVER, p);			//重新发送数据包
	}

}