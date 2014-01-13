import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;

/**
 * Creates dummy dumps.
 * New version for SIGCOMM 2012 work.
 * 
 */
public abstract class DummyDumpCreator {
	
	private Pattern[] patterns;
	private char nonexistent;
	
	/**
	 * Constructor.
	 * 
	 * @param patternsFile File of patterns (binary format)
	 * @throws IOException When an IO error occurs
	 */
	public DummyDumpCreator(String patternsFile) throws IOException {
		readPatterns(patternsFile);
	}
	
	/**
	 * Generates a dump file and writes it to a file.
	 * 
	 * @param path Output file path
	 */
	public abstract void GenerateDump(String path);
	
	/**
	 * Writes a series of packets to a file
	 * 
	 * @param packets Packets to write
	 * @param path Output file path
	 * @throws IOException When an IO error occurs
	 */
	protected void writeDumpFile(Packet[] packets, String path) throws IOException {
		PacketOutputStream out = null;
		
		try {
			out = new PacketOutputStream(path);
			for (Packet p : packets) {
				try {
					out.writePacket(p);
				} catch (NotAnIpPacketException e) { }
			}
		} catch (IOException e) {
			throw e;
		} finally {
			if (out != null) {
				try {
					out.close();
				} catch (Exception e) { }
			}
		}
	}
	
	/**
	 * Creates a dummy packet with the requested match percentage
	 * 
	 * @param size Size of payload
	 * @param matchPercentage Match percentage
	 * @return A dummy packet
	 */
	protected Packet createPacket(int size, double matchPercentage) {
		int total;
		int heavy;
		double rate;
		int toCopy, i;
		char[] data = new char[size];
		
		total = heavy = 0;
		
		while (total < size) {
			rate = (total == 0) ? 0 : (double)heavy / total;
			
			if (rate < matchPercentage && Math.random() < matchPercentage) {
				// Put a pattern here
				Pattern pat = getRandomPattern();
				toCopy = Math.min(size - total, pat.getLength() + 1);
				System.arraycopy(pat.getData(), 0, data, total, toCopy);
				total += toCopy;
				heavy += toCopy;
				// End with a non-existent char to make automaton reach the root
				data[total++] = nonexistent;
			} else {
				// Put some random data
				toCopy = Math.min(size - total, (int)((matchPercentage - rate) * total));
				for (i = 0; i < toCopy; i++) {
					data[total + i] = (char)((int)(Math.random() * 255) & 0xFF);
				}
				total += toCopy;
			}
		}
		
		return new NewDummyPacket(data);
	}
	
	/**
	 * Returns a random pattern from the pattern-set
	 * @return a pattern
	 */
	private Pattern getRandomPattern() {
		return this.patterns[(int)(Math.random() * this.patterns.length)];
	}
	
	/**
	 * Reads pattern-set from a file and sets patterns for the session and a non-existent char.
	 * @param path Pattern-set file (binary format)
	 * @throws IOException When an IO error occurs
	 */
	private void readPatterns(String path) throws IOException {
		byte[] sizeBuff = new byte[2];
		byte[] data;
		int size, len;
		BufferedInputStream in = null;
		
		try {
			// Open file
			in = new BufferedInputStream(new FileInputStream(path));
			
			ArrayList<Pattern> lst = new ArrayList<Pattern>();
			
			while (true) {
				// Read size
				len = in.read(sizeBuff);
				if (len == -1) {
					break;
				}
				if (len != 2) {
					System.err.println("ERROR: Cannot read size of pattern");
					throw new FatalException();
				}
				
				size = getSize(sizeBuff);
				
				if (size < 0 || size > 10000000) {
					System.err.println("ERROR: Invalid size of pattern: " + size);
					throw new FatalException();
				}
				
				// Read pattern
				data = new byte[size];
				len = in.read(data);
				if (len != size) {
					System.err.println("ERROR: Cannot read pattern from file");
					throw new FatalException();
				}
				
				// Add pattern to result list
				Pattern pat = new Pattern(data);
				lst.add(pat);
			}
			
			// Set patterns
			this.patterns = lst.toArray(new Pattern[0]);
			
			// Search for a non-existent char
			boolean[] nonexisting = new boolean[256];

			char[] patdata;
			
			for (Pattern pat: patterns) {
				patdata = pat.getData();
				for (int i = 0; i < pat.getLength(); i++) {
					nonexisting[(int)(patdata[i] & 0x00FF)] = true;
				}
			}
			for (int i = 0; i < 256; i++) {
				if (!nonexisting[i]) {
					nonexistent = (char)i;
					break;
				}
			}
		} catch (IOException e) {
			throw e;
		} catch (FatalException e) {
			System.exit(1);
		} finally {
			if (in != null) {
				try {
					in.close();
				} catch (Exception e) { }
			}
		}
	}
	
	/**
	 * Translates size from 2-bytes (as appears in patterns file) to an integer
	 * 
	 * @param size 2-bytes buffer
	 * @return size as integer
	 */
	private static int getSize(byte[] size) {
		return ((size[0] & 0xFF) << 8) | (size[1] & 0xFF);
	}
	
	/**
	 * A class that represents a pattern
	 */
	private static class Pattern {
		
		private char[] data;
		private int length;
		
		public Pattern(byte[] data) {
			this.data = new char[data.length];
			for (int i = 0; i < data.length; i++) {
				this.data[i] = (char)(data[i] & 0xFF);
			}
			this.length = data.length;
		}
		
		public char[] getData() {
			return this.data;
		}
		
		public int getLength() {
			return this.length;
		}
	}
	
	/**
	 * A custom exception to make the tool exit in fatal errors
	 */
	private static class FatalException extends Exception {
		private static final long serialVersionUID = 1L;
	}
}
