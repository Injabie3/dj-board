-- Filename: counter.vhd
-- Author 1: Ryan Lui
-- Author 1 Student #: 301251951
-- Author 2: Greyson Wang
-- Author 2 Student #: 301249759
-- Group Number: 27
-- Lab Section
-- Lab: 6
-- Task Completed: All.
-- Date: 2017-01-10
-- Description: A counter that is used for slowing down the 50MHz clock.
------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

-- Counter, need a real clock.
entity counter is
	generic(	Nbits: integer := 12);
	-- Default counter size of 12 bits, but can be changed.
	port(	SCLR, EN, clk: in std_logic;
			Cout: out std_logic
	);
end counter;

architecture behavioural of counter is
	signal int_count: unsigned((Nbits - 1) downto 0) := to_unsigned(0,Nbits);
begin
	process(SCLR, EN, clk, int_count) --Analogous to process(all)
	begin
		if (rising_edge(clk)) then
			if (EN = '1' and SCLR = '0') then -- Increment counter
				int_count <= int_count + 1;
			elsif (SCLR = '1') then --Synchronous clear.
				int_count <= to_unsigned(0,Nbits);
			end if;
		end if;
		Cout <= int_count(Nbits - 1); -- Carry out bit is assigned the output.
	end process;
end behavioural;