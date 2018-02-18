-- Filename: debouncer5.vhd
-- Description: A 5 input debouncer, based off of my ENSC 350 debouncer.
-- Author 1: Ryan Lui
-- Author 1 Student #: 301251951

-- Date: 2018-02-16
------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

-- Debouncer circuit, based off of the one in Slide Set 02.
entity debouncer5 is
    port(   clk: in std_logic;
            myinput: in std_logic_vector(4 downto 0);
            debounced: out std_logic_vector(4 downto 0)
	);
end debouncer5;

architecture behavioural of debouncer5 is
    component debouncer is
        port(	myinput, clk: in std_logic;
                debounced: out std_logic
        );
    end component;
begin
    -- Could probably use a generate here.
    
    debouncer_0: debouncer port map (
        clk => clk,
        myinput => myinput(0),
        debounced => debounced(0)
    );
    
    debouncer_1: debouncer port map (
        clk => clk,
        myinput => myinput(1),
        debounced => debounced(1)
    );
    
    debouncer_2: debouncer port map (
        clk => clk,
        myinput => myinput(2),
        debounced => debounced(2)
    );
    
    debouncer_3: debouncer port map (
        clk => clk,
        myinput => myinput(3),
        debounced => debounced(3)
    );
    
    debouncer_4: debouncer port map (
        clk => clk,
        myinput => myinput(4),
        debounced => debounced(4)
    );

end behavioural;

