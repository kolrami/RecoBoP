library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library reconos_v3_01_a;
use reconos_v3_01_a.reconos_pkg.all;
use reconos_v3_01_a.reconos_calls.all;

library rt_performance_v1_00_a;
use rt_performance_v1_00_a.reconos_thread.all;

entity rt_performance is
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
		HWT_Signal : in  std_logic
	);
end entity rt_performance;

architecture implementation of rt_performance is
	signal i_osif  : i_osif_t;
	signal o_osif  : o_osif_t;
	signal i_memif : i_memif_t;
	signal o_memif : o_memif_t;

	subtype  C_RT_RANGE is natural range 31 downto 28;
	subtype  C_PC_RANGE is natural range 27 downto 24;

	constant C_RT_TOUCH   : std_logic_vector(3 downto 0) := x"0";
	constant C_RT_CONTROL : std_logic_vector(3 downto 0) := x"1";
	constant C_RT_INVERSE : std_logic_vector(3 downto 0) := x"2";
	constant C_RT_SERVO   : std_logic_vector(3 downto 0) := x"3";

	constant C_PC_BEGIN   : std_logic_vector(3 downto 0) := x"0";
	constant C_PC_END     : std_logic_vector(3 downto 0) := x"1";

	type STATE_TYPE is (STATE_THREAD_INIT, STATE_INIT_DATA,
	                    STATE_RECV, STATE_STORE);
	signal state : STATE_TYPE;

	signal rb_info : unsigned(31 downto 0);

	signal perf_cmd : std_logic_vector(31 downto 0);
	signal perf_cnt : unsigned(31 downto 0);

	signal rt_touch_begin, rt_touch_end     : unsigned(31 downto 0);
	signal rt_control_begin, rt_control_end : unsigned(31 downto 0);
	signal rt_inverse_begin, rt_inverse_end : unsigned(31 downto 0);
	signal rt_servo_begin, rt_servo_end     : unsigned(31 downto 0);

	signal ignore, ret : std_logic_vector(31 downto 0);

	signal counter : unsigned(31 downto 0) := (others => '0');
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

		variable rt_inverse_begin_leg, rt_inverse_end_leg : std_logic_vector(5 downto 0) := (others => '0');
		variable rt_servo_leg : std_logic_vector(5 downto 0) := (others => '0');
	begin
		if HWT_Rst = '1' then
			osif_reset(o_osif);
			memif_reset(o_memif);

			rt_inverse_begin_leg := (others => '0');
			rt_inverse_end_leg   := (others => '0');

			rt_servo_leg := (others => '0');

			state <= STATE_THREAD_INIT;
		elsif rising_edge(HWT_Clk) then
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

						state <= STATE_RECV;
					end if;

				when STATE_RECV =>
					MBOX_GET(i_osif, o_osif, performance_perf, perf_cmd, done);
					if done then
						perf_cnt <= counter;

						state <= STATE_STORE;
					end if;

				when STATE_STORE =>
					done := false;

					case perf_cmd(31 downto 24) is
						when C_RT_TOUCH & C_PC_BEGIN =>
							rt_touch_begin <= perf_cnt;
							done := true;

						when C_RT_TOUCH & C_PC_END =>
							rt_touch_end <= perf_cnt;
							MEM_WRITE_WORD(i_memif, o_memif, std_logic_vector(rb_info + 20), std_logic_vector(perf_cnt - rt_touch_begin), done);

						when C_RT_CONTROL & C_PC_BEGIN =>
							rt_control_begin <= perf_cnt;				
							done := true;

						when C_RT_CONTROL & C_PC_END =>
							rt_control_end <= perf_cnt;
							MEM_WRITE_WORD(i_memif, o_memif, std_logic_vector(rb_info + 24), std_logic_vector(perf_cnt - rt_control_begin), done);

						when C_RT_INVERSE & C_PC_BEGIN =>
							if rt_inverse_begin_leg = "000000" then
								rt_inverse_begin <= perf_cnt;

								rt_inverse_end_leg := (others => '0');
								rt_servo_leg       := (others => '0');
							end if;
							rt_inverse_begin_leg(to_integer(unsigned(perf_cmd(2 downto 0)))) := '1';
							done := true;

						when C_RT_INVERSE & C_PC_END =>
							rt_inverse_end_leg(to_integer(unsigned(perf_cmd(2 downto 0)))) := '1';
							if rt_inverse_end_leg = "111111" then
								rt_inverse_end <= perf_cnt;
								MEM_WRITE_WORD(i_memif, o_memif, std_logic_vector(rb_info + 28), std_logic_vector(perf_cnt - rt_inverse_begin), done);

								rt_inverse_begin_leg := (others => '0');
							else
								done := true;
							end if;

						when C_RT_SERVO & C_PC_BEGIN =>
							rt_servo_begin <= perf_cnt;
							done := true;

						when C_RT_SERVO & C_PC_END =>
							rt_servo_leg(to_integer(unsigned(perf_cmd(2 downto 0)))) := '1';
							if rt_servo_leg = "111111" then
								rt_servo_end <= perf_cnt;
								MEM_WRITE_WORD(i_memif, o_memif, std_logic_vector(rb_info + 32), std_logic_vector(perf_cnt - rt_touch_begin), done);
							else
								done := true;
							end if;

						when others =>
							done := true;
					end case;

					if done then
						state <= STATE_RECV;
					end if;

				when others =>
			end case;
		end if;
	end process osfsm_proc;

	counter_proc: process (HWT_Clk) is
	begin
		if rising_edge(HWT_Clk) then
			counter <= counter + 1;
		end if;
	end process counter_proc;

end architecture;
