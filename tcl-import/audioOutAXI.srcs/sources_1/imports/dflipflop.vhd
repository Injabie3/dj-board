-- Filename: dflipflop.vhd
-- Author 1: Ryan Lui
-- Author 1 Student #: 301251951
-- Author 2: Greyson Wang
-- Author 2 Student #: 301249759
-- Group Number: 27
-- Lab Section
-- Lab: 6
-- Task Completed: All.
-- Date: 2017-01-10
-- Description: Doing a D Flip-flop in another entity to practice writing
-- another process.
------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity dflipflop is
	port(	D, EN, clk: in std_logic;
			Q: out std_logic
	);
end dflipflop;

architecture behavioural of dflipflop is
begin
	process(D, clk, EN)
	begin
		if(rising_edge(clk) and EN = '1') then
			Q <= D;
		end if;
	end process;
end behavioural;