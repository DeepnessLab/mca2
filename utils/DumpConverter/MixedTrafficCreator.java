import java.io.IOException;


public class MixedTrafficCreator extends DummyDumpCreator {

	public MixedTrafficCreator(String patternsFile,
							   double heavyPacketRate,
							   double heavyPacketMatchPercentage) throws IOException {
		super(patternsFile);
	}

	@Override
	public void GenerateDump(String path) {
		// TODO Auto-generated method stub

	}

}
