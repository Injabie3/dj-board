----------------------------------------------------------------------------------
-- Company: NA
-- Engineer: Ivana Jovasevic 
-- 
-- Create Date: 03/20/2018 11:04:26 AM
-- Design Name: 
-- Module Name: audioMixing - Behavioral
-- Project Name: The Ultimate DJ Board 
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
use IEEE.STD_LOGIC_UNSIGNED.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity audioMixing is
    Port (
        reset:      in std_logic;   -- Active low reset.
        clk:        in std_logic;
    
        -- ##########################
        -- # AXI-STREAM PASSTHROUGH #
        -- ##########################
        S_Rx_tData:        in std_logic_vector(31 downto 0);
        S_Rx_tReady:       out std_logic;
        S_Rx_tValid:       in std_logic;
        S_Rx_tLast:        in std_logic;
        -- playInterrupt:   in std_logic;
    
    
        -- S_Rec_tData:        in std_logic_vector(31 downto 0); -- extra bit will tell us to playInterrupt 
        -- S_Rec_tValid:       in std_logic;
        -- S_Rec_tLast:        in std_logic;
        -- M_dmaRec_tReady:    in std_logic;
    
        M_dma_tData:        out std_logic_vector(31 downto 0);
        M_dma_tValid:       out std_logic;
        M_dma_tReady:       in std_logic;
        M_dma_tLast:        out std_logic
    );
end audioMixing;

architecture Behavioral of audioMixing is
    type states is (Idle, SendAudio);
    signal PS, NS: states; -- Present State and Next State, respectively.
    signal tDataReg:            std_logic_vector(31 downto 0);
    signal tDataLoad:           std_logic; -- register enable
    signal tLastReg:            std_logic;
    signal tLastLoad:           std_logic; -- register enable
 
begin

    M_dma_tData <= tDataReg;  
    M_dma_tLast <= tLastReg; 


    -- Synchronous Logic
    process(clk)
        -- No variables to declare.
    begin
        if(rising_edge(clk)) then
            if(reset = '0') then -- Active low synchronous reset.
                PS <= Idle;
                tDataReg <= x"00000000";
                tLastReg <= '0';
            else 
                PS <= NS; 
                -- load data into corresponding registers 
                if (tDataLoad = '1') then 
                    tDataReg <= S_Rx_tData;
                end if;
                if (tLastLoad = '1') then
                    tLastReg <= S_Rx_tLast;
                end if;
            end if; -- if (reset = 0) 
        end if; -- if rising_edge(clk)    
    end process; 
  
    -- IFL and OFL 
    process(PS, S_Rx_tValid, M_dma_tReady) 
    begin 
        -- default values to avoid latching 
        tDataLoad <= '0';
        tLastLoad <= '0';
        M_dma_tValid <= '0'; 
        S_Rx_tReady <= '0'; 
        case PS is     
            when Idle => 
                S_Rx_tReady <= '1'; 
                tDataLoad <= '1';
                tLastLoad <= '1'; 
                if (S_Rx_tValid = '0') then 
                    NS <= PS; 
                else  -- if S_Rx_tValid = '1' 
                    NS <= SendAudio; 
                end if; 
            when SendAudio => 
                M_dma_tValid <= '1';
                if (M_dma_tReady = '0') then 
                    NS <= PS; 
                else -- if M_dma_tReady = 1  
                    NS <= Idle;
                end if;
            when others => -- should never go here 
                NS <= PS; 
        end case; 
    end process; 

end Behavioral;
