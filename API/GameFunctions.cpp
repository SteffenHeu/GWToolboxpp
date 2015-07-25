#include "GameFunctions.h"

void GWAPI::SendPacket(DWORD Size, DWORD Header, ...)
{
	static DWORD* pak = NULL;
	if (pak != NULL) delete pak;

	pak = new DWORD[Size / 4];
	pak[0] = Header;


	va_list vl;

	va_start(vl, Header);

	for (DWORD i = 1; i <= Size / 4; i++)
	{
		pak[i] = va_arg(vl, DWORD);

	}

	va_end(vl);

	GameThreadMgr::GetInstance()->Enqueue(MemoryMgr::GetInstance()->CtoGSSendFunction, MemoryMgr::GetInstance()->GetCtoSObj(), Size, pak);
}

void GWAPI::Dialog(DWORD ID)
{
	SendPacket(0x8, 0x35, ID);
}

void GWAPI::GoNPC(DWORD AgentID, DWORD CallTarget)
{
	SendPacket(0xC, 0x33, AgentID, CallTarget);
}

void GWAPI::UseItem(DWORD ItemID)
{
	SendPacket(0x8, 0x78, ItemID);
}

void GWAPI::WriteChat(wchar_t* message, ...)
{
	va_list vl;

	static wchar_t* buf = NULL;
	if (buf != NULL) delete buf;

	va_start(vl, message);

	size_t strSize = _vscwprintf(message, vl);
	buf = new wchar_t[strSize + 1];

	vswprintf_s(buf, strSize, message, vl);

	va_end(vl);

	GameThreadMgr::GetInstance()->Enqueue(MemoryMgr::GetInstance()->WriteChatFunction, 0, L"GWToolbox++", buf);
}

struct P5E_SendChat{
	const DWORD header = 0x5E;
	wchar_t channel;
	wchar_t message[137];
};

void GWAPI::SendChat(wchar_t* Message, wchar_t Channel /*= '!'*/)
{
	
	P5E_SendChat* packet = NULL;
	if (packet != NULL) delete packet;

	packet->channel = Channel;
	wcscpy_s(packet->message, Message);

	GameThreadMgr::GetInstance()->Enqueue(MemoryMgr::GetInstance()->CtoGSSendFunction, MemoryMgr::GetInstance()->GetCtoSObj(), 0x11C, (DWORD*)packet);
}
