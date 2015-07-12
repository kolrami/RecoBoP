library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library reconos_v3_01_a;
use reconos_v3_01_a.reconos_pkg.all;
use reconos_v3_01_a.reconos_calls.all;

library rt_vga_v1_00_a;
use rt_vga_v1_00_a.reconos_thread.all;

entity rt_vga is
	port (
		-- OSIF FIFO ports
		OSIF_Sw2Hw_Data    : in  std_logic_vector(31 downto 0);
		OSIF_Sw2Hw_Empty   : in  std_logic;
		OSIF_Sw2Hw_RE      : out std_logic;

		OSIF_Hw2Sw_Data    : out std_logic_vector(31 downto 0);
		OSIF_Hw2Sw_Full    : in  std_logic;
		OSIF_Hw2Sw_WE      : out std_logic;

		-- MEMIF FIFO ports
		MEMIF_Hwt2Mem_Data    : out std_logic_vector(31 downto 0);
		MEMIF_Hwt2Mem_Full    : in  std_logic;
		MEMIF_Hwt2Mem_WE      : out std_logic;

		MEMIF_Mem2Hwt_Data    : in  std_logic_vector(31 downto 0);
		MEMIF_Mem2Hwt_Empty   : in  std_logic;
		MEMIF_Mem2Hwt_RE      : out std_logic;

		HWT_Clk    : in  std_logic;
		HWT_Rst    : in  std_logic;
		HWT_Signal : in  std_logic;

		VGA_Red   : out std_logic_vector(3 downto 0);
		VGA_Green : out std_logic_vector(3 downto 0);
		VGA_Blue  : out std_logic_vector(3 downto 0);
		VGA_HSync : out std_logic;
		VGA_VSync : out std_logic
	);
end entity rt_vga;

architecture implementation of rt_vga is
	signal i_osif  : i_osif_t;
	signal o_osif  : o_osif_t;
	signal i_memif : i_memif_t;
	signal o_memif : o_memif_t;

	type STATE_TYPE is (STATE_THREAD_INIT,
	                    STATE_READ);
	signal state : STATE_TYPE;

	signal h_count, v_count : unsigned(31 downto 0);

	signal pos          : std_logic_vector(31 downto 0);
	signal pos_x, pos_y : unsigned(15 downto 0);

	signal ignore : std_logic_vector(31 downto 0);
begin
	osif_setup (
		i_osif,
		o_osif,
		OSIF_Sw2Hw_Data,
		OSIF_Sw2Hw_Empty,
		OSIF_Sw2Hw_RE,
		OSIF_Hw2Sw_Data,
		OSIF_Hw2Sw_Full,
		OSIF_Hw2Sw_WE
	);

	memif_setup (
		i_memif,
		o_memif,
		MEMIF_Mem2Hwt_Data,
		MEMIF_Mem2Hwt_Empty,
		MEMIF_Mem2Hwt_RE,
		MEMIF_Hwt2Mem_Data,
		MEMIF_Hwt2Mem_Full,
		MEMIF_Hwt2Mem_WE
	);

	osfsm_proc: process (HWT_Clk,HWT_Rst,o_osif,o_memif) is
		variable resume, done : boolean;
	begin
		if HWT_Rst = '1' then
			osif_reset(o_osif);
			memif_reset(o_memif);

			state <= STATE_THREAD_INIT;
		elsif rising_edge(HWT_Clk) then
			case state is
				when STATE_THREAD_INIT =>
					THREAD_INIT(i_osif, o_osif, resume, done);
					if done then
						state <= STATE_READ;
					end if;

				when STATE_READ =>
					MBOX_GET(i_osif, o_osif, touch_pos, pos, done);
			
				when others =>

			end case;
		end if;
	end process osfsm_proc;

	pos_x <= unsigned(pos(31 downto 16));
	pos_y <= unsigned(pos(15 downto 0));

	vc_inst : entity rt_vga_v1_00_a.vga_controller
		generic map (
			G_VC_CLK_PRD  => 10ns,
			G_VGA_CLK_PRD => 20ns,

			G_H_DISPLAY => 800,
			G_H_FRONT   => 56,
			G_H_SYNC    => 120,
			G_H_BACK    => 64,
			G_H_POL     => '1',
			G_V_DISPLAY => 600,
			G_V_FRONT   => 37,
			G_V_SYNC    => 6,
			G_V_BACK    => 23,
			G_V_POL     => '1'
		)
		port map (
			VGA_HSync  => VGA_HSync,
			VGA_VSync  => VGA_VSync,
			VGA_HCount => h_count,
			VGA_VCount => v_count,

			VC_Clk => HWT_Clk
		);

	VGA_Red   <= (others => '0') when h_count = pos_x and v_count = pos_y else
	             (others => '1');
	VGA_Green <= (others => '0') when h_count = pos_x and v_count = pos_y else
	             (others => '1');
	VGA_Blue  <= (others => '0') when h_count = pos_x and v_count = pos_y else
	             (others => '1');

end architecture;




