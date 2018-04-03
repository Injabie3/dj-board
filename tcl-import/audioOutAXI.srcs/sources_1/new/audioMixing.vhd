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
        playInterrupt:     in std_logic;
    
    
         S_Rec_tData:        in std_logic_vector(31 downto 0); -- extra bit will tell us to playInterrupt 
         S_Rec_tValid:       in std_logic;
         S_Rec_tLast:        in std_logic;
         --M_dmaRec_tReady:    in std_logic;
        S_Rec_tReady:      out std_logic;
        
            
        M_dma_tData:        out std_logic_vector(31 downto 0);
        M_dma_tValid:       out std_logic;
        M_dma_tReady:       in std_logic;
        M_dma_tLast:        out std_logic
    );
end audioMixing;

architecture Behavioral of audioMixing is
    type states is (Idle, SendOriginal, SendMix);
    signal PS, NS: states; -- Present State and Next State, respectively.
    
    -- ##########################
    -- # Registers # -- 
    signal tRxDataReg:            std_logic_vector(31 downto 0);
    signal tRecDataReg:           std_logic_vector(31 downto 0);
    
    --####Control Signals###-- 
    signal tRxDataLoad:           std_logic; -- register enable
    signal tRecDataLoad:          std_logic; -- register enable
    signal tLastReg:              std_logic;
    signal tLastLoad:             std_logic; -- register enable
    signal mixAudio:              std_logic; 
    
    
    signal lChannelSum:          signed(16 downto 0); 
    signal rChannelSum:          signed(16 downto 0); 
    signal mixedAudio:            std_logic_vector(31 downto 0); 
    
begin

    M_dma_tLast <= tLastReg; 
    
    lChannelSum <= resize(signed(tRxDataReg(15 downto 0)), lChannelSum'length) + resize(signed(tRecDataReg(15 downto 0)), lChannelSum'length); 
    rChannelSum <= resize(signed(tRxDataReg(31 downto 16)), rChannelSum'length) + resize(signed(tRecDataReg(31 downto 16)), rChannelSum'length); 
    mixedAudio <= std_logic_vector(rChannelSum(16 downto 1)) & std_logic_vector(lChannelSum(16 downto 1)); 

    -- Synchronous Logic
    process(clk)
        -- No variables to declare.
    begin
        if(rising_edge(clk)) then
            if(reset = '0') then -- Active low synchronous reset.
                PS <= Idle;
                tRxDataReg <= x"00000000";
                tRecDataReg <= x"00000000";
                tLastReg <= '0';
            else 
                PS <= NS; 
                -- load data into corresponding registers 
                if (tRxDataLoad = '1') then 
                    tRxDataReg <= S_Rx_tData;
                end if;
                if (tRecDataLoad = '1') then 
                    tRecDataReg <= S_Rec_tData; 
                end if; 
                if (tLastLoad = '1') then
                    tLastReg <= S_Rx_tLast;
                end if;
            end if; -- if (reset = 0) 
        end if; -- if rising_edge(clk)    
    end process; 
  
  
  
    process(mixAudio, mixedAudio, tRxDataReg) 
    begin 
        if (mixAudio = '1') then 
            M_dma_tData <= mixedAudio; 
        else 
            M_dma_tData <= tRxDataReg;
        end if;
    end process; 
    
    
    -- IFL and OFL 
    process(PS, S_Rec_tValid, S_Rx_tValid, M_dma_tReady, playInterrupt) 
    begin 
        -- default values to avoid latching 
        tRxDataLoad <= '0';
        tRecDataLoad <= '0'; 
        tLastLoad <= '0';
        mixAudio <= '0'; 
        M_dma_tValid <= '0'; 
        S_Rx_tReady <= '0'; 
        S_Rec_tReady <= '0'; 
        case PS is     
            when Idle => 
                S_Rx_tReady <= '1';                 
                tRxDataLoad <= '1';
                tRecDataLoad <= '1';
                tLastLoad <= '1'; 
                if (S_Rx_tValid = '0') then 
                    NS <= PS; 
                elsif (S_Rx_tValid = '1' and S_Rec_tValid = '1' and playInterrupt = '1') then -- if S_Rx_tValid = '1' 
                    NS <= SendMix;
                    S_Rec_tReady <= '1';
               elsif (S_Rx_tValid = '1' and  playInterrupt = '0') then 
                    NS <= SendOriginal; 
               else -- should NEVER go here but we will just stay in present state 
                    NS <= PS; 
                end if; 
            when SendOriginal => 
                M_dma_tValid <= '1';
                if (M_dma_tReady = '0') then 
                    NS <= PS; 
                else -- if M_dma_tReady = 1  
                    NS <= Idle;
                end if;
            when SendMix => 
             M_dma_tValid <= '1';
             mixAudio <= '1'; 
             if (M_dma_tReady = '0') then 
                NS <= PS; 
            else -- if M_dma_tReady = 1  
                NS <= Idle;
            end if;
        end case; 
    end process; 

end Behavioral;
