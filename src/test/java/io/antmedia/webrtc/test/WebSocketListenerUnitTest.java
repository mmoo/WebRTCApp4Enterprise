package io.antmedia.webrtc.test;

import static org.junit.Assert.*;

import java.io.UnsupportedEncodingException;
import java.util.List;
import java.util.Map;

import org.apache.mina.core.session.DummySession;
import org.apache.mina.core.session.IoSession;
import org.junit.Test;
import org.red5.net.websocket.WebSocketConnection;

import io.antmedia.webrtc.WebSocketListener;


public class WebSocketListenerUnitTest {

	class WebSocketConnectionMock extends WebSocketConnection {

		public WebSocketConnectionMock(IoSession session) {
			super(session);

		}

	}

	@Test
	public void testTakeConf() {

		try {
			WebSocketListener listener = new WebSocketListener();
			WebSocketConnectionMock mockConnection = new WebSocketConnectionMock(new DummySession());
			String streamName = "stream1";
			
			boolean result = listener.processSignallingTakeConf(streamName, mockConnection, "typeString", "sdpDescription");
			assertFalse(result);
			
			result = listener.processSignallingJoin(streamName, mockConnection);
			assertTrue(result);
			
			result = listener.processSignallingTakeConf(streamName, mockConnection, "typeString", "sdpDescription");
			assertFalse(result);
			
			WebSocketConnectionMock mockConnection2 = new WebSocketConnectionMock(new DummySession());
			result = listener.processSignallingJoin(streamName, mockConnection2);
			assertTrue(result);
			
			result = listener.processSignallingTakeConf(streamName, mockConnection, "typeString", "sdpDescription");
			assertTrue(result);
			
			
			
		}
		catch (Exception e) {
			e.printStackTrace();
			fail(e.getMessage());
		}

	}
	
	@Test
	public void testTakeCandidate() {

		try {
			WebSocketListener listener = new WebSocketListener();
			WebSocketConnectionMock mockConnection = new WebSocketConnectionMock(new DummySession());
			String streamName = "stream1";
			
			boolean result = listener.processSignallingTakeCandidate(streamName, mockConnection, 5L, "typeString", "sdpDescription");
			assertFalse(result);
			
			result = listener.processSignallingJoin(streamName, mockConnection);
			assertTrue(result);
			
			result = listener.processSignallingTakeCandidate(streamName, mockConnection, 6L, "typeString", "sdpDescription");
			assertFalse(result);
			
			WebSocketConnectionMock mockConnection2 = new WebSocketConnectionMock(new DummySession());
			result = listener.processSignallingJoin(streamName, mockConnection2);
			assertTrue(result);
			
			result = listener.processSignallingTakeCandidate(streamName, mockConnection, 7L, "typeString", "sdpDescription");
			assertTrue(result);
			
			
			
		}
		catch (Exception e) {
			e.printStackTrace();
			fail(e.getMessage());
		}

	}

	@Test
	public void testJoinAndDisconnect() {
		try {
			WebSocketListener listener = new WebSocketListener();
			WebSocketConnectionMock mockConnection = new WebSocketConnectionMock(new DummySession());
			String streamName = "stream1";

			Map<String, List<WebSocketConnection>> signallingConnections = listener.getSignallingConnections();
			assertNotNull(signallingConnections);
			assertEquals(0, signallingConnections.size());


			boolean result;

			result = listener.processSignallingJoin(streamName, mockConnection);
			assertTrue(result);
			signallingConnections = listener.getSignallingConnections();
			assertEquals(1, signallingConnections.size());


			WebSocketConnectionMock mockConnection2 = new WebSocketConnectionMock(new DummySession());
			result = listener.processSignallingJoin(streamName, mockConnection2);
			assertTrue(result);

			signallingConnections = listener.getSignallingConnections();
			assertEquals(1, signallingConnections.size());

			List<WebSocketConnection> webSocketConnectionList = signallingConnections.get(streamName);
			assertNotNull(webSocketConnectionList);
			assertEquals(2, webSocketConnectionList.size());


			String streamName2 = "stream2";
			WebSocketConnectionMock mockConnection3 = new WebSocketConnectionMock(new DummySession());
			result = listener.processSignallingJoin(streamName2, mockConnection3);
			assertTrue(result);
			signallingConnections = listener.getSignallingConnections();
			assertEquals(2, signallingConnections.size());


			listener.wsSignallingDisconnected(mockConnection);
			signallingConnections = listener.getSignallingConnections();
			assertEquals(2, signallingConnections.size());

			assertEquals(1, signallingConnections.get(streamName).size());

			listener.wsSignallingDisconnected(mockConnection2);
			assertNull(signallingConnections.get(streamName));

		} catch (UnsupportedEncodingException e) {
			e.printStackTrace();
		}
	}

	@Test
	public void testJoin() {

		WebSocketListener listener = new WebSocketListener();
		WebSocketConnectionMock mockConnection = new WebSocketConnectionMock(new DummySession());
		String streamName = "stream1";
		try {
			Map<String, List<WebSocketConnection>> signallingConnections = listener.getSignallingConnections();
			assertNotNull(signallingConnections);
			assertEquals(0, signallingConnections.size());

			boolean result = listener.processSignallingJoin(streamName, mockConnection);
			assertTrue(result);

			signallingConnections = listener.getSignallingConnections();
			assertEquals(1, signallingConnections.size());

			List<WebSocketConnection> webSocketConnectionList = signallingConnections.get(streamName);
			assertNotNull(webSocketConnectionList);
			assertEquals(1, webSocketConnectionList.size());


			WebSocketConnectionMock mockConnection2 = new WebSocketConnectionMock(new DummySession());
			result = listener.processSignallingJoin(streamName, mockConnection2);
			assertTrue(result);

			signallingConnections = listener.getSignallingConnections();
			assertEquals(1, signallingConnections.size());

			webSocketConnectionList = signallingConnections.get(streamName);
			assertNotNull(webSocketConnectionList);
			assertEquals(2, webSocketConnectionList.size());


			//room has two connections, new connection will not be added
			WebSocketConnectionMock mockConnection3 = new WebSocketConnectionMock(new DummySession());
			result = listener.processSignallingJoin(streamName, mockConnection3);
			//return false, because room is full
			assertFalse(result);

			signallingConnections = listener.getSignallingConnections();
			assertEquals(1, signallingConnections.size());

			webSocketConnectionList = signallingConnections.get(streamName);
			assertNotNull(webSocketConnectionList);
			assertEquals(2, webSocketConnectionList.size());


			//leave one connection 
			listener.processSignallingLeave(streamName, mockConnection);

			signallingConnections = listener.getSignallingConnections();
			assertEquals(1, signallingConnections.size());

			webSocketConnectionList = signallingConnections.get(streamName);
			assertNotNull(webSocketConnectionList);
			assertEquals(1, webSocketConnectionList.size());

			//leave the same connection once more but nothing should be changed
			listener.processSignallingLeave(streamName, mockConnection);

			signallingConnections = listener.getSignallingConnections();
			assertEquals(1, signallingConnections.size());

			webSocketConnectionList = signallingConnections.get(streamName);
			assertNotNull(webSocketConnectionList);
			assertEquals(1, webSocketConnectionList.size());


			//leave a connection that is not in the room
			listener.processSignallingLeave(streamName, mockConnection3);

			signallingConnections = listener.getSignallingConnections();
			assertEquals(1, signallingConnections.size());

			webSocketConnectionList = signallingConnections.get(streamName);
			assertNotNull(webSocketConnectionList);
			assertEquals(1, webSocketConnectionList.size());


			//leave the other connection in the room
			listener.processSignallingLeave(streamName, mockConnection2);

			signallingConnections = listener.getSignallingConnections();
			assertEquals(0, signallingConnections.size());

			webSocketConnectionList = signallingConnections.get(streamName);
			assertNull(webSocketConnectionList);

		} catch (UnsupportedEncodingException e) {

			e.printStackTrace();
			fail(e.getMessage());
		}

	}
}
