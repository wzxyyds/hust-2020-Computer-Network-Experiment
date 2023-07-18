#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"
#include "SRConfig.h"

SRRdtReceiver::SRRdtReceiver():rcvPkt(WIN_SIZ),maxSeq(1<<SEQ_BIT)
{
	f.open("D:\\workfile\\ComputerTelecommunicationsAndNetwork\\lab\\lab2\\RDT\\SR\\receiverWindow.txt");
	for (int i = 0; i < WIN_SIZ; i++) {
		rcvPkt[i].acknum = 0;//标记是否接收到
		rcvPkt[i].checksum = 0;
		rcvPkt[i].seqnum = i;
	}
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


SRRdtReceiver::~SRRdtReceiver()
{
	f.close();
}

void SRRdtReceiver::receive(const Packet &packet) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);

	//如果校验和正确，同时收到报文的序号等于接收方期待收到的报文序号一致
	if (checkSum == packet.checksum) {
		pUtils->printPacket("接收方正确收到发送方的报文", packet);
		int t = (packet.seqnum + this->maxSeq - this->rcvPkt.front().seqnum) % this->maxSeq;//计算接收到的数据包在窗口中的下标
		if (t<WIN_SIZ) {
			this->rcvPkt[t]=packet;
			this->rcvPkt[t].acknum=1;
		}
		while (this->rcvPkt.front().acknum == 1) {
			cout << "滑动前接收方窗口中报文序号为：";
			f << "滑动前接收方窗口中报文序号为：";
			for (int i = 0; i < rcvPkt.size(); i++) {
				if (rcvPkt[i].acknum) {
					f << rcvPkt[i].seqnum << " ";
					cout << rcvPkt[i].seqnum << " ";
				}
			}
			f << endl;
			cout << endl;
			Message msg;
			memcpy(msg.data, rcvPkt.front().payload, sizeof(rcvPkt.front().payload));
			pns->delivertoAppLayer(RECEIVER, msg);
			rcvPkt.pop_front();
			Packet p;
			p.acknum = 0;
			p.seqnum = (rcvPkt.back().seqnum + 1) % maxSeq;
			rcvPkt.push_back(p);
			cout << "滑动后接收方窗口中报文序号滑动为：";
			f << "滑动后接收方窗口中报文序号滑动为：";
			for (int i = 0; i < rcvPkt.size(); i++) {
				if (rcvPkt[i].acknum) {
					f << rcvPkt[i].seqnum << " ";
					cout << rcvPkt[i].seqnum << " ";
				}
			}
			f << "\n"<<endl;
			cout <<"\n"<< endl;
		}
		lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("接收方发送确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
	}
}