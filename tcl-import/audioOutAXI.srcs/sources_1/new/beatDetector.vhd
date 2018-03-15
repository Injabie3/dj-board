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
        S_tData:    in std_logic_vector(63 downto 0);
        S_tReady:   out std_logic;
        S_tValid:   in std_logic;
        S_tLast:    in std_logic;
        M_tData:    out std_logic_vector(63 downto 0);
        M_tReady:   in std_logic;
        M_tValid:   out std_logic;
        M_tLast:    out std_logic;
        isFft:      in std_logic; -- Take a slice from gpioFftConfig.
        led:        out std_logic;
        
        -- Signals to and from the complex multiplier
        complexATDataIn:    out std_logic_vector(31 downto 0);
        complexBTDataIn:    out std_logic_vector(31 downto 0);
        complexTValidIn:    out std_logic;
        
        complexTDataOut:    in std_logic_vector(31 downto 0);
        complexTValidOut:   in std_logic;
        complexTReadyOut:   out std_logic;
        
        reset:      in std_logic;   -- Active low reset.
        clk:        in std_logic
    );
end beatDetector;

architecture behavioural of beatDetector is

    type States is (Idle, GetBin, SendComplex, StallComplex, SaveMagnitude, Compare, StallValid);
    
    signal PS, NS:          States; -- Present State and Next State, respectively.
    
    -- #############
    -- # REGISTERS #
    -- #############
    signal counterReg:          unsigned(8 downto 0) := to_unsigned(0,9); -- 2^9 is 512.  We could use 256 here.
    signal tDataReg:            std_logic_vector(31 downto 0);
    signal magReg:              unsigned(31 downto 0);  
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
    M_tData     <= S_tData;
    S_tReady    <= M_tReady;
    M_tValid    <= S_tValid;
    M_tLast     <= S_tLast;
    led         <= ledReg;
    led         <= ledReg;
    
    -- The following process will help us get a+jb and a-jb
    process (tDataReg)
        variable realValue:         signed(15 downto 0);
        variable imaginaryValue:    signed(15 downto 0);
    begin
        -- Nothing changes for this part.
        -- This is a+jb
        complexATDataIn <= tDataReg;
        
        -- For the other input to the complex multiplier, we will negate the imaginary part.
        -- This will be a-jb
        realValue := signed(tDataReg(15 downto 0));
        imaginaryValue := signed(tDataReg(31 downto 16));
        
        -- Negate the imaginary part.
        imaginaryValue := -imaginaryValue;
        
        complexBTDataIn <= std_logic_vector(imaginaryValue) & std_logic_vector(realValue);
        
    end process;
    
    -- Synchronous Logic
    process(clk)
        -- No variables to declare.
    begin
        if(rising_edge(clk)) then
            if(reset = '0') then -- Active low synchronous reset.
                PS <= Idle;
                counterReg  <= to_unsigned(0, counterReg'length);
                tDataReg    <= x"00000000";
                magReg      <= to_unsigned(0, magReg'length);
                ledReg      <= '0';
                
            else
                PS <= NS;
                if counterInit = '1' then
                    counterReg <= to_unsigned(0, counterReg'length);
                elsif counterLoad = '1' then
                    counterReg <= counterReg + 1;
                end if;
                
                if tDataLoad = '1' then
                    tDataReg <= S_tData(31 downto 0); -- Only take one channel.
                end if;
                
                if magLoad = '1' then
                    magReg <= unsigned(complexTDataOut);
                end if;
                
                if ledLoad = '1' then
                    ledReg <= ledDReg;
                end if;
            end if; -- Active low synchronous reset.
        end if;
    end process;
    
    
    -- Input Forming Logic (IFL), Output Forming Logic (OFL)
    process(PS, S_tValid, isFft, counterReg, complexTValidOut)
        -- No variables to declare.
    begin
        -- Default values to avoid latching
        tDataLoad           <= '0';
        counterLoad         <= '0';
        complexTValidIn     <= '0';
        complexTReadyOut    <= '0';
        magLoad             <= '0';
        ledLoad             <= '0';
        ledDReg             <= '0';
        
        case PS is
            when Idle =>
                tDataLoad <= '1';
                
                if (S_tValid = '1' and isFft = '1') then
                    NS <= GetBin;
                else
                    NS <= Idle;
                end if;
                
            when GetBin =>
                tDataLoad <= '1';
                counterLoad <= '1';
                
                if (counterReg < bin) then
                    NS <= GetBin;
                else -- counterReg >= bin
                    NS <= SendComplex;
                end if;
            
            when SendComplex =>
                complexTValidIn <= '1';
                NS <= StallComplex;
             
            when StallComplex =>
                if (complexTValidOut = '0') then
                    NS <= StallComplex;
                else -- complexTValidOut = '1'
                    NS <= SaveMagnitude;
                end if;
                
            when SaveMagnitude =>
                complexTReadyOut <= '1';  -- Do a transaction at the next clock cycle.
                magLoad <= '1';
                NS <= Compare;
                
            when Compare =>
                ledLoad <= '1';
                
                if (magReg > threshold) then
                    ledDReg <= '1';
                else -- magReg <= threshold
                    ledDReg <= '0';
                end if;
                
                NS <= StallValid;
                
            when StallValid =>
                if (S_tValid = '1') then
                    NS <= StallValid;
                else -- S_tValid = '0'
                    NS <= Idle;
                end if;
            
            when others => -- We should not end up in this state, but just as a precaution.
                NS <= Idle;
            
        end case;
    end process;
    
    

end behavioural;