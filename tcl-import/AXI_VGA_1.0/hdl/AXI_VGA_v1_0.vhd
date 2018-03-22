library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity AXI_VGA_v1_0 is
	generic (
		-- Users to add parameters here

		-- User parameters ends
		-- Do not modify the parameters beyond this line


		-- Parameters of Axi Slave Bus Interface S00_AXIS
		C_S00_AXIS_TDATA_WIDTH	: integer	:= 32
	);
	port (
		-- Users to add ports here
        H_SYNC : out STD_LOGIC;
        V_SYNC : out STD_LOGIC;
        RED : out STD_LOGIC_VECTOR(3 downto 0);
        GREEN : out STD_LOGIC_VECTOR(3 downto 0);
        BLUE : out STD_LOGIC_VECTOR(3 downto 0);
		-- User ports ends
		-- Do not modify the ports beyond this line


		-- Ports of Axi Slave Bus Interface S00_AXIS
		s00_axis_aclk	: in std_logic;
		s00_axis_aresetn	: in std_logic;
		s00_axis_tready	: out std_logic;
		s00_axis_tdata	: in std_logic_vector(C_S00_AXIS_TDATA_WIDTH-1 downto 0);
		s00_axis_tstrb	: in std_logic_vector((C_S00_AXIS_TDATA_WIDTH/8)-1 downto 0);
		s00_axis_tlast	: in std_logic;
		s00_axis_tvalid	: in std_logic
	);
end AXI_VGA_v1_0;

architecture arch_imp of AXI_VGA_v1_0 is

	-- component declaration
	component clk_wiz_0 is
		port (
		clk_in1 : in STD_LOGIC;
		clk_out1: out STD_LOGIC
		);
	end component;
	
	component fifo_generator_0 is
        port (
        rst : IN STD_LOGIC;
        wr_clk : IN STD_LOGIC;
        rd_clk : IN STD_LOGIC;
        din : IN STD_LOGIC_VECTOR(23 DOWNTO 0);
        wr_en : IN STD_LOGIC;
        rd_en : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR(11 DOWNTO 0);
        full : OUT STD_LOGIC;
        empty : OUT STD_LOGIC;
        wr_rst_busy : OUT STD_LOGIC;
        rd_rst_busy : OUT STD_LOGIC
      );
    end component;
    
    component VGA_Controller is
        port (
        CLK : in STD_LOGIC;
        RESETN : in STD_LOGIC;
        DATA : in STD_LOGIC_VECTOR (11 downto 0);
        FIFO_BSY : in STD_LOGIC;
        FIFO_EMPTY : in STD_LOGIC;
        HSYNC : out STD_LOGIC;
        VSYNC : out STD_LOGIC;
        R : out STD_LOGIC_VECTOR (3 downto 0);
        G : out STD_LOGIC_VECTOR (3 downto 0);
        B : out STD_LOGIC_VECTOR (3 downto 0);
        FIFO_EN : out STD_LOGIC
        );
    end component;

    signal clk_100MHz, clk_25MHz : STD_LOGIC;
    signal fifo_reset_sig : STD_LOGIC;
    signal fifo_wren, fifo_ren : STD_LOGIC;
    signal write_busy, read_busy : STD_LOGIC;
    signal full_sig, empty_sig : STD_LOGIC;
    
    signal double_data : STD_LOGIC_VECTOR(23 downto 0);
    signal vid_data : STD_LOGIC_VECTOR(11 downto 0);
    
    signal ready_sig : STD_LOGIC;
begin

-- Instantiation of Axi Bus Interface S00_AXIS
clock_switch : clk_wiz_0
	port map (
		clk_in1	=> s00_axis_aclk,
		clk_out1 => clk_25MHz
	);
	
pixel_buffer : fifo_generator_0
    port map(
        rst => fifo_reset_sig,
        wr_clk => s00_axis_aclk,
        rd_clk => clk_25MHz,
        din => double_data,
        wr_en => fifo_wren,
        rd_en => fifo_ren,
        dout => vid_data,
        full => full_sig,
        empty => empty_sig,
        wr_rst_busy => write_busy,
        rd_rst_busy => read_busy
    );
 
VGA : VGA_Controller
    port map(
        CLK => clk_25MHz, 
        RESETN => s00_axis_aresetn,
        DATA => vid_data,
        FIFO_BSY => read_busy,
        FIFO_EMPTY => empty_sig,
        HSYNC => H_SYNC,
        VSYNC => V_SYNC,
        R => RED,
        G => GREEN,
        B => BLUE,
        FIFO_EN => fifo_ren
    );
    
    ready_sig <= '1' when ((full_sig = '0') and (write_busy = '0')) else '0';
    
    fifo_reset_sig <= not s00_axis_aresetn;
    fifo_wren <= '1' when ((s00_axis_tvalid = '1') and (ready_sig = '1')) else '0';
    double_data <= s00_axis_tdata(27 downto 16) & s00_axis_tdata(11 downto 0); -- ignore first nibbles
    
    s00_axis_tready <= ready_sig;

end arch_imp;
