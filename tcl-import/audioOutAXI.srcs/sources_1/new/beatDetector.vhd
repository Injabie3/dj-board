----------------------------------------------------------------------------------
-- Company: N/A
-- Engineer: Ryan Lui 
-- 
-- Create Date: 03/14/2018 02:37:12 PM
-- Design Name: 
-- Module Name: beatDetector - Behavioural
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

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity beatDetector is
    generic(
        bin:        integer := 4;
        threshold:  integer := 4
    );
    port (
        reset:      in std_logic;   -- Active low reset.
        clk:        in std_logic;
        
        isFft:      in std_logic; -- Take a slice from gpioFftConfig.
        led:        out std_logic;
        
        -- ##########################
        -- # AXI-STREAM PASSTHROUGH #
        -- ##########################
        S_fft_tData:        in std_logic_vector(63 downto 0);
        S_fft_tReady:       out std_logic;
        S_fft_tValid:       in std_logic;
        S_fft_tLast:        in std_logic;
        
        M_dma_tData:        out std_logic_vector(63 downto 0);
        M_dma_tReady:       in std_logic;
        M_dma_tValid:       out std_logic;
        M_dma_tLast:        out std_logic;

        -- ############################
        -- # AXI-STREAM CONFIGURATION #
        -- ############################
        -- Bits 45 downto 32:   Bin Number              (unsigned - 13 bits)
        -- Bits 31 downto 0:    Magnitude Threshold     (unsigned - 32 bits)
        S_config_tData:     in std_logic_vector(44 downto 0);
        S_config_tValid:    in std_logic;
        
        -- ##############################
        -- # COMPLEX MULTIPLIER SIGNALS #
        -- ##############################
        M_complexA_tData:   out std_logic_vector(31 downto 0);
        M_complexB_tData:   out std_logic_vector(31 downto 0);
        M_complexA_tValid:  out std_logic;
        M_complexB_tValid:  out std_logic;
        
        S_complexO_tData:   in std_logic_vector(63 downto 0);
        S_complexO_tValid:  in std_logic;
        S_complexO_tReady:  out std_logic
        

    );
end beatDetector;

architecture behavioural of beatDetector is

    type States is (Idle, GetBin, SendComplex, StallComplex, SaveMagnitude, Compare, StallValid);
    
    signal PS, NS:          States; -- Present State and Next State, respectively.
    
    -- #############
    -- # REGISTERS #
    -- #############
    signal counterReg:          unsigned(12 downto 0) := to_unsigned(0, 13); -- 2^13 is 8192.  We could use 256 here.
    signal binReg:              unsigned(12 downto 0) := to_unsigned(4, 13);
    signal tDataReg:            std_logic_vector(31 downto 0);
    signal magReg:              unsigned(31 downto 0);
    signal thresholdReg:        unsigned(31 downto 0);
    signal ledReg:              std_logic; -- Only keeping 1 LED on in this case, you can use more in the upper layers.

    -- ####################
    -- # INTERNAL SIGNALS #
    -- ####################
    -- signal tDataComplexOut:     std_logic_vector
    signal ledDReg:             std_logic;
    
    
    -- ###################
    -- # CONTROL SIGNALS #
    -- ###################
    signal tDataLoad:           std_logic;
    signal counterLoad:         std_logic;
    signal counterInit:         std_logic; -- We need this to reset the counter before every iteration.
    signal tReadyComplexOut:    std_logic;
    signal magLoad:             std_logic;
    signal ledLoad:             std_logic;
    
begin

    -- The AXI-Stream interface is just a passthrough
    -- We could also take a tap off of it, which may save some resources, but we can
    -- change this afterwards nbd.
    M_dma_tData     <= S_fft_tData;
    S_fft_tReady    <= M_dma_tReady;
    M_dma_tValid    <= S_fft_tValid;
    M_dma_tLast     <= S_fft_tLast;
    led         <= ledReg;
    
    -- The following process will help us get a+jb and a-jb
    process (tDataReg)
        variable realValue:         signed(15 downto 0);
        variable imaginaryValue:    signed(15 downto 0);
    begin
        -- Nothing changes for this part.
        -- This is a+jb
        M_complexA_tData <= tDataReg;
        
        -- For the other input to the complex multiplier, we will negate the imaginary part.
        -- This will be a-jb
        realValue := signed(tDataReg(15 downto 0));
        imaginaryValue := signed(tDataReg(31 downto 16));
        
        -- Negate the imaginary part.
        imaginaryValue := -imaginaryValue;
        
        M_complexB_tData <= std_logic_vector(imaginaryValue) & std_logic_vector(realValue);
        
    end process;
    
    -- Synchronous Logic
    process(clk)
        -- No variables to declare.
    begin
        if(rising_edge(clk)) then
            if(reset = '0') then -- Active low synchronous reset.
                PS <= Idle;
                counterReg      <= to_unsigned(0, counterReg'length);
                binReg          <= to_unsigned(4, binReg'length);
                tDataReg        <= x"00000000";
                magReg          <= to_unsigned(0, magReg'length);
                thresholdReg    <= to_unsigned(2097152, thresholdReg'length); -- 2^21
                ledReg          <= '0';
                
            else
                PS <= NS;
                if counterInit = '1' then
                    counterReg <= to_unsigned(0, counterReg'length);
                elsif counterLoad = '1' then
                    counterReg <= counterReg + 1;
                end if;
                
                if tDataLoad = '1' then
                    tDataReg <= S_fft_tData(31 downto 0); -- Only take one channel.
                end if;
                
                if magLoad = '1' then
                    magReg <= unsigned(S_complexO_tData(31 downto 0));
                end if;
                
                if S_config_tValid ='1' then
                    binReg          <= unsigned(S_config_tData(44 downto 32));
                    thresholdReg    <= unsigned(S_config_tData(31 downto 0));
                end if;
                
                if ledLoad = '1' then
                    ledReg <= ledDReg;
                end if;
            end if; -- Active low synchronous reset.
        end if;
    end process;
    
    
    -- Input Forming Logic (IFL), Output Forming Logic (OFL)
    process(PS, S_fft_tValid, isFft, counterReg, S_complexO_tValid)
        -- No variables to declare.
    begin
        -- Default values to avoid latching
        tDataLoad           <= '0';
        counterInit         <= '0';
        counterLoad         <= '0';
        M_complexA_tValid   <= '0';
        M_complexB_tValid   <= '0';
        S_complexO_TReady   <= '0';
        magLoad             <= '0';
        ledLoad             <= '0';
        ledDReg             <= '0';
        
        case PS is
            when Idle =>
                tDataLoad <= '1';
                counterInit <= '1';
                
                if (S_fft_tValid = '1' and isFft = '1') then
                    NS <= GetBin;
                else
                    NS <= Idle;
                end if;
                
            when GetBin =>
                tDataLoad <= '1';
                counterLoad <= '1';
                
                if (counterReg < binReg) then
                    NS <= GetBin;
                else -- counterReg >= bin
                    NS <= SendComplex;
                end if;
            
            when SendComplex =>
                M_complexA_tValid <= '1';
                M_complexB_tValid <= '1';
                NS <= StallComplex;
             
            when StallComplex => -- Wait for the complex multiplier to give a result.
                if (S_complexO_tValid = '1') then
                    NS <= SaveMagnitude;
                else -- complexTValidOut = '0'
                    NS <= StallComplex;
                end if;
                
            when SaveMagnitude =>
                S_complexO_tReady <= '1';  -- Do a transaction at the next clock cycle.
                magLoad <= '1';
                NS <= Compare;
                
            when Compare =>
                ledLoad <= '1';
                
                if (magReg > thresholdReg) then
                    ledDReg <= '1';
                else -- magReg <= thresholdReg
                    ledDReg <= '0';
                end if;
                
                NS <= StallValid;
                
            when StallValid =>
                if (S_fft_tValid = '1') then
                    NS <= StallValid;
                else -- S_tValid = '0'
                    NS <= Idle;
                end if;
            
            when others => -- We should not end up in this state, but just as a precaution.
                NS <= Idle;
            
        end case;
    end process;
    
    

end behavioural;