import sys
from time import strftime, localtime
from serial import Serial
from struct import unpack
from binascii import crc32

TS0 = 946674000 # date -d '2000-01-01 00:00' +%s

if __name__ == "__main__":
	if len(sys.argv) != 3:
		print("USAGE: {} COM_PORT file.txt".format(sys.argv[0]))
		exit(1)
	
	with Serial(sys.argv[1], baudrate=9600) as sr:
		sr.reset_input_buffer()

		print("START DOWNLOAD")

		m = sr.read(12)
		ts, per, ds, np = unpack("<IIHH", m)

		print(strftime("DATE: %Y-%m-%d %H:%M:%S", localtime(TS0 + ts))) 
		print("PERIOD: {} min".format(per / 60))
		print("DATA: {} x {}B".format(np, ds))
		
		if ds == 1:
			df = "<B"
		elif ds == 2:
			df = "<H"
		elif ds == 4:
			df = "<I"
		else:
			raise NameError, "Invalid data size {}".format(ds)
		dd = []
		for i in range(np):
			m += sr.read(ds)
			dd.append(unpack(df, m[-ds:])[0])
			sys.stderr.write("\r{}/{}".format(i + 1, np))
		sys.stderr.write(" DONE\n")

		if crc32(m) & 0xffffffff != unpack("<I", sr.read(4))[0]:
			raise NameError, "CHECKSUM MISMATCH!"
		print("CHECKSUM OK")

	with open(sys.argv[2], "w") as f:
		f.write(strftime("# DATE: %Y-%m-%d %H:%M:%S\n", localtime(TS0 + ts)))
		f.write("# DATA: {} x {} min\n".format(np, per / 60))
		for d in dd:
			f.write("{}\n".format(d))
