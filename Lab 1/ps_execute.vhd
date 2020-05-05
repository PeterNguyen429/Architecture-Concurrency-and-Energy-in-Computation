-- ECE 3056: Architecture, Concurrency and Energy in Computation
-- Sudhakar Yalamanchili
-- Pipelined MIPS Processor VHDL Behavioral Mode--
--
--
-- execution unit. only a subset of instructions are supported in this
-- model, specifically add, sub, lw, sw, beq, and, or
--

Library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_arith.all;
use IEEE.std_logic_signed.all;

entity execute is
port(
--
-- inputs
--
     PC4 : in std_logic_vector(31 downto 0);
     register_rs, register_rt, wb_memory_data, wb_alu_result, mem_alu_result :in std_logic_vector (31 downto 0); --=added p4 memory data and alu result
     Sign_extend :in std_logic_vector(31 downto 0);
     ALUOp: in std_logic_vector(1 downto 0);
     ALUSrc, RegDst, wb_RegWrite, wb_MemtoReg, mem_RegWrite : in std_logic; --added pipeline 4 RegWrite signal, MemtoReg
     wreg_rd, wreg_rt, wreg_rs, wb_wreg_addr, mem_wreg_addr : in std_logic_vector(4 downto 0); --added p4 wreg addr

-- outputs
--
     alu_result, Branch_PC :out std_logic_vector(31 downto 0);
     wreg_address : out std_logic_vector(4 downto 0);
     zero: out std_logic);
     end execute;


architecture behavioral of execute is
SIGNAL Ainput, Binput	: STD_LOGIC_VECTOR( 31 DOWNTO 0 );
signal ALU_Internal : std_logic_vector (31 downto 0);
Signal Function_opcode : std_logic_vector (5 downto 0);

SIGNAL ALU_ctl	: STD_LOGIC_VECTOR( 2 DOWNTO 0 );
SIGNAL ForwA, ForwB  : STD_LOGIC_VECTOR(1 DOWNTO 0);

BEGIN
  ForwA <= "10" WHEN (mem_RegWrite = '1' and (mem_wreg_addr /= X"000000")) and (wreg_rs = mem_wreg_addr) ELSE
           "01" WHEN (wb_RegWrite = '1' and (wb_wreg_addr /= X"000000")) and NOT(mem_RegWrite = '1' and mem_wreg_addr /= X"000000" and mem_wreg_addr = wreg_rs) and wb_wreg_addr = wreg_rs ELSE
           "00"; --logic from notes
  ForwB <= "10" WHEN (mem_RegWrite = '1' and (mem_wreg_addr /= X"000000")) and (wreg_rt = mem_wreg_addr) ELSE
           "01" WHEN (wb_RegWrite = '1' and (wb_wreg_addr /= X"000000")) and NOT(mem_RegWrite = '1' and mem_wreg_addr /= X"000000" and mem_wreg_addr = wreg_rt) and wb_wreg_addr = wreg_rt ELSE
           "00"; --logic from notes
    -- compute the two ALU inputs
	Ainput <= register_rs when ForwA = "00" ELSE
            mem_alu_result when ForwA = "10" ELSE
            wb_memory_data when (ForwA = "01" and wb_MemtoReg = '1') ELSE
            wb_alu_result when (ForwA = "01" and wb_MemToReg = '0');

	-- ALU input mux
	Binput <= register_rt WHEN (ALUSrc = '0' and ForwB = "00") ELSE
            mem_alu_result when (ALUSrc = '0' and ForwB = "10") ELSE
            wb_memory_data when (ALUSrc = '0' and ForwB = "01" and wb_MemToReg = '1') ELSE
            wb_alu_result when (ALUSrc = '0' and ForwB = "01" and wb_MemToReg = '0') ELSE
	          Sign_extend(31 downto 0) when ALUSrc = '1' ELSE
	         X"BBBBBBBB";

	 Branch_PC <= PC4 + (Sign_extend(29 downto 0) & "00");

	 -- Get the function field. This will be the least significant
	 -- 6 bits of  the sign extended offset

	 Function_opcode <= Sign_extend(5 downto 0);

		-- Generate ALU control bits

	ALU_ctl( 0 ) <= ( Function_opcode( 0 ) OR Function_opcode( 3 ) ) AND ALUOp(1 );
	ALU_ctl( 1 ) <= ( NOT Function_opcode( 2 ) ) OR (NOT ALUOp( 1 ) );
	ALU_ctl( 2 ) <= ( Function_opcode( 1 ) AND ALUOp( 1 )) OR ALUOp( 0 );

		-- Generate Zero Flag
	Zero <= '1' WHEN ( ALU_internal = X"00000000"  )
		         ELSE '0';

-- implement the RegDst mux in this pipeline stage
--
wreg_address <= wreg_rd when RegDst = '1' else wreg_rt;

  ALU_result <= ALU_internal;

PROCESS ( ALU_ctl, Ainput, Binput )
	BEGIN
					-- Select ALU operation
 	CASE ALU_ctl IS
						-- ALU performs ALUresult = A_input AND B_input
		WHEN "000" 	=>	ALU_internal 	<= Ainput AND Binput;
						-- ALU performs ALUresult = A_input OR B_input
     	WHEN "001" 	=>	ALU_internal 	<= Ainput OR Binput;
						-- ALU performs ALUresult = A_input + B_input
	 	WHEN "010" 	=>	ALU_internal 	<= Ainput + Binput;
						-- ALU performs ?
 	 	WHEN "011" 	=>	ALU_internal <= X"00000000";
						-- ALU performs ?
 	 	WHEN "100" 	=>	ALU_internal 	<= X"00000000";
						-- ALU performs ?
 	 	WHEN "101" 	=>	ALU_internal 	<=  X"00000000";
						-- ALU performs ALUresult = A_input -B_input
 	 	WHEN "110" 	=>	ALU_internal 	<= (Ainput - Binput);
						-- ALU performs SLT
  	 	WHEN "111" 	=>	ALU_internal 	<= (Ainput - Binput) ;
 	 	WHEN OTHERS	=>	ALU_internal 	<= X"FFFFFFFF" ;
  	END CASE;
  END PROCESS;

end behavioral;
