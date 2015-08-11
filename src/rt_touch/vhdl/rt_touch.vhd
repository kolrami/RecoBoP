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
		TC_SSn   : out std_logic;
		TC_IRQ   : in  std_logic := '0'
	);
end entity rt_touch;

architecture implementation of rt_touch is
	signal i_osif  : i_osif_t;
	signal o_osif  : o_osif_t;
	signal i_memif : i_memif_t;
	signal o_memif : o_memif_t;

	constant C_AVG_COUNT     : integer := 4;
	constant C_AVG_COUNT_LOG : integer := 2;

	type STATE_TYPE is (STATE_THREAD_INIT, STATE_INIT_DATA,
	                    STATE_WAIT, STATE_CTRL, STATE_IRQ_START, STATE_IRQ_END,
	                    STATE_START_X, STATE_READ_X, STATE_AVG_X,
	                    STATE_START_Y, STATE_READ_Y, STATE_AVG_Y,
	                    STATE_POS, STATE_SCALE,
	                    STATE_STORE_POS, STATE_STORE_DELTA,
	                    STATE_SAW, STATE_PERF_BEGIN, STATE_PERF_END);
	signal state : STATE_TYPE;

	signal irq_s_0, irq_s_1 : std_logic;

	signal rb_info : unsigned(31 downto 0);

	signal sm_start, sm_ready, sm_conti : std_logic;
	signal sm_txdata, sm_rxdata         : std_logic_vector(23 downto 0);
	
	signal wait_count : unsigned(23 downto 0);
	signal irq_count  : unsigned(23 downto 0);

	signal x_pos_sum, y_pos_sum : unsigned(11 + C_AVG_COUNT_LOG downto 0);
	signal pos_sum_count        : unsigned(C_AVG_COUNT_LOG downto 0);

	signal x_pos_avg, y_pos_avg : std_logic_vector(11 downto 0);
	signal x_pos_s, y_pos_s     : std_logic_vector(11 downto 0);

	signal ctrl_wait : unsigned(23 downto 0);
	signal ctrl_avg : std_logic_vector(3 downto 0);

	signal ignore, ret : std_logic_vector(31 downto 0);

	signal scale_start, scale_done, scale_idle, scale_ready : std_logic;
	signal scale_x_pos_s, scale_y_pos_s                     : std_logic_vector(11 downto 0);
	signal scale_x_pos_s_vld, scale_y_pos_s_vld             : std_logic;

	component scale is
		port (
			ap_clk   : in  std_logic;
			ap_rst   : in  std_logic;
			ap_start : in  std_logic;
			ap_done  : out std_logic;
			ap_idle  : out std_logic;
			ap_ready : out std_logic;

			x_u_V        : in std_logic_vector (11 downto 0);
			y_u_V        : in std_logic_vector (11 downto 0);
			x_s_V        : out std_logic_vector (11 downto 0);
			x_s_V_ap_vld : out std_logic;
			y_s_V        : out std_logic_vector (11 downto 0);
			y_s_V_ap_vld : out std_logic
		);
	end component;
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

			wait_count    <= (others => '0');
			pos_sum_count <= (others => '0');

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
						wait_count <= (others => '0');

						state <= STATE_CTRL;
					end if;

				when STATE_CTRL =>
					MEM_READ_WORD(i_memif, o_memif, std_logic_vector(rb_info + 16), ret, done);
					if done then
						ctrl_wait <= unsigned(ret(23 downto 0));
						ctrl_avg  <= ret(27 downto 24);

						x_pos_sum <= (others => '0');
						y_pos_sum <= (others => '0');

						state <= STATE_WAIT;
					end if;

				when STATE_WAIT =>
					if wait_count = ctrl_wait then
						state <= STATE_PERF_BEGIN;
					end if;

				when STATE_PERF_BEGIN =>
					ctrl_wait <= ctrl_wait + 1;

					MBOX_PUT(i_osif, o_osif, performance_perf, x"00000000", ignore, done);
					if done then
						irq_count <= (others => '0');
						state <= STATE_IRQ_START;
					end if;

				when STATE_IRQ_START =>
					ctrl_wait <= ctrl_wait + 1;

					if irq_s_0 = '0' then
						irq_count <= irq_count + 1;
					else
						irq_count <= (others => '0');
					end if;

					if irq_count = 9999 then
						state <= STATE_START_X;
					end if;

				when STATE_START_X =>
					ctrl_wait <= ctrl_wait + 1;

					state <= STATE_READ_X;

				when STATE_READ_X =>
					ctrl_wait <= ctrl_wait + 1;

					if sm_ready = '1' then
						if pos_sum_count /= 0 then
							x_pos_sum <= x_pos_sum + unsigned(sm_rxdata(14 downto 3));
						end if;

						state <= STATE_AVG_X;
					end if;

				when STATE_AVG_X =>
					ctrl_wait <= ctrl_wait + 1;

					pos_sum_count <= pos_sum_count + 1;

					if pos_sum_count = C_AVG_COUNT then
						pos_sum_count <= (others => '0');
						state <= STATE_START_Y;
					else
						state <= STATE_START_X;
					end if;

				when STATE_START_Y =>
					state <= STATE_READ_Y;

				when STATE_READ_Y =>
					if sm_ready = '1' then
						if pos_sum_count /= 0 then
							y_pos_sum <= y_pos_sum + unsigned(sm_rxdata(14 downto 3));
						end if;

						state <= STATE_AVG_Y;
					end if;

				when STATE_AVG_Y =>
					pos_sum_count <= pos_sum_count + 1;

					if pos_sum_count = C_AVG_COUNT then
						pos_sum_count <= (others => '0');
						state <= STATE_IRQ_END;
					else
						state <= STATE_START_Y;
					end if;

				when STATE_IRQ_END =>
					if irq_s_0 = '0' then
						state <= STATE_SCALE;
						wait_count <= (others => '0');
					else
						irq_count <= (others => '0');
						state <= STATE_IRQ_START;
					end if;

				when STATE_SCALE =>
					if scale_done = '1' then
						state <= STATE_PERF_END;
					end if;

					if scale_x_pos_s_vld = '1' then
						x_pos_s <= scale_x_pos_s;
					end if;

					if scale_y_pos_s_vld = '1' then
						y_pos_s <= scale_y_pos_s;
					end if;

				when STATE_PERF_END =>
					MBOX_PUT(i_osif, o_osif, performance_perf, x"01000000", ignore, done);
					if done then
						state <= STATE_STORE_POS;
					end if;

				when STATE_STORE_POS =>
					MBOX_PUT(i_osif, o_osif, touch_pos, x"00" & x_pos_s & y_pos_s, ignore, done);

					if done then
						state <= STATE_STORE_DELTA;
					end if;

				when STATE_STORE_DELTA =>
					MBOX_PUT(i_osif, o_osif, touch_pos, x"00" & std_logic_vector(ctrl_wait), ignore, done);

					if done then
						state <= STATE_SAW;
					end if;

				when STATE_SAW =>
					MEM_WRITE_WORD(i_memif, o_memif, std_logic_vector(rb_info + 12), x"00" & x_pos_s & y_pos_s, done);
					if done then
						state <= STATE_CTRL;
					end if;

				when others =>

			end case;
		end if;
	end process osfsm_proc;

	x_pos_avg <= std_logic_vector(x_pos_sum(11 + C_AVG_COUNT_LOG downto C_AVG_COUNT_LOG));
	y_pos_avg <= std_logic_vector(y_pos_sum(11 + C_AVG_COUNT_LOG downto C_AVG_COUNT_LOG));

	sync_proc : process(HWT_Clk) is
	begin
		if rising_edge(HWT_Clk) then
			irq_s_1 <= TC_IRQ;
			irq_s_0 <= irq_s_1;
		end if;
	end process sync_proc;

	sm_start <= '1' when state = STATE_START_X else
	            '1' when state = STATE_START_Y else
	            '0';
	sm_conti <= '0' when pos_sum_count = C_AVG_COUNT else
	            '1';
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
	sm_txdata <= "11010000" & x"00" & "00000000" when state = STATE_START_X and pos_sum_count = C_AVG_COUNT else
	             "11010000" & x"00" & "11010000" when state = STATE_START_X else
	             "10010000" & x"00" & "00000000" when state = STATE_START_Y and pos_sum_count = C_AVG_COUNT else
	             "10010000" & x"00" & "10010000" when state = STATE_START_Y else
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
			SM_Conti  => sm_conti,
			SM_Clk    => HWT_Clk
		);

	scale_start <= '1' when state = STATE_SCALE else '0';

	scale_inst : scale
		port map (
			ap_clk => HWT_Clk,
			ap_rst => HWT_Rst,
			ap_start => scale_start,
			ap_done  => scale_done,
			ap_idle  => scale_idle,
			ap_ready => scale_ready,
			x_u_V => x_pos_avg,
			y_u_V => y_pos_avg,
			x_s_V => scale_x_pos_s,
			x_s_V_ap_vld => scale_x_pos_s_vld,
			y_s_V => scale_y_pos_s,
			y_s_V_ap_vld => scale_y_pos_s_vld
		);

end architecture;
