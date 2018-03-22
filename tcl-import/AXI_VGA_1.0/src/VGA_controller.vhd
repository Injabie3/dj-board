----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 02/15/2018 03:24:26 PM
-- Design Name: 
-- Module Name: VGA_controller - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity VGA_controller is
    Port ( CLK : in STD_LOGIC;
           RESETN : in STD_LOGIC;
           DATA : in STD_LOGIC_VECTOR (11 downto 0);
           FIFO_BSY : in STD_LOGIC;
           FIFO_EMPTY : in STD_LOGIC;
           HSYNC : out STD_LOGIC;
           VSYNC : out STD_LOGIC;
           R : out STD_LOGIC_VECTOR (3 downto 0);
           G : out STD_LOGIC_VECTOR (3 downto 0);
           B : out STD_LOGIC_VECTOR (3 downto 0);
           FIFO_EN : out STD_LOGIC);
end VGA_controller;

architecture Behavioral of VGA_controller is
--constants
constant h_length : INTEGER := 640;
constant v_length : INTEGER := 480;

constant h_start_pulse : INTEGER := h_length + 16;
constant h_end_pulse : INTEGER := h_start_pulse + 96;

constant v_start_pulse : INTEGER := v_length + 10;
constant v_end_pulse : INTEGER := v_start_pulse + 2;

constant h_end : INTEGER := 800;
constant v_end : INTEGER := 525;

--signals
signal vid_out_sig, hsync_sig, vsync_sig, read_sig : STD_LOGIC;
signal h_count, v_count : UNSIGNED(15 downto 0);

signal R_sig, G_sig, B_sig : STD_LOGIC_VECTOR(3 downto 0);
begin

timing_proc : process(CLK)
begin

	if(rising_edge(CLK)) then
		if(RESETN = '0') then
			h_count <= to_unsigned(0, h_count'LENGTH);
			v_count <= to_unsigned(0, v_count'LENGTH);
			
			hsync_sig <= '0';
			vsync_sig <= '0';
			vid_out_sig <= '0';
		else		
		    if(read_sig = '1') then
			    if(h_count < h_length AND v_count < v_length) then
			    	vid_out_sig <= '1';
			    else
			    	vid_out_sig <= '0';
			    end if;
			    	
			    if(h_count = (h_start_pulse - 1)) then
			    	hsync_sig <= '0';
			    elsif(h_count = (h_end_pulse - 1)) then
			    	hsync_sig <= '1';
			    end if;
			    
			    if(v_count = (v_start_pulse - 1)) then
			    	vsync_sig <= '0';
			    elsif(v_count = (v_end_pulse - 1)) then
			    	vsync_sig <= '1';
			    end if;
			    
			    if(h_count = (h_end - 1)) then
			    	h_count <= to_unsigned(0, h_count'LENGTH);
			    	
			    	if(v_count = (v_end - 1)) then
			    		v_count <= to_unsigned(0, v_count'LENGTH);
			    	else
			    		v_count <= v_count + 1;
			    	end if;
			    else
			    	h_count <= h_count + 1;
			    end if;
			end if; -- fifo full
		
		end if; -- reset
		
	end if; -- rising_edge

end process;

video_proc : process(CLK)
    begin
    
    if(rising_edge(CLK)) then
        if (RESETN = '0') then
            R_sig <= (others => '0');
            G_sig <= (others => '0');
            B_sig <= (others => '0');
        else
            if((read_sig = '1') and (vid_out_sig = '1')) then
                R_sig <= DATA(11 downto 8);
                G_sig <= DATA(7 downto 4);
                B_sig <= DATA(3 downto 0);
            else
                R_sig <= (others => '0');
                G_sig <= (others => '0');
                B_sig <= (others => '0');
            end if;
        end if;
    end if; -- rising edge
    
end process;
    read_sig <= NOT(FIFO_EMPTY or FIFO_BSY);
    
    FIFO_EN <= vid_out_sig;
    
    HSYNC <= HSYNC_sig;
    VSYNC <= VSYNC_sig;
    
    R <= R_sig;
    G <= G_sig;
    B <= B_sig;
end Behavioral;
