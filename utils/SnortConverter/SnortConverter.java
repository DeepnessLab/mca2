import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.FileOutputStream;
import java.io.FileReader;

/**
 * Converts Snort pattern file from ascii to binary
 * Output is a binary file in which each pattern is a record of the format [size(2B), pattern]
 * Records are concatenated to each other. 
 */
public class SnortConverter {
	
	/**
	 * Converts a single pattern
	 * 
	 * @param s Pattern
	 * @param buff Output buffer
	 * @return Length of converted pattern, in bytes
	 */
	private static int toBinary(String s, byte[] buff) {
		int j;
		int idx = 0;
		for (int i = 0; i < s.length(); i++) {
			if (s.charAt(i) == '|' && ((j = s.indexOf('|', i + 1)) != -1)) {
				String hex = s.substring(i + 1, j);
				String[] bytes = hex.split("\\ ");
				for (int k = 0; k < bytes.length; k++) {
					buff[idx] = (byte)Integer.parseInt(bytes[k], 16);
					idx++;
				}
				i = j;
			} else {
				buff[idx] = (byte)s.charAt(i);
				idx++;
			}
		}
		return idx;
	}
	
	/**
	 * Main method.
	 * @param args Command line arguments
	 */
	public static void main(String[] args) {
		if (args.length < 2 || args.length > 3) {
			System.out.println("Usage: java SnortConverter asciiFile binFile [min pattern length]");
			return;
		}
		
		BufferedReader reader = null;
		BufferedOutputStream out = null;
		byte[] buff = new byte[4096];
		byte[] size = new byte[2];
		int minLen = 1;
		if (args.length == 3) {
			try {
				minLen = Integer.parseInt(args[2]);
				if (minLen < 1)
					throw new NumberFormatException();
			} catch (NumberFormatException e) {
				System.out.println("Error: min pattern length should be a positive integer");
				return;
			}
		}
		
		try {
			reader = new BufferedReader(new FileReader(args[0]));
			out = new BufferedOutputStream(new FileOutputStream(args[1]));
			String line;
			int len;
			// Read patterns by lines
			while ((line = reader.readLine()) != null) {
				// Convert pattern to binary
				len = toBinary(line, buff);
				if (len < minLen)
					continue;
				
				// Write size to output file
				size[0] = (byte)(len >> 8);
				size[1] = (byte)len;
				out.write(size);
				// Write binary data to output file
				out.write(buff, 0, len);
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (reader != null) {
				try {
					reader.close();
				} catch (Exception e) { }
			}
			if (out != null) {
				try {
					out.close();
				} catch (Exception e) { }
			}
		}
	}
}
