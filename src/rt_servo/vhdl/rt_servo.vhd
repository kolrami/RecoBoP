library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library reconos_v3_01_a;
use reconos_v3_01_a.reconos_pkg.all;
use reconos_v3_01_a.reconos_calls.all;

library rt_servo_v1_00_a;
use rt_servo_v1_00_a.reconos_thread.all;

entity rt_servo is
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

		SRV0_Out : out std_logic;
		SRV1_Out : out std_logic;
		SRV2_Out : out std_logic;
		SRV3_Out : out std_logic;
		SRV4_Out : out std_logic;
		SRV5_Out : out std_logic
	);
end entity rt_servo;

architecture implementation of rt_servo is
	signal i_osif  : i_osif_t;
	signal o_osif  : o_osif_t;
	signal i_memif : i_memif_t;
	signal o_memif : o_memif_t;

	type STATE_TYPE is (STATE_THREAD_INIT,
	                    STATE_CMD,STATE_STORE, STATE_PERF);
	signal state : STATE_TYPE;

	subtype C_ANGLE_RANGE is natural range 31 downto 21;
	subtype C_LEG_RANGE   is natural range 20 downto 18;

	signal cmd    : std_logic_vector(31 downto 0);

	signal srv0_a, srv1_a, srv2_a, srv3_a, srv4_a, srv5_a : unsigned(10 downto 0);
	signal srv0_c, srv1_c, srv2_c, srv3_c, srv4_c, srv5_c : unsigned(21 downto 0);
	signal srv0_p, srv1_p, srv2_p, srv3_p, srv4_p, srv5_p : std_logic;

	constant C_SRV0_CAL : integer := 57334;
	constant C_SRV1_CAL : integer := 63222;
	constant C_SRV2_CAL : integer := 53375;
	constant C_SRV3_CAL : integer := 64465;
	constant C_SRV4_CAL : integer := 57482;
	constant C_SRV5_CAL : integer := 66722;

	signal srv_count : unsigned(21 downto 0) := (others => '0');

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

			srv0_a <= to_unsigned(900, 11);
			srv1_a <= to_unsigned(900, 11);
			srv2_a <= to_unsigned(900, 11);
			srv3_a <= to_unsigned(900, 11);
			srv4_a <= to_unsigned(900, 11);
			srv5_a <= to_unsigned(900, 11);
		elsif rising_edge(HWT_Clk) then
			case state is
				when STATE_THREAD_INIT =>
					THREAD_INIT(i_osif, o_osif, resume, done);
					if done then
						state <= STATE_CMD;
					end if;

				when STATE_CMD =>
					MBOX_GET(i_osif, o_osif, servo_cmd, cmd, done);
					if done then
						state <= STATE_STORE;
					end if;
				
				when STATE_STORE =>
					case cmd(C_LEG_RANGE) is
						when "000" => srv0_a <= unsigned(cmd(C_ANGLE_RANGE));
						when "001" => srv1_a <= unsigned(cmd(C_ANGLE_RANGE));
						when "010" => srv2_a <= unsigned(cmd(C_ANGLE_RANGE));
						when "011" => srv3_a <= unsigned(cmd(C_ANGLE_RANGE));
						when "100" => srv4_a <= unsigned(cmd(C_ANGLE_RANGE));
						when "101" => srv5_a <= unsigned(cmd(C_ANGLE_RANGE));
						when others =>
					end case;

					state <= STATE_PERF;

				when STATE_PERF =>
					MBOX_PUT(i_osif, o_osif, performance_perf, x"31000000", ignore, done);
					if done then
						state <= STATE_CMD;
					end if;

			end case;
		end if;
	end process osfsm_proc;

	srv0_c <= srv0_a * 100 + C_SRV0_CAL;
	srv1_c <= srv1_a * 100 + C_SRV1_CAL;
	srv2_c <= srv2_a * 100 + C_SRV2_CAL;
	srv3_c <= srv3_a * 100 + C_SRV3_CAL;
	srv4_c <= srv4_a * 100 + C_SRV4_CAL;
	srv5_c <= srv5_a * 100 + C_SRV5_CAL;

	srv_proc: process(HWT_Clk) is
	begin
		if rising_edge(HWT_Clk) then
			srv_count <= srv_count + 1;

			if srv_count = 1000000 then
				srv_count <= (others => '0');
	--			srv0_c <= srv0_a * 100 + C_SRV0_CAL;
	--			srv1_c <= srv1_a * 100 + C_SRV1_CAL;
	--			srv2_c <= srv2_a * 100 + C_SRV2_CAL;
	--			srv3_c <= srv3_a * 100 + C_SRV3_CAL;
	--			srv4_c <= srv4_a * 100 + C_SRV4_CAL;
	--			srv5_c <= srv5_a * 100 + C_SRV5_CAL;
			end if;
		end if;
	end process srv_proc;

	srv0_p <= '1' when srv_count < srv0_c else '0';
	srv1_p <= '1' when srv_count < srv1_c else '0';
	srv2_p <= '1' when srv_count < srv2_c else '0';
	srv3_p <= '1' when srv_count < srv3_c else '0';
	srv4_p <= '1' when srv_count < srv4_c else '0';
	srv5_p <= '1' when srv_count < srv5_c else '0';

	srv_out_proc: process(HWT_Clk) is
	begin
		if rising_edge(HWT_Clk) then
			SRV0_Out <= srv0_p;
			SRV1_Out <= srv1_p;
			SRV2_Out <= srv2_p;
			SRV3_Out <= srv3_p;
			SRV4_Out <= srv4_p;
			SRV5_Out <= srv5_p;
		end if;
	end process srv_out_proc;
	
end architecture;
