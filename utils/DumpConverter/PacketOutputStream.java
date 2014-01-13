import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;


public class PacketOutputStream extends BufferedOutputStream {

	private static final byte[] HEADER = new byte[16];
	private static final SimpleDateFormat DATE_FORMAT = new SimpleDateFormat("HH:mm:ss.SSS");
	private static long BASE_MILLIS;
	
	static {
		try {
			BASE_MILLIS = DATE_FORMAT.parse("00:00:00.000").getTime();
		} catch (Exception e) { }
	}
	
	public PacketOutputStream(String path) throws IOException {
		super(new FileOutputStream(path));
	}
	
	private void setIntHeaderValue(long value, byte[] header, int idx) {
		value = value & 0xFFFFFFFF;
		for (int i = idx + 3; i >= idx; i--) {
			header[i] = (byte)(value & 0xFF);
			value >>= 8;
		}
	}
	
	private void setTime(Packet p, byte[] header) {
		String time = p.getTime();
		time = time.substring(0, time.indexOf('.') + 4);
		try {
			Date date = DATE_FORMAT.parse(time);
			long value = (date.getTime() - BASE_MILLIS);
			setIntHeaderValue(value, header, 12);
		} catch (Exception e) {
			throw new RuntimeException("Date parsing error");
		}
	}
	
	private void setSize(Packet p, byte[] header) {
		setIntHeaderValue(p.getData().length, header, 0);
	}
	
	private void setIP(String ip, byte[] header, int idx) throws NotAnIpPacketException {
		for (int i = 0; i < 4; i++) {
			header[idx + i] = (byte)0;
		}
		/*
		String[] parts = ip.split("\\.");
		if (parts.length < 4) {
			throw new NotAnIpPacketException();
		}
		try {
			for (int i = 0; i < 4; i++) {
				parts[i] = parts[i].replaceAll("\\:", "");
				header[idx + i] = (byte)(Short.parseShort(parts[i]) & 0xFF);
			}
		} catch(Exception e) {
			for (int i = 0; i < 4; i++) {
				header[idx + i] = (byte)0;
			}
		}
		*/
	}
	
	private void setSource(Packet p, byte[] header) throws NotAnIpPacketException {
		setIP(p.getSource(), header, 4);
	}
	
	private void setDest(Packet p, byte[] header) throws NotAnIpPacketException {
		setIP(p.getDest(), header, 8);
	}
	
	public boolean writePacket(Packet p) throws IOException, NotAnIpPacketException {
		char[] data = p.getData();
		byte[] buff = new byte[data.length];
		
		for (int i = 0; i < buff.length; i++) {
			buff[i] = (byte)(data[i] & 0x00FF);
		}
		
		writeHeader(p);
		this.write(buff);
		return true;
	}
	
	private void writeHeader(Packet p) throws IOException, NotAnIpPacketException {
		Arrays.fill(HEADER, (byte)0);
		setSize(p, HEADER);
		setSource(p, HEADER);
		setDest(p, HEADER);
		setTime(p, HEADER);
		this.write(HEADER);
	}

}
