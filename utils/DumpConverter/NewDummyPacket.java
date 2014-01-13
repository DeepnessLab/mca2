
public class NewDummyPacket extends Packet {

	protected NewDummyPacket(char[] payload) {
		super("23:55:59.279341", "0.0.0.0", "0.0.0.0", "");
		super.setData(payload);
	}

}
