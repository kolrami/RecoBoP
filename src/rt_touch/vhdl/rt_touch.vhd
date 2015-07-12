library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library reconos_v3_01_a;
use reconos_v3_01_a.reconos_pkg.all;
use reconos_v3_01_a.reconos_calls.all;

library rt_touch_v1_00_a;
use rt_touch_v1_00_a.reconos_thread.all;

entity rt_touch is
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

		TC_SCLK  : out std_logic;
		TC_MOSI  : out std_logic;
		TC_MISO  : in  std_logic := '0';
		TC_SSn   : out std_logic
	);
end entity rt_touch;

architecture implementation of rt_touch is
	signal i_osif  : i_osif_t;
	signal o_osif  : o_osif_t;
	signal i_memif : i_memif_t;
	signal o_memif : o_memif_t;

	constant C_WAIT_COUNT : integer := 10;

	type STATE_TYPE is (STATE_THREAD_INIT,
	                    STATE_WAIT, STATE_START, STATE_READ,STATE_STORE);
	signal state : STATE_TYPE;

	--subtype C_ANGLE_RANGE is natural range 31 downto 21;

	signal sm_start, sm_ready   : std_logic;
	signal sm_txdata, sm_rxdata : std_logic_vector(23 downto 0);
	
	signal wait_count :  unsigned(31 downto 0);

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

			wait_count <= (others => '0');

			state <= STATE_THREAD_INIT;
		elsif rising_edge(HWT_Clk) then
			case state is
				when STATE_THREAD_INIT =>
					THREAD_INIT(i_osif, o_osif, resume, done);
					if done then
						state <= STATE_WAIT;
					end if;

				when STATE_WAIT =>
					wait_count <= wait_count + 1;

					if wait_count = C_WAIT_COUNT then
						wait_count <= (others => '0');

						state <= STATE_START;
					end if;

				when STATE_START =>
					state <= STATE_READ;

				when STATE_READ =>
					if sm_ready = '1' then
						state <= STATE_STORE;
					end if;

				when STATE_STORE =>
					MBOX_PUT(i_osif, o_osif, touch_pos, sm_rxdata & x"00", ignore, done);

					if done then
						state <= STATE_WAIT;
					end if;

			end case;
		end if;
	end process osfsm_proc;

	sm_start <= '1' when state = STATE_START else '0';

	sm_inst : entity rt_touch_v1_00_a.spi_master
		generic map (
			G_SM_CLK_PRD  => 10ns,
			G_SPI_CLK_PRD => 400ns,

			G_DATA_LEN => 24
		)
		port map (
			SPI_SCLK => TC_SCLK,
			SPI_MOSI => TC_MOSI,
			SPI_MISO => TC_MISO,
			SPI_SSn  => TC_SSn,

			SM_TxData => sm_txdata,
			SM_RxData => sm_rxdata,
			SM_Start  => sm_start,
			SM_Ready  => sm_ready,
			SM_Clk    => HWT_Clk
		);

end architecture;
