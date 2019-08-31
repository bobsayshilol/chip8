#include <cstring>
#include "chip8.h"

namespace
{
	uint16_t bswap(uint16_t val) noexcept
	{
		uint16_t o = 0;
		o |= (val >> 8) & 0x00FF;
		o |= (val << 8) & 0xFF00;
		return o;
	}
}

namespace emu
{
	bool ROM::Load(const void* data, std::size_t size)
	{
		mData.resize(size);
		std::memcpy(mData.data(), data, size);
		return true;
	}
	
	
	CHIP8::CHIP8()
		: mRAM{}
		, mRegisters{}
		, mPC{}
		, mI{}
		, mStackFrames{}
		, mStack{}
	{
	}
	
	bool CHIP8::Load(const ROM& rom)
	{
		bool loaded = false;
		const auto& data = rom.GetData();
		
		if (data.size() < mRAM.size())
		{
			std::copy(data.begin(), data.end(), mRAM.begin());
			loaded = true;
		}
		
		return loaded;
	}
	
	void CHIP8::Step(std::size_t instructions)
	{
		for (size_t i = 0; i < instructions; i++)
		{
			// Read the next instruction
			const Instruction ins = ReadInstruction();
			
			// Handle it
			switch (ins / 0x1000)
			{
#define CASE(x) case 0x ## x : Handle_ ## x (ins); break
				
				CASE(0);	CASE(1);	CASE(2);	CASE(3);
				CASE(4);	CASE(5);	CASE(6);	CASE(7);
				CASE(8);	CASE(9);	CASE(A);	CASE(B);
				CASE(C);	CASE(D);	CASE(E);	CASE(F);
				
#undef CASE
			}
		}
	}
	
	void CHIP8::Dump() const
	{
		printf("CHIP-8 state:\n");
		
		// Registers
		printf("\tRegisters:\n");
		for (size_t i = 0; i < mRegisters.size(); i++)
		{
			printf("\t\tV%zX: 0x%x", i, mRegisters[i]);
			if ((i & 3) == 3)
			{
				printf("\n");
			}
		}
		printf("\t\tPC: 0x%x", mPC);
		printf("\t\tI:  0x%x\n", mI);
		
		// Stack
		printf("\tStack:\n");
		for (size_t i = 0; i < mStack; i++)
		{
			printf("\t\t%zu:\t0x%x", i, mStackFrames[i]);
		}
	}
	
	void CHIP8::OnError(const char* msg) const
	{
		// Dump the state of the emulator before throwing so we have a chance at seeing what's going on
		Dump();
		throw std::runtime_error(msg);
	}
	
	CHIP8::Instruction CHIP8::ReadInstruction()
	{
		// Check for overflow
		if (mPC + sizeof(Instruction) >= mRAM.size())
		{
			OnError("Program counter left RAM");
		}
		
		// Read the instruction
		Instruction ins;
		memcpy(&ins, &mRAM[mPC], sizeof(ins));
		ins = bswap(ins);
		
		// Update the PC
		mPC += sizeof(Instruction);
		
		return ins;
	}
	
	
	void CHIP8::Unhandled(Instruction ins)
	{
		// Log the unhandled instruction
		char message[64];
		snprintf(message, sizeof(message), "Unhandled instruction: 0x%X", ins);
		OnError(message);
	}
	
	void CHIP8::Handle_0(Instruction ins)
	{
		Unhandled(ins);
	}
	
	void CHIP8::Handle_1(Instruction ins)
	{
		// Read off the address
		const Address address = ins & 0x0FFF;
		
		// Update PC
		mPC = address;
	}
	
	void CHIP8::Handle_2(Instruction ins)
	{
		Unhandled(ins);
	}
	
	void CHIP8::Handle_3(Instruction ins)
	{
		Unhandled(ins);
	}
	
	void CHIP8::Handle_4(Instruction ins)
	{
		Unhandled(ins);
	}
	
	void CHIP8::Handle_5(Instruction ins)
	{
		Unhandled(ins);
	}
	
	void CHIP8::Handle_6(Instruction ins)
	{
		// Read off the register and value
		const uint8_t reg = (ins >> 16) & 0x0F;
		const uint8_t val = (ins >>  0) & 0xFF;
		
		// Update the register
		mRegisters[reg] = val;
	}
	
	void CHIP8::Handle_7(Instruction ins)
	{
		// Read off the register and value
		const uint8_t reg = (ins >> 16) & 0x0F;
		const uint8_t val = (ins >>  0) & 0xFF;
		
		// Update the register
		mRegisters[reg] += val;
	}
	
	void CHIP8::Handle_8(Instruction ins)
	{
		Unhandled(ins);
	}
	
	void CHIP8::Handle_9(Instruction ins)
	{
		Unhandled(ins);
	}
	
	void CHIP8::Handle_A(Instruction ins)
	{
		// Read off the address
		const Address address = ins & 0x0FFF;
		
		// Update I
		mI = address;
	}
	
	void CHIP8::Handle_B(Instruction ins)
	{
		Unhandled(ins);
	}
	
	void CHIP8::Handle_C(Instruction ins)
	{
		Unhandled(ins);
	}
	
	void CHIP8::Handle_D(Instruction ins)
	{
		Unhandled(ins);
	}
	
	void CHIP8::Handle_E(Instruction ins)
	{
		Unhandled(ins);
	}
	
	void CHIP8::Handle_F(Instruction ins)
	{
		Unhandled(ins);
	}
}
