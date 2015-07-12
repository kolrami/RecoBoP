library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity vga_controller is
	generic (
		G_VC_CLK_PRD  : time := 10ns;
		G_VGA_CLK_PRD : time := 20ns;

		G_H_DISPLAY : integer   := 800;
		G_H_FRONT   : integer   := 56;
		G_H_SYNC    : integer   := 120;
		G_H_BACK    : integer   := 64;
		G_H_POL     : std_logic := '1';
		G_V_DISPLAY : integer   := 600;
		G_V_FRONT   : integer   := 37;
		G_V_SYNC    : integer   := 6;
		G_V_BACK    : integer   := 23;
		G_V_POL     : std_logic := '1'
	);
	port (
		VGA_HSync  : out std_logic;
		VGA_VSync  : out std_logic;
		VGA_HCount : out unsigned(31 downto 0);
		VGA_VCount : out unsigned(31 downto 0);

		VC_Clk : in  std_logic
	);
end entity vga_controller;

architecture implementation of vga_controller is
	constant C_CLK_DIV_COUNT : integer := G_VGA_CLK_PRD / G_VC_CLK_PRD / 2 - 1;
	signal vclk       : std_logic := '0';
	signal vclk_count : unsigned(31 downto 0) := (others => '0');

	constant C_H_COUNT : integer := G_H_DISPLAY + G_H_FRONT + G_H_SYNC + G_H_BACK - 1;
	constant C_V_COUNT : integer := G_V_DISPLAY + G_V_FRONT + G_V_SYNC + G_V_BACK - 1;

	signal h_count, v_count : unsigned(31 downto 0) := (others => '0');
begin

	ctrl_proc : process (VC_Clk) is
	begin
		if rising_edge(VC_Clk) then
			vclk_count <= vclk_count + 1;

			if vclk_count = C_CLK_DIV_COUNT then
				vclk <= not vclk;
				vclk_count <= (others => '0');

				if vclk = '0' then
					h_count <= h_count + 1;

					if h_count = C_H_COUNT then
						h_count <= (others => '0');
						v_count <= v_count + 1;

						if v_count = C_V_COUNT then
							v_count <= (others => '0');
						end if;
					end if;
				end if;
			end if;
		end if;
	end process ctrl_proc;

	VGA_HCount <= v_count;
	VGA_VCount <= h_count;

	VGA_HSync <= not G_H_POL when h_count < G_H_DISPLAY + G_H_FRONT else
	                 G_H_POL when h_count < G_H_DISPLAY + G_H_FRONT + G_H_SYNC else
	             not G_H_POL when h_count < G_H_DISPLAY + G_H_FRONT + G_H_SYNC + G_H_BACK else
	             not G_H_POL;

	VGA_VSync <= not G_H_POL when v_count < G_V_DISPLAY + G_V_FRONT else
	                 G_H_POL when v_count < G_V_DISPLAY + G_V_FRONT + G_V_SYNC else
	             not G_H_POL when v_count < G_V_DISPLAY + G_V_FRONT + G_V_SYNC + G_V_BACK else
	             not G_H_POL;

end architecture implementation;