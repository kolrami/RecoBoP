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

	constant C_WAIT_COUNT : integer := 10000;

	type STATE_TYPE is (STATE_THREAD_INIT, STATE_INIT_DATA,
	                    STATE_WAIT, STATE_CTRL, STATE_START_X, STATE_READ_X,
	                    STATE_START_Y, STATE_READ_Y, STATE_STORE, STATE_SAW);
	signal state : STATE_TYPE;

	signal rb_info : unsigned(31 downto 0);

	signal sm_start, sm_ready   : std_logic;
	signal sm_txdata, sm_rxdata : std_logic_vector(23 downto 0);
	
	signal wait_count :  unsigned(31 downto 0);

	signal x_pos, y_pos : unsigned(11 downto 0);
	signal pos          : std_logic_vector(31 downto 0);

	signal ctrl_wait : unsigned(31 downto 0);

	signal ignore, ret : std_logic_vector(31 downto 0);
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

	pos <= x"00" & std_logic_vector(x_pos) & std_logic_vector(y_pos);

	osfsm_proc: process (HWT_Clk,HWT_Rst,o_osif,o_memif) is
		variable resume, done : boolean;
	begin
		if HWT_Rst = '1' then
			osif_reset(o_osif);
			memif_reset(o_memif);

			wait_count <= (others => '0');

			state <= STATE_THREAD_INIT;
		elsif rising_edge(HWT_Clk) then
			wait_count <= wait_count + 1;

			case state is
				when STATE_THREAD_INIT =>
					THREAD_INIT(i_osif, o_osif, resume, done);
					if done then
						state <= STATE_INIT_DATA;
					end if;

				when STATE_INIT_DATA =>
					GET_INIT_DATA(i_osif, o_osif, ret, done);
					if done then
						rb_info <= unsigned(ret);

						state <= STATE_CTRL;
					end if;

				when STATE_CTRL =>
					MEM_READ_WORD(i_memif, o_memif, std_logic_vector(rb_info + 16), ret, done);
					if done then
						ctrl_wait <= unsigned(ret);

						state <= STATE_WAIT;
					end if;

				when STATE_WAIT =>
					if wait_count = ctrl_wait then
						state <= STATE_START_X;
					end if;

				when STATE_START_X =>
					state <= STATE_READ_X;

				when STATE_READ_X =>
					if sm_ready = '1' then
						x_pos <= unsigned(sm_rxdata(14 downto 3));

						state <= STATE_START_Y;
					end if;

				when STATE_START_Y =>
					state <= STATE_READ_Y;

				when STATE_READ_Y =>
					if sm_ready = '1' then
						y_pos <= unsigned(sm_rxdata(14 downto 3));

						state <= STATE_STORE;
					end if;

				when STATE_STORE =>
					MBOX_PUT(i_osif, o_osif, touch_pos, pos, ignore, done);

					if done then
						wait_count <= (others => '0');

						state <= STATE_SAW;
					end if;

				when STATE_SAW =>
					MEM_WRITE_WORD(i_memif, o_memif, std_logic_vector(rb_info + 12), pos, done);
					if done then
						state <= STATE_CTRL;
					end if;

			end case;
		end if;
	end process osfsm_proc;

	sm_start <= '1' when state = STATE_START_X else
	            '1' when state = STATE_START_Y else
	            '0';
	-- Start A2 A1 A0 Mode SER PD1 PD0
	--       0  0  1                    = y pos
	--       1  0  1                    = x pos
	--                0                 = 12bit conversion
	--                1                 = 8bit conversion
	--                     0            = internal reference
	--                     1            = external reference
	--                         0   0    = power down
	--                         0   1    = power down without penirq
	--                         1   0    = reserved
	--                         1   1    = no power down
	sm_txdata <= "11010000" & x"0000" when state = STATE_START_X else
	             "10010000" & x"0000" when state = STATE_START_Y else
	             (others => '0');

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
