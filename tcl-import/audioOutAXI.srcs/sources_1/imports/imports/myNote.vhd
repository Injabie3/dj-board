LIBRARY ieee;
USE ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity myNote is
	generic(
		busWidth: integer;
		frequency: integer
		);
	port(
		clk: in std_logic;
		reset: in std_logic; -- Active high synchronous reset.
		increment: in std_logic; -- ENable
		amplHigh: in signed (busWidth-1 downto 0);
		amplLow: in signed (busWidth-1 downto 0);
		myOutput: out signed(busWidth-1 downto 0)
		);
end myNote;

architecture behavioural of myNote is
	constant sampleSize: integer := 48000/frequency;
	signal noteRegister: unsigned(10 downto 0); --Arbitrary
begin

	process(clk)
	begin
		if (rising_edge(clk)) then
			if(increment = '1') then
				noteRegister <= noteRegister + 1;
				if ((noteRegister >= sampleSize) or (reset = '1')) then -- 262 samples for C
					noteRegister <= to_unsigned(0, noteRegister'length);
				end if;
			end if;
		end if;
	end process;
	
	process(noteRegister, amplLow, amplHigh)
	begin
		if(noteRegister >= sampleSize/2) then
			myOutput <= amplLow;
		else
			myOutput <= amplHigh;
		end if;
	end process;
	
end behavioural;