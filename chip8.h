#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>

namespace emu
{
	class ROM
	{
	public:
		bool Load(const void * data, std::size_t size);
		const auto& GetData() const { return mData; }
		
	private:
		std::vector<std::byte> mData;
	};
	
	
	class CHIP8
	{
	public:
		CHIP8();
		
		bool Load(const ROM&);
		void Step(std::size_t instructions);
		void Dump() const;
		
	private:
		using Address = uint16_t;
		using Instruction = uint16_t;
		using Register = uint8_t;
		
	private:
		[[noreturn]] void OnError(const char * msg) const;
		Instruction ReadInstruction();
		
		
	private:
		void Unhandled(Instruction);
		void Handle_0(Instruction);
		void Handle_1(Instruction);
		void Handle_2(Instruction);
		void Handle_3(Instruction);
		void Handle_4(Instruction);
		void Handle_5(Instruction);
		void Handle_6(Instruction);
		void Handle_7(Instruction);
		void Handle_8(Instruction);
		void Handle_9(Instruction);
		void Handle_A(Instruction);
		void Handle_B(Instruction);
		void Handle_C(Instruction);
		void Handle_D(Instruction);
		void Handle_E(Instruction);
		void Handle_F(Instruction);
		
		
	private:
		std::array<std::byte, 4096> mRAM;
		
		std::array<Register, 16> mRegisters;
		Address mPC;
		Address mI;
		
		std::array<Address, 24> mStackFrames;
		size_t mStack;
	};
}

#endif
