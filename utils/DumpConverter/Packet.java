

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.util.ArrayList;

public class Packet {
	private String time;
	private String source;
	private String dest;
	private String flags;
	private char[] data;
	
	protected Packet(String time, String source, String dest, String flags) {
		this.time = time;
		this.source = source;
		this.dest = dest;
		this.flags = flags;
	}
	
	protected void setData(char[] data) {
		this.data = data;
	}

	public Packet(String[] rows) throws NotAnIpPacketException {
		parseHeader(rows[0]);
		parseBody(rows, 1, rows.length - 1);
	}
	
	private void parseHeader(String row) throws NotAnIpPacketException {
		String[] parts = row.split(" ");
		if (parts.length < 5 || !parts[1].equals("IP"))
			throw new NotAnIpPacketException();
		
		time = parts[0];
		source = parts[2];
		if (!parts[3].equals(">"))
			throw new NotAnIpPacketException();
		dest = parts[4];
		
		if (parts.length > 5) {
			StringBuffer sb = new StringBuffer();
			for (int i = 5; i < parts.length; i++) {
				sb.append(parts[i]);
				if (i < parts.length - 1)
					sb.append(" ");
			}
			flags = sb.toString();
		}
	}
	
	private void parseBody(String[] rows, int start, int offset) {
		ArrayList<Character> lst = new ArrayList<Character>();
		for (int i = 0; i < offset; i++) {
			String data = rows[i + start].substring(rows[i + start].indexOf(':') + 1).replaceAll(" ", "");
			for (int j = 0; j < data.length(); j += 2) {
				String hex = data.substring(j, j + 2);
				lst.add((char)(Integer.parseInt(hex, 16)));
			}
		}
		this.data = new char[lst.size()];
		for (int i = 0; i < lst.size(); i++)
			this.data[i] = lst.get(i);
	}

	public String getTime() {
		return time;
	}

	public String getSource() {
		return source;
	}

	public String getDest() {
		return dest;
	}

	public String getFlags() {
		return flags;
	}

	public char[] getData() {
		return data;
	}
	
	public DataInputStream getInputStream() throws IOException {
		byte[] bytes = new String(data).getBytes();
		ByteArrayInputStream in = new ByteArrayInputStream(bytes);
		return new DataInputStream(in);
	}
	
	public String toString() {
		return time + " " + source + " > " + dest + " "  + flags;
	}
	
	public String getDataString() {
		return new String(data);
	}
}
