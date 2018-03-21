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
    playInterrupt:      in  std_logic;
    
    
    S_Rec_tData:        in std_logic_vector(31 downto 0); -- extra bit will tell us to playInterrupt 
    S_Rec_tValid:       in std_logic;
    S_Rec_tLast:        in std_logic;
    --M_dmaRec_tReady:    in std_logic;
    
    M_dma_tData:        out std_logic_vector(31 downto 0);
    M_dma_tValid:       out std_logic;
    M_dma_tLast:        out std_logic
   );
end audioMixing;

architecture Behavioral of audioMixing is
    type states is (Idle, SendOriginal, Mix);

    signal PS, NS: states; -- Present State and Next State, respectively.
    -- #############
    -- # REGISTERS #
    -- #############
    signal tDataReg:            std_logic_vector(31 downto 0);
    signal tRecReg:             std_logic_vector(31 downto 0); 
    
    -- ###################
    -- # CONTROL SIGNALS #
    -- ###################
    signal tDataLoad:           std_logic; -- register enable 
    signal tRecLoad:            std_logic; -- register enable 
    signal mixAudio:            std_logic; -- select for MUX  
    
    
    -- ####################
    -- # INTERNAL SIGNALS #
    -- ####################
    signal ledDReg:             std_logic;
    signal lChannelSum:          signed(16 downto 0);
    signal rChannelSum:          signed(16 downto 0); 
    signal mixedAudio:           std_logic_vector(31 downto 0);   
  
begin

    M_dma_tLast <= S_Rx_tLast; 
-- need to add data always 
    lChannelSum <=  resize(signed(tDataReg(15 downto 0)), lChannelSum'length)  + resize(signed(tRecReg(15 downto 0)), lChannelSum'length); 
    rChannelSum <= resize(signed(tDataReg(31 downto 16)), rChannelSum'length) + resize(signed(tRecReg(31 downto 16)), rChannelSum'length); 
    mixedAudio <= std_logic_vector(rChannelSum(16 downto 1)) & std_logic_vector(lChannelSum(16 downto 1)); 
    

    
-- Synchronous Logic
    process(clk)
        -- No variables to declare.
    begin
        if(rising_edge(clk)) then
            if(reset = '0') then -- Active low synchronous reset.
                PS <= Idle;
                tDataReg <= x"00000000"; 
                tRecReg <= x"00000000";
            else 
                PS <= NS; 
                -- load data into corresponding registers 
                if (tDataLoad = '1') then 
                    tDataReg <= S_Rx_tData;
                end if;
                if (tRecLoad = '1') then 
                    tRecReg <= S_Rec_tData; -- don't need the playInterrupt bit  
                end if; 
            end if; -- if (reset = 0) 
       end if; -- if rising_edge(clk)    
  end process; 
  
  -- output MUX 
  process(mixAudio, mixedAudio, tDataReg) 
  -- No variables to declare 
  begin 
    if (mixAudio = '1') then 
        M_dma_tData <= mixedAudio; 
    else 
        M_dma_tData <= tDataReg; 
    end if; 
  end process; 
  
  -- IFL and OFL 
  process(PS, S_Rec_tValid, S_Rx_tValid, playInterrupt) 
  begin 
    -- default values to avoid latching 
        tDataLoad <= '0';
        mixAudio <= '0';
        tRecLoad <= '0'; 
        S_Rx_tReady <= '0';              
        M_dma_tValid <= '0'; 
        case PS is 
        
            when Idle => 
                tDataLoad <= '1'; 
                tRecLoad <= '1'; 
                S_Rx_tReady <= '1'; 
                    if (S_Rx_tValid = '1' and playInterrupt = '0') then 
                           NS <= sendOriginal; 
                    elsif ((S_Rx_tValid = '1') and (playInterrupt = '1') and (S_Rec_tValid = '1')) then 
                           NS <= Mix; 
                    else 
                            NS <= PS; 
                    end if; 
               
          when sendOriginal => 
                M_dma_tValid <= '1';
                NS <= Idle; 
                
         when Mix => -- when we are in the Mix state 
            mixAudio <= '1'; 
            M_dma_tValid <= '1';
            NS <= Idle;  
            
        when others => -- should never get here 
            NS <= Idle; 
     
     end case;  
  end process; 

end Behavioral;
