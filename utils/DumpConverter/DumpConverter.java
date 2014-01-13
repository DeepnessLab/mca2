import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;

// To create a readable file from tcpdump output: $ tcpdump -xr <file> > <outfile>
// <outfile> is the input for this program

/**
 * Creates a dump file for the AC-NFA-C software
 * 
 * @author yotam
 */
public class DumpConverter {
	
	public static final String FILE_PATH = "/Users/yotamhc/Documents/Study/CompactDFA/Code-Yaron/DARPA_eval_b/sample_data01.tcpdump.hex";
	//public static final String FILE_PATH = "/Users/yotamhc/Documents/Study/CompactDFA/Code-Yaron/DARPA_eval_b/onepacket.tcpdump.hex";
	/*
	private static void printPacket(Packet p) {
		System.out.println("Packet from " + p.getSource() + " to " + p.getDest() + " from time " + p.getTime() + " (size: " + p.getData().length + "):");
		System.out.println(p.getData());
	}
	*/
	private static void convertFile(String input, String output) throws IOException {
		BufferedReader reader = null;
		PacketOutputStream out = null;
		int count = 0;
		try {
			reader = new BufferedReader(new FileReader(input));
			out = new PacketOutputStream(output);
			String line;
			ArrayList<String> lines = null;
			while ((line = reader.readLine()) != null) {
				if (line.startsWith("\t")) {
					lines.add(line);
				} else if (lines == null) {
					lines = new ArrayList<String>();
					lines.add(line);
				} else {
					try {
						Packet p = new Packet(lines.toArray(new String[0]));
						//printPacket(p);
						//inspector.inspect(p);
						out.writePacket(p);
						count++;
					} catch (NotAnIpPacketException e) {
						// Skip ethernet packets
					}
					lines.clear();
					lines.add(line);
				}
			}
			if (lines != null && lines.size() > 0) {
				try {
					Packet p = new Packet(lines.toArray(new String[0]));
					//printPacket(p);
					//inspector.inspect(p);
					out.writePacket(p);
					count++;
				} catch (NotAnIpPacketException e) {
					// Skip ethernet packets
				}
				lines.clear();
			}
		} finally {
			if (reader != null) {
				reader.close();
			}
			if (out != null) {
				out.flush();
				out.close();
			}
		}
	}
	
	private static int getSize(byte[] size) {
		return ((size[0] & 0xFF) << 8) | (size[1] & 0xFF);
	}
	
	private static char[] getHex(byte value, char[] hex) {
		if (hex.length != 2)
			throw new IllegalArgumentException();
		
		int high = ((value & 0xF0) >> 4);
		int low = (value & 0x0F);
		hex[0] = (char)((high < 10) ? ('0' + high) : ('A' + (high - 10)));
		hex[1] = (char)((low < 10) ? ('0' + low) : ('A' + (low - 10)));
		
		return hex;
	}
	
	private static String patternToString(byte[] pattern) {
		StringBuffer sb = new StringBuffer();
		char[] hex = { 0, 0 };
		for (int i = 0; i < pattern.length; i++) {
			if (pattern[i] >= 32 && pattern[i] < 127) {
				sb.append((char)(pattern[i]));
			} else {
				sb.append('|').append(getHex(pattern[i], hex)).append('|');
			}
		}
		return sb.toString();
	}
	
	private static void createDummyDump(String patternsFile, String output, int numPackets, int maxPatterns, int skipFirst, boolean printPatterns, float concentrationFactor) {
		// Read patterns
		BufferedInputStream in = null;
		int skipped = 0;
		int count = 0;
		ArrayList<byte[]> patterns = new ArrayList<byte[]>();
		try {
			byte[] size = new byte[2];
			int length;
			
			in = new BufferedInputStream(new FileInputStream(patternsFile));
			
			while (in.available() > 0 && (maxPatterns <= 0 || count < maxPatterns)) {
				if (in.read(size) != 2) {
					System.err.println("ERROR: Cannot read size of pattern");
					return;
				}
				length = getSize(size);
				
				if (length <= 0 || length > 10000000) {
					System.err.println("ERROR: Invalid length of pattern: " + length);
					return;
				}
				
				byte[] data = new byte[length];
				if (in.read(data) != length) {
					System.err.println("ERROR: Cannot read pattern");
					return;
				}
				
				if (skipped++ < skipFirst) {
					continue;
				}
				
				count++;
/*
				if (length < 39)
					continue;
*/		
				patterns.add(data);
				
				if (printPatterns) {
					String pat = patternToString(data);
					System.out.println(pat + " (size: " + length + ")");
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
			return;
		} finally {
			if (in != null) {
				try {
					in.close();
				} catch (Exception e) { }
			}
		}
		
		// Create dump
		PacketOutputStream out = null;
		try {
			out = new PacketOutputStream(output);
			byte[][] pats = patterns.toArray(new byte[0][]);
			for (int i = 0; i < numPackets; i++) {
				Packet p = new DummyPacket(pats, maxPatterns, concentrationFactor);
				out.writePacket(p);
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (out != null) {
				try {
					out.flush();
					out.close();
				} catch (Exception e) { }
			}
		}
		DummyPacket.printCounters();
	}
	
	private static void printUsage() {
		System.out.println("Usage: DumpConverter [-d] [-p] input output [numPackets] [maxPatterns] [skipFirst] [concentrationFactor]");
		System.out.println("\tinput: tcpdump hex file (or patterns file in dummy mode)");
		System.out.println("\toutput: binary dump file for pattern matching application");
		System.out.println("\t-d: dummy mode, use numPackets to define number of packets to generate. input is patterns file in this mode.");
		System.out.println("\t\t-p: valid in dummy mode only. Prints patterns to standard output.");
		System.out.println("\t\tnumPackets: number of packets to generate");
		System.out.println("\t\tmaxPatterns: max number of patterns to use");
		System.out.println("\t\tskipFirst: number of first patterns in file to be skipped");
		System.out.println("\t\tconcentrationFactor: how concentrated the patterns are in the dump, from 0 to 1");
		System.out.println("To create a readable file from tcpdump output use redirection as follows: $ tcpdump -xr <file> > <hexfile>");
	}
	
	public static void main(String[] args) {
		try {
			if (args.length < 2 || (args[0].equals("-d") && args.length < 4)) {
				printUsage();
				return;
			}
			
			String input, output;
			int numPackets, maxPatterns = 0, skip = 0;
			int i = 0;
			boolean dummy = false;
			boolean print = false;
			float concentrationFactor = 1;
			
			if (args[0].equals("-d")) {
				i = 1;
				dummy = true;
				if (args[1].equals("-p")) {
					i = 2;
					print = true;
				}
			}
			
			input = args[i];
			output = args[i + 1];

			if (!dummy) {
				convertFile(input, output);
			} else {
				try {
					numPackets = Integer.parseInt(args[i + 2]);
					if (args.length > i + 3) {
						maxPatterns = Integer.parseInt(args[i + 3]);
						if (args.length > i + 4) {
							skip = Integer.parseInt(args[i + 4]);
							if (args.length > i + 5) {
								concentrationFactor = Float.parseFloat(args[i + 5]);
							}
						}
					}
				} catch (Exception e) {
					printUsage();
					return;
				}
				
				createDummyDump(input, output, numPackets, maxPatterns, skip, print, concentrationFactor);
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
