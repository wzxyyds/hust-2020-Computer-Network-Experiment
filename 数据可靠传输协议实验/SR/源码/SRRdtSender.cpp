#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"
#include "SRConfig.h"
SRRdtSender::SRRdtSender(): nextSeqnum(0), waitingState(false), winSize(WIN_SIZ), maxSeq(1 << SEQ_BIT)
{
	f.open("D:\\hust\\大三\\计网\\实验二\\Exp2\\Exp2\\SR\\senderWindow.txt");
}


SRRdtSender::~SRRdtSender()
{
	f.close();
}



bool SRRdtSender::getWaitingState() {
	return waitingState;
}

bool SRRdtSender::send(const Message &message) {
	if (this->waitingState) { //发送方处于等待确认状态
		return false;
	}
	Packet packetWaitingACK;
	packetWaitingACK.acknum = 0;//0为未接收到回复
	packetWaitingACK.seqnum = this->nextSeqnum;
	packetWaitingACK.checksum = 0;
	memcpy(packetWaitingACK.payload, message.data, sizeof(message.data));
	packetWaitingACK.checksum = pUtils->calculateCheckSum(packetWaitingACK);
	this->sndpkt.push_back(packetWaitingACK);
	pUtils->printPacket("发送方发送报文", this->sndpkt.back());
	pns->startTimer(SENDER, Configuration::TIME_OUT, sndpkt.back().seqnum);			//启动发送方定时器
	pns->sendToNetworkLayer(RECEIVER, this->sndpkt.back());								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	if (this->sndpkt.size() >= this->winSize) {
		this->waitingState = true;																	//进入等待状态
	}
	this->nextSeqnum = (this->nextSeqnum + 1) % this->maxSeq;
	return true;
}

void SRRdtSender::receive(const Packet &ackPkt) {
	if (!this->sndpkt.empty()) {
		//检查校验和是否正确
		int checkSum = pUtils->calculateCheckSum(ackPkt);

		//如果校验和正确
		if (checkSum == ackPkt.checksum) {
			//计算该序号数据包在队列中的下标
			int temp = (ackPkt.acknum + this->maxSeq - this->sndpkt.front().seqnum) % this->maxSeq;
			if (temp >= 0 && temp < this->sndpkt.size()) {
				this->sndpkt[temp].acknum = 1;//标记为已读
				pns->stopTimer(SENDER, ackPkt.acknum);		//关闭定时器
				pUtils->printPacket("发送方正确收到确认", ackPkt);
				while (!this->sndpkt.empty() && this->sndpkt.front().acknum) {
					cout << "滑动前发送方窗口中报文序号为：";
					f << "滑动前发送方窗口中报文序号为：";
					for (auto p : sndpkt) {
						
						f << p.seqnum << " ";
						cout << p.seqnum << " ";
					}
					cout << endl;
					f << endl;
					this->sndpkt.pop_front();
					cout << "滑动后发送方窗口中报文序号为：";
					f << "滑动后发送方窗口中报文序号为：";
					for (auto p : sndpkt) {
						f << p.seqnum << " ";
						cout << p.seqnum << " ";
					}
					cout << "\n" << endl;
					f << "\n" << endl;
				}
				if (this->sndpkt.size() < this->winSize) {
					this->waitingState = false;																	//取消等待状态
				}
			}
		}
	}
}

void SRRdtSender::timeoutHandler(int seqNum) {
	int temp = (seqNum + this->maxSeq - this->sndpkt.front().seqnum) % this->maxSeq;
	if (temp >= 0 && temp < this->sndpkt.size()) {
		pUtils->printPacket("发送方定时器时间到，重发上次发送的报文", this->sndpkt[(seqNum + this->maxSeq - this->sndpkt.front().seqnum) % this->maxSeq]);
		pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
		pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
		pns->sendToNetworkLayer(RECEIVER, this->sndpkt[(seqNum + this->maxSeq - this->sndpkt.front().seqnum) % this->maxSeq]);			//重新发送数据包
	}
}
