-- Filename: debouncer.vhd
-- Author 1: Ryan Lui
-- Author 1 Student #: 301251951
-- Author 2: Greyson Wang
-- Author 2 Student #: 301249759
-- Group Number: 27
-- Lab Section
-- Lab: 6
-- Task Completed: All.
-- Date: 2017-01-10
------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

-- Debouncer circuit, based off of the one in Slide Set 02.
entity debouncer is
	port(	myinput, clk: in std_logic;
			debounced: out std_logic
	);
end debouncer;

architecture behavioural of debouncer is
	signal q1, q2, cout: std_logic;
	-- q1: Intermediate output from flip flop 1.
	-- q2: Intermediate output from flip flop 2.
	-- cout: The carry out from the counter, used to drive the enable on the final flip flop.
	signal SCLRinput, ENinput: std_logic;
	component dflipflop is
		port(	D, EN, clk: in std_logic;
				Q: out std_logic
		);
	end component;
	component counter is
		generic (Nbits: integer := 23);
		-- Input must be stable for 1/(100MHz/2^23) = 83.89ms for input to stabilize.
		port(	SCLR, EN, clk: in std_logic;
				Cout: out std_logic
		);
	end component;
begin
	SCLRinput <= q1 xor q2;
	ENinput <= not cout;
	dflipflop1: dflipflop port map(D => myinput, clk => clk, Q => q1, EN => '1');
	dflipflop2: dflipflop port map(D => q1, clk => clk, Q => q2, EN => '1');
	dflipflop3: dflipflop port map(D => q2, clk => clk, Q => debounced, EN => cout);
	counter1: counter generic map(Nbits => 23) port map(SCLR => SCLRinput, clk => clk, EN => ENinput, Cout => cout);
end behavioural;
	
