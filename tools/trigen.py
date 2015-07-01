import sys
import argparse
import math

parser = argparse.ArgumentParser("build_sw", description="""
	Generates a lookup table for sine and cosine functions.
	""")
parser.add_argument("-f", "--format", help="Format of export", choices=["c", "vhdl"], default="c")
parser.add_argument("-a", "--address", help="Number of bits for address", type=int, default=10)
parser.add_argument("-m", "--maxangle", help="Maximum angle in degrees to include", type=float, default=180)
parser.add_argument("-t", "--type", help="Type of lookup table to generate", default="double")

args = parser.parse_args()
addr = (1 << args.address)
step = args.maxangle / (addr - 1)

angles = [step * _ for _ in range(0, addr)]
sin = [math.sin(math.radians(_)) for _ in angles]
cos = [math.cos(math.radians(_)) for _ in angles]
sin_str = ["{:.8f}".format(_) for _ in sin]
cos_str = ["{:.8f}".format(_) for _ in cos]

out = ""
if args.format == "c":
	out += "#define TRIG_COUNT " + str(addr) + "\n"
	out += "#define TRIG_ADDR " + str(args.address) + "\n"
	out += "#define TRIG_MIN_ANGLE_DEG 0\n"
	out += "#define TRIG_MAX_ANGLE_DEG " + str(args.maxangle) + "\n"
	#out += "#define TRIG_ADDR_PER_ANGLE_DEG " + "{:.8f}".format((addr - 1) / args.maxangle) + "\n"
	#out += "#define TRIG_SIN_DEG(x) (sin[(unsigned int)(TRIG_ADDR_PER_ANGLE_DEG * (x))])\n"
	#out += "#define TRIG_COS_DEG(x) (cos[(unsigned int)(TRIG_ADDR_PER_ANGLE_DEG * (x))])\n"
	out += args.type + " sin_lut[" + str(addr) + "] = {" + ",".join(sin_str) + "};\n"
	out += args.type + " cos_lut[" + str(addr) + "] = {" + ",".join(cos_str) + "};\n"
elif args.format == "vhdl":
	out += "type TRIG_T is array (0 to " + str(addr - 1) + ") of " + args.type;
	out += "signal sin : TRIG_T := (" + ",".join(sin_str) + ")\n"
	out += "signal sin : TRIG_T := (" + ",".join(cos_str) + ")\n"
else:
	print("ERROR: Unsupported format")

print(out)