-- Filename: genericDebouncer.vhd
-- Description: A variable input debouncer, based off of my ENSC 350 debouncer.
-- Author 1: Ryan Lui
-- Author 1 Student #: 301251951

-- Date: 2018-02-22
------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

-- Debouncer circuit, based off of the one in Slide Set 02.
entity genericDebouncer is
    generic(
        busWidth: integer
    );
    port(   clk: in std_logic;
            myinput: in std_logic_vector(busWidth-1 downto 0);
            debounced: out std_logic_vector(busWidth-1 downto 0)
	);
end genericDebouncer;

architecture behavioural of genericDebouncer is
    component debouncer is
        port(	myinput, clk: in std_logic;
                debounced: out std_logic
        );
    end component;
begin
    -- Could probably use a generate here.
    generate_debouncers: for index in busWidth-1 downto 0 generate        
        oneDebouncer: debouncer port map (
            clk => clk,
            myinput => myinput(index),
            debounced => debounced(index)
        );
    end generate;

end behavioural;
