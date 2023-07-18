#include "stdafx.h"
#include "Global.h"
#include "TCPRdtSender.h"
#include "TCPConfig.h"

TCPRdtSender::TCPRdtSender() : nextSeqnum(0),waitingState(false),winSize(WIN_SIZE),maxSeq(1<<SEQ_BIT),base(0), invalidAck(0),lastACK(-1)
{
	f.open("D:\\hust\\大三\\计网\\实验二\\Exp2\\Exp2\\TCP\\senderWindow.txt");
}


TCPRdtSender::~TCPRdtSender()
{
	f.close();
}



bool TCPRdtSender::getWaitingState() {
	return waitingState;
}




bool TCPRdtSender::send(const Message& message) {
	if (this->waitingState) { //发送方处于等待确认状态
		return false;
	}
	Packet packetWaitingACK;
	packetWaitingACK.acknum = -1;
	packetWaitingACK.seqnum = this->nextSeqnum;
	packetWaitingACK.checksum = 0;
	memcpy(packetWaitingACK.payload, message.data, sizeof(message.data));
	packetWaitingACK.checksum = pUtils->calculateCheckSum(packetWaitingACK);
	this->sndpkt.push_back(packetWaitingACK);
	pUtils->printPacket("发送方发送报文", this->sndpkt.back());
	if(base==nextSeqnum)
		pns->startTimer(SENDER, Configuration::TIME_OUT, 0);			//启动发送方定时器
	pns->sendToNetworkLayer(RECEIVER, this->sndpkt.back());								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	if (this->sndpkt.size() >= this->winSize) {
		this->waitingState = true;																	//进入等待状态
	}
	this->nextSeqnum = (this->nextSeqnum + 1) % this->maxSeq;
	return true;
}

void TCPRdtSender::receive(const Packet& ackPkt) {
	if (!this->sndpkt.empty()) {
		//检查校验和是否正确
		int checkSum = pUtils->calculateCheckSum(ackPkt);

		//如果校验和正确
		if (checkSum == ackPkt.checksum) {
			pns->stopTimer(SENDER, 0);		//关闭定时器
			this->base = (ackPkt.acknum + 1) % this->maxSeq;
			if(base != nextSeqnum){
				pns->startTimer(SENDER, Configuration::TIME_OUT, 0);			//启动发送方定时器
			}
			//快速重传
			if (ackPkt.acknum == this->lastACK) {
				pUtils->printPacket("发送方收到冗余确认", ackPkt);
				invalidAck++;
			}
			else {
				pUtils->printPacket("发送方正确收到确认", ackPkt);
				this->lastACK = ackPkt.acknum;
				invalidAck = 0;
			}
			if (this->invalidAck == 3) {
				pUtils->printPacket("接受到三个冗余确认，执行快速重传，重发上次发送的报文", this->sndpkt.front());
				pns->sendToNetworkLayer(RECEIVER, this->sndpkt.front());			//重新发送数据包
				this->invalidAck = 0;
				pns->startTimer(SENDER, Configuration::TIME_OUT, 0);			//启动发送方定时器
			}	
			//检查是否需要滑动窗口
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

void TCPRdtSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	pUtils->printPacket("发送方定时器时间到，重发上次发送的报文", this->sndpkt.front());
	pns->sendToNetworkLayer(RECEIVER, this->sndpkt.front());			//重新发送数据包

}