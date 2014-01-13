import java.io.ByteArrayOutputStream;
import java.util.PriorityQueue;


public class DummyPacket extends Packet {
	private int maxPatterns;
	private float concentrationFactor;
	
	private static char _nonexists = 0;
	private static boolean _nonexists_set = false;
	
	private static int _count_in = 0;
	private static int _count_out = 0;
	
	public static void printCounters() {
		System.out.println("Inside patterns: " + _count_in);
		System.out.println("Outside patterns: " + _count_out);
		System.out.println("Anat's Ratio: " + ((double)_count_in) / (_count_in + _count_out));
	}
	
	public DummyPacket(byte[][] patterns, int maxPatterns, float concentrationFactor) {
		super("23:55:59.279341", "0.0.0.0", "0.0.0.0", "");
		this.maxPatterns = maxPatterns;
		this.concentrationFactor = concentrationFactor;
		setData(patterns);
	}
	
	private void setData(byte[][] patterns) {
		if (!_nonexists_set) {
			boolean[] nonexisting = new boolean[256];
			
			for (byte[] s: patterns) {
				for (int i = 0; i < s.length; i++) {
					nonexisting[(int)(s[i] & 0x00FF)] = true;
				}
			}
			for (int i = 0; i < 256; i++) {
				if (!nonexisting[i]) {
					_nonexists = (char)i;
					_nonexists_set = true;
					break;
				}
			}
		}
		
		PriorityQueue<Pattern> q = new PriorityQueue<Pattern>();
		
		for (byte[] s: patterns) {
			q.add(new Pattern(s));
		}
		
		try {
			int i = 0;
			ByteArrayOutputStream sb = new ByteArrayOutputStream();
			while (!q.isEmpty() && sb.size() < 100000 && (maxPatterns == 0 || i < maxPatterns)) {
				byte[] pat = q.poll().getPattern();
				//if (pat.length > 1000)
				//	continue;
				sb.write(pat);
				sb.write((byte)(_nonexists & 0xFF));
				
				_count_in += pat.length;
				//_count_out += 1;
				
				float expansionFactor = (1 - concentrationFactor);
				
				int extra = (int)((Math.pow(1 + expansionFactor, 1 + 7 * expansionFactor) - 1) * 3);
				//int extra = (int)(765 * expansionFactor);
				
				for (int j = 0; j < extra; j++) {
					sb.write((byte)(Math.random() * Byte.MAX_VALUE));
				}
				
				_count_out += extra;
				
				i++;
			}
			//super.setData(sb.toString("US-ASCII").toCharArray());
			super.setData(toCharArray(sb.toByteArray()));
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	private char[] toCharArray(byte[] data) {
		char[] res = new char[data.length];
		for (int i = 0; i < data.length; i++) {
			res[i] = (char)(data[i] & 0xFF);
		}
		return res;
	}
	
	private static class Pattern implements Comparable<Pattern> {

		private int i;
		private byte[] s;
		
		public Pattern(byte[] pattern) {
			i = (int)(Math.random() * 1000000) + pattern.length;
			s = pattern;
		}
		
		@Override
		public int compareTo(Pattern arg0) {
			return  arg0.i - this.i;
		}
		
		public byte[] getPattern() {
			return s;
		}		
	}
}
