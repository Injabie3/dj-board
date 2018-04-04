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
        
        
        recInterrupt:     in std_logic;
        storeInterrupt:   in std_logic;
    
-- ######### Inputs for communications with Recorded Sound DMA ########## -- 
         S_Rec_tData:        in std_logic_vector(31 downto 0); -- extra bit will tell us to recInterrupt 
         S_Rec_tValid:       in std_logic;
         S_Rec_tLast:        in std_logic;
        S_Rec_tReady:        out std_logic;
        
-- ######### Inputs for communications with Stored Sound DMA ########## --         
        S_Store_tData:        in std_logic_vector(31 downto 0); -- extra bit will tell us to recInterrupt 
        S_Store_tValid:       in std_logic;
        S_Store_tLast:        in std_logic;
       S_Store_tReady:        out std_logic;
            
        M_dma_tData:        out std_logic_vector(31 downto 0);
        M_dma_tValid:       out std_logic;
        M_dma_tReady:       in std_logic;
        M_dma_tLast:        out std_logic
    );
end audioMixing;

architecture Behavioral of audioMixing is
    type states is (Idle, SendOriginal, SendRecordMix, SendStoredMix);
    signal PS, NS: states; -- Present State and Next State, respectively.
    
    -- ##########################
    -- # Registers # -- 
    signal tRxDataReg:            std_logic_vector(31 downto 0);
    signal tRecDataReg:           std_logic_vector(31 downto 0);
    signal tStoreDataReg:         std_logic_vector(31 downto 0);
    
    --####Control Signals###-- 
    signal tRxDataLoad:           std_logic; -- register enable
    signal tRecDataLoad:          std_logic; -- register enable
    signal tStoreDataLoad:        std_logic; -- register enable
    signal tLastLoad:             std_logic; -- register enable
    
    signal tLastReg:              std_logic;
    signal outputSel:             std_logic_vector(1 downto 0); 
    
    
    signal lChannelRecordSum:          signed(16 downto 0); 
    signal rChannelRecordSum:          signed(16 downto 0); 
    signal mixedRecordedAudio:   std_logic_vector(31 downto 0); 
    
    signal lChannelStoreSum:          signed(16 downto 0); 
    signal rChannelStoreSum:          signed(16 downto 0);
    signal mixedStoredAudio:      std_logic_vector(31 downto 0); 
    
begin

    M_dma_tLast <= tLastReg; 
    
    lChannelRecordSum <= resize(signed(tRxDataReg(15 downto 0)), lChannelRecordSum'length) + resize(signed(tRecDataReg(15 downto 0)), lChannelRecordSum'length); 
    rChannelRecordSum <= resize(signed(tRxDataReg(31 downto 16)), rChannelRecordSum'length) + resize(signed(tRecDataReg(31 downto 16)), rChannelRecordSum'length); 
    mixedRecordedAudio <= std_logic_vector(rChannelRecordSum(16 downto 1)) & std_logic_vector(lChannelRecordSum(16 downto 1)); 
 
    lChannelStoreSum <= resize(signed(tRxDataReg(15 downto 0)), lChannelStoreSum'length) + resize(signed(tStoreDataReg(15 downto 0)), lChannelStoreSum'length); 
    rChannelStoreSum <= resize(signed(tRxDataReg(31 downto 16)), rChannelStoreSum'length) + resize(signed(tStoreDataReg(31 downto 16)), rChannelStoreSum'length); 
    mixedStoredAudio <= std_logic_vector(rChannelStoreSum(16 downto 1)) & std_logic_vector(lChannelStoreSum(16 downto 1)); 

    -- Synchronous Logic
    process(clk)
        -- No variables to declare.
    begin
        if(rising_edge(clk)) then
            if(reset = '0') then -- Active low synchronous reset.
                PS <= Idle;
                tRxDataReg <= x"00000000";
                tRecDataReg <= x"00000000";
                tStoreDataReg <= x"00000000";
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
                if (tStoreDataLoad = '1') then 
                    tStoreDataReg <= S_Store_tData; 
                end if; 
                if (tLastLoad = '1') then
                    tLastReg <= S_Rx_tLast;
                end if;
            end if; -- if (reset = 0) 
        end if; -- if rising_edge(clk)    
    end process; 
  
  
  -- output MUX to determine what to send out 
    process(outputSel, mixedRecordedAudio, mixedStoredAudio, tRxDataReg) 
    begin 
        if (outputSel = "01") then 
            M_dma_tData <= mixedRecordedAudio; 
        elsif (outputSel = "10") then 
            M_dma_tData <= mixedStoredAudio;
        else -- outputSel will only be set to '00' otherwise
            M_dma_tData <=  tRxDataReg;
        end if;
    end process; 
    
    
    -- IFL and OFL 
    process(PS, S_Rec_tValid, S_Rx_tValid, S_Store_tValid, M_dma_tReady, recInterrupt, storeInterrupt) 
    begin 
        -- default values to avoid latching 
        tRxDataLoad <= '0';
        tRecDataLoad <= '0'; 
        tStoreDataLoad <= '0'; 
        tLastLoad <= '0';
        outputSel <= "00"; 
        M_dma_tValid <= '0'; 
        S_Rx_tReady <= '0'; 
        S_Rec_tReady <= '0'; 
        S_Store_tReady <= '0'; 
        case PS is     
            when Idle => 
                S_Rx_tReady <= '1';                 
                tRxDataLoad <= '1';
                tRecDataLoad <= '1';
                tStoreDataLoad <= '1';
                tLastLoad <= '1'; 
                if (S_Rx_tValid = '0') then 
                    NS <= PS; 
               elsif (S_Rx_tValid = '1' and  recInterrupt = '0' and storeInterrupt = '0') then 
                     NS <= SendOriginal; 
                elsif (S_Rx_tValid = '1' and S_Rec_tValid = '1' and recInterrupt = '1') then
                    NS <= SendRecordMix;
                    S_Rec_tReady <= '1';
               elsif (S_Rx_tValid = '1' and S_Store_tValid = '1' and storeInterrupt = '1') then 
                    NS <= SendStoredMix; 
                    S_Store_tReady <= '1';
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
                
                
            when SendRecordMix => 
             M_dma_tValid <= '1';
             outputSel <= "01"; 
             if (M_dma_tReady = '0') then 
                NS <= PS; 
            else -- if M_dma_tReady = 1  
                NS <= Idle;
            end if;
            
            
            when SendStoredMix => 
             M_dma_tValid <= '1';
             outputSel <= "10"; 
             if (M_dma_tReady = '0') then 
                NS <= PS; 
            else -- if M_dma_tReady = 1  
                NS <= Idle;
            end if;
            
        end case; 
    end process; 

end Behavioral;
