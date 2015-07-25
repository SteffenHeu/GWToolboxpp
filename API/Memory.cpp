#include "Memory.h"



template <typename T>
T GWAPI::MemoryMgr::ReadPtrChain(DWORD pBase, long pOffset1 /*= 0*/, long pOffset2 /*= 0*/, long pOffset3 /*= 0*/, long pOffset4 /*= 0*/, long pOffset5 /*= 0*/)
{
	DWORD pRead = pBase;
	if (pRead == NULL){ return 0; }

	if (pOffset1){ pRead = *(DWORD*)(pRead + pOffset1); }
	if (pRead == NULL){ return 0; }

	if (pOffset2){ pRead = *(DWORD*)(pRead + pOffset2); }
	if (pRead == NULL){ return 0; }

	if (pOffset3){ pRead = *(DWORD*)(pRead + pOffset3); }
	if (pRead == NULL){ return 0; }

	if (pOffset4){ pRead = *(DWORD*)(pRead + pOffset4); }
	if (pRead == NULL){ return 0; }

	if (pOffset5){ pRead = *(DWORD*)(pRead + pOffset5); }
	if (pRead == NULL){ return 0; }

	return (T)(pRead);
}

void GWAPI::MemoryMgr::Retour(BYTE *src, BYTE *restore, const int len)
{
	DWORD dwBack;

	VirtualProtect(src, len, PAGE_READWRITE, &dwBack);
	memcpy(src, restore, len);

	restore[0] = 0xE9;
	*(DWORD*)(restore + 1) = (DWORD)(src - restore) - 5;

	VirtualProtect(src, len, dwBack, &dwBack);

	delete[] restore;
}

void * GWAPI::MemoryMgr::Detour(BYTE *src, const BYTE *dst, const int len, BYTE** restore /*= NULL*/)
{
	if (restore){
		*restore = new BYTE[len];
		memcpy(*restore, src, len);
	}


	BYTE *jmp = (BYTE*)malloc(len + 5);
	DWORD dwBack;

	VirtualProtect(src, len, PAGE_READWRITE, &dwBack);

	memcpy(jmp, src, len);
	jmp += len;

	jmp[0] = 0xE9;
	*(DWORD*)(jmp + 1) = (DWORD)(src + len - jmp) - 5;

	src[0] = 0xE9;
	*(DWORD*)(src + 1) = (DWORD)(dst - src) - 5;

	for (int i = 5; i < len; i++)
		src[i] = 0x90;

	VirtualProtect(src, len, dwBack, &dwBack);

	return (jmp - len);
}

bool GWAPI::MemoryMgr::Scan()
{
#define SCAN_START (BYTE*)0x401000
#define SCAN_END (BYTE*)0x900000

	for (BYTE* scan = SCAN_START; scan < SCAN_END; scan++)
	{
		// Agent Array
		const BYTE AgentBaseCode[] = { 0x56, 0x8B, 0xF1, 0x3B, 0xF0, 0x72, 0x04 };
		if (!memcmp(scan, AgentBaseCode, sizeof(AgentBaseCode)))
		{
			agArrayPtr = *(AgentArray**)(scan + 0xC);
			PlayerAgentIDPtr = (DWORD*)(agArrayPtr - 0x54);
			TargetAgentIDPtr = (DWORD*)(agArrayPtr - 0x500);
		}

		// Packet Sender Stuff
		const BYTE CtoGSObjectCode[] = { 0x56, 0x33, 0xF6, 0x3B, 0xCE, 0x74, 0x0E, 0x56, 0x33, 0xD2 };
		const BYTE CtoGSSendCode[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x2C, 0x53, 0x56, 0x57, 0x8B, 0xF9, 0x85 };
		if (!memcmp(scan, CtoGSObjectCode, sizeof(CtoGSObjectCode)))
		{
			CtoGSObjectPtr = (DWORD*)scan;
		}
		if (!memcmp(scan, CtoGSSendCode, sizeof(CtoGSSendCode)))
		{
			CtoGSSendFunction = (SendCtoGSPacket_t)scan;
		}

		// Base pointer, used to get context pointer for game world.
		const BYTE BasePointerLocationCode[] = { 0x8B, 0x42, 0x0C, 0x56, 0x8B, 0x35 };
		if (!memcmp(scan, BasePointerLocationCode, sizeof(BasePointerLocationCode)))
		{
			BasePointerLocation = (BYTE*)(*(DWORD*)(scan + 6));
		}

		// Used for gamethread calls, as well as disable/enable rendering.
		const BYTE EngineCode[] = { 0x53, 0x56, 0xDF, 0xE0, 0xF6, 0xC4, 0x41 };
		if (!memcmp(scan, EngineCode, sizeof(EngineCode)))
		{
			RenderLoopLocation = scan + 0x65;
			GameLoopLocation = RenderLoopLocation - 0x76;
			RenderLoopLocation = GameLoopLocation + 0x5D;
			GameLoopReturn = (BYTE*)MemoryMgr::Detour(GameLoopLocation, (BYTE*)gameLoopHook, 5, &GameLoopRestore);
		}

		// For Map IDs
		const BYTE MapIdLocationCode[] = { 0xB0, 0x7F, 0x8D, 0x55 };
		if (!memcmp(scan, MapIdLocationCode, sizeof(MapIdLocationCode))){
			MapIDPtr = *(DWORD**)(scan + 0x46);
		}

		// To write info / Debug as a PM in chat
		const BYTE WriteChatCode[] = { 0x55, 0x8B, 0xEC, 0x51, 0x53, 0x89, 0x4D, 0xFC, 0x8B, 0x4D, 0x08, 0x56, 0x57, 0x8B };
		if (!memcmp(scan, WriteChatCode, sizeof(WriteChatCode))){
			WriteChatFunction = (WriteChat_t)scan;
		}

		// Skill timer to use for exact effect times.
		const BYTE SkillTimerCode[] = { 0x85, 0xc9, 0x74, 0x15, 0x8b, 0xd6, 0x2b, 0xd1, 0x83, 0xfa, 0x64 };
		if (!memcmp(scan, SkillTimerCode, sizeof(SkillTimerCode))){
			SkillTimerPtr = (DWORD*)(scan - 4);
		}

		// Skill array.
		const BYTE SkillArrayCode[] = { 0x8D, 0x04, 0xB6, 0x5E, 0xC1, 0xE0, 0x05, 0x05 };
		if (!memcmp(scan, SkillArrayCode, sizeof(SkillArrayCode))){
			SkillArray = *(Skill**)(scan + 8);
		}

		if (agArrayPtr &&
			CtoGSObjectPtr &&
			CtoGSSendFunction &&
			BasePointerLocation &&
			RenderLoopLocation &&
			MapIDPtr &&
			WriteChatFunction &&
			SkillTimerPtr &&
			SkillArray
			) return true;
	}
	return false;
}

GWAPI::AgentArray GWAPI::MemoryMgr::GetAgentArray()
{
	return *agArrayPtr;
}

GWAPI::SkillbarArray GWAPI::MemoryMgr::GetSkillbarArray()
{
	return *ReadPtrChain<SkillbarArray*>(GetContextPtr(), 0x2C, 0x6F0);
}

GWAPI::Skillbar GWAPI::MemoryMgr::GetPlayerSkillbar()
{
	return *ReadPtrChain<Skillbar*>(GetContextPtr(), 0x2C, 0x6F0);
}

GWAPI::EffectArray GWAPI::MemoryMgr::GetPlayerEffectArray()
{
	return *ReadPtrChain<gw_array<Effect>*>(GetContextPtr(), 0x2C, 0x508, 0x14);
}

GWAPI::MemoryMgr* GWAPI::MemoryMgr::GetInstance()
{
	static MemoryMgr* inst = new MemoryMgr();
	return inst;
}
