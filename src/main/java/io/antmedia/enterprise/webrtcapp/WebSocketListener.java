package io.antmedia.enterprise.webrtcapp;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.bytedeco.javacpp.avcodec;
import org.bytedeco.javacpp.avutil;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;
import org.red5.logging.Red5LoggerFactory;
import org.red5.net.websocket.WebSocketConnection;
import org.red5.net.websocket.listener.WebSocketDataListener;
import org.red5.net.websocket.model.MessageType;
import org.red5.net.websocket.model.WSMessage;
import org.slf4j.Logger;
import org.springframework.beans.BeansException;
import org.springframework.context.ApplicationContext;
import org.springframework.context.ApplicationContextAware;
import org.webrtc.IceCandidate;
import org.webrtc.MediaConstraints;
import org.webrtc.PeerConnection;
import org.webrtc.PeerConnection.IceServer;
import org.webrtc.PeerConnectionFactory;
import org.webrtc.SessionDescription;
import org.webrtc.SessionDescription.Type;

import io.antmedia.AntMediaApplicationAdapter;
import io.antmedia.AppSettings;
import io.antmedia.enterprise.adaptive.WebRTCEncoderAdaptor;
import io.antmedia.storage.StorageClient;
import io.antmedia.webrtc.WebRTCClient;
import io.antmedia.webrtc.api.IWebRTCAdaptor;


public class WebSocketListener extends WebSocketDataListener implements ApplicationContextAware{

	private static final String ATTR_STREAM_NAME = "ATTR_STREAM_NAME";



	public static final String DTLS_SRTP_KEY_AGREEMENT_CONSTRAINT = "DtlsSrtpKeyAgreement";



	private static final Logger log = Red5LoggerFactory.getLogger(WebSocketListener.class);

	private JSONParser parser = new JSONParser();
	
	private Map<Long, WebRTCEncoderAdaptor> publisherAdaptorList = new HashMap<>();

	private Map<Long, WebRTCClient> webRTCClientsMap = new HashMap();

	private Set<WebSocketConnection> connections = new HashSet<WebSocketConnection>();

	private IWebRTCAdaptor webRTCAdaptor;

	private ApplicationContext applicationContext;

	private Map<String, List<WebSocketConnection>> signallingConnections = new HashMap<String, List<WebSocketConnection>>();



	private AppSettings appSettings;



	private AntMediaApplicationAdapter appAdaptor;




	public static PeerConnectionFactory createPeerConnectionFactory(){
		PeerConnectionFactory.Options options = new PeerConnectionFactory.Options();
		options.networkIgnoreMask = 0;
		return new PeerConnectionFactory(options);
	}

	public boolean processSignallingJoin(String streamName, WebSocketConnection connection) throws UnsupportedEncodingException {

		boolean result = false;
		List<WebSocketConnection> webSocketConnections = signallingConnections.get(streamName);
		if (webSocketConnections == null) {
			webSocketConnections = new ArrayList<WebSocketConnection>();
		}

		if (webSocketConnections.size() < 2) {

			if (webSocketConnections.size() == 1) {
				JSONObject jsonResponse = new JSONObject();
				jsonResponse.put("command", "start");
				webSocketConnections.get(0).send(jsonResponse.toJSONString());
			}
			connection.getSession().setAttribute(ATTR_STREAM_NAME, streamName);
			webSocketConnections.add(connection);
			signallingConnections.put(streamName, webSocketConnections);
			JSONObject jsonResponse = new JSONObject();
			jsonResponse.put("command", "notification");
			jsonResponse.put("definition", "joined");
			connection.send(jsonResponse.toJSONString());
			result = true;
		}
		else {
			JSONObject jsonResponse = new JSONObject();
			jsonResponse.put("command", "error");
			jsonResponse.put("definition", "NoSpaceForNewPeer");
			connection.send(jsonResponse.toJSONString());
		}
		return result;
	}

	public void takeAction(JSONObject jsonObject, WebSocketConnection connection) {
		try {
			String cmd = (String) jsonObject.get("command");
			if (cmd.equals("join")) 
			{
				String streamName = (String) jsonObject.get("streamName");
				if (streamName == null || streamName.equals("")) {
					//do nothing
					JSONObject jsonResponse = new JSONObject();
					jsonResponse.put("command", "error");
					jsonResponse.put("definition", "No stream name specified");
					connection.send(jsonResponse.toJSONString());
					return;
				}
				processSignallingJoin(streamName, connection);
			}
			else if (cmd.equals("publish")) {
				String streamName = (String) jsonObject.get("streamName");
				if (streamName == null || streamName.equals("")) {
					//do nothing
					JSONObject jsonResponse = new JSONObject();
					jsonResponse.put("command", "error");
					jsonResponse.put("definition", "No stream name specified");
					connection.send(jsonResponse.toJSONString());
					return;
				}
				if (appSettings == null || appSettings.getAdaptiveResolutionList() == null ) {
					JSONObject jsonResponse = new JSONObject();
					jsonResponse.put("command", "error");
					jsonResponse.put("definition", "No encoder settings available");
					connection.send(jsonResponse.toJSONString());
					return;
				}

				WebRTCEncoderAdaptor encoderAdaptor = getNewWebRTCEncoderAdaptor();

				publisherAdaptorList.put(connection.getId(), encoderAdaptor);
				
				encoderAdaptor.setWsConnection(connection);
				
				encoderAdaptor.init(appAdaptor.getScope(), streamName, false);
				
				encoderAdaptor.start();

			}
			else if (cmd.equals("play")) {
				String streamName = (String) jsonObject.get("streamName");
				if (streamName == null || streamName.equals("")) {
					//do nothing
					JSONObject jsonResponse = new JSONObject();
					jsonResponse.put("command", "error");
					jsonResponse.put("definition", "No stream name specified");
					connection.send(jsonResponse.toJSONString());
					return;
				}
				//get WebRTCAdaptor bean

				//ask WebRTCAdaptor that whether there is a live stream with the specified name
				boolean streamExists = this.webRTCAdaptor.streamExists(streamName);
				if (streamExists) {
					// if there is a live stream, create WebRTCClient
					System.out.println("Stream "+ streamName +" exists -> " + streamExists);

					WebRTCClient webRTCClient = new WebRTCClient(connection, streamName);
					webRTCClient.setWebRTCAdaptor(webRTCAdaptor);
					webRTCClientsMap.put(connection.getId(), webRTCClient);
					webRTCClient.start();
				}
				else {
					JSONObject jsonResponse = new JSONObject();
					jsonResponse.put("command", "error");
					jsonResponse.put("code", "404");
					jsonResponse.put("definition", "No stream available with that name");
					connection.send(jsonResponse.toJSONString());
					return;
				}

			}
			else if (cmd.equals("takeConfiguration")) {

				String typeString = (String)jsonObject.get("type");
				String sdpDescription = (String)jsonObject.get("sdp");

				SessionDescription.Type type;
				if (typeString.equals("offer")) {
					type = Type.OFFER;
				}
				else {
					type = Type.ANSWER;
				}
				
				WebRTCEncoderAdaptor webRTCEncoderAdaptor = publisherAdaptorList.get(connection.getId());
				if (webRTCEncoderAdaptor != null) {
					// webrtc publish
					SessionDescription sdp = new SessionDescription(type, sdpDescription);
					webRTCEncoderAdaptor.setRemoteDescription(sdp);
				}
				else {
					WebRTCClient webRTCClient = webRTCClientsMap.get(connection.getId());
					if (webRTCClient != null) 
					{
						//webrtc play
						SessionDescription sdp = new SessionDescription(type, sdpDescription);
						webRTCClient.setRemoteDescription(sdp);
					}
					else 
					{
						//webrtc p2p
						String streamName = (String) jsonObject.get("streamName");
						if (streamName != null && !streamName.equals("")) 
						{
							processSignallingTakeConf(streamName, connection, typeString, sdpDescription);
						}
					}
				}
			}
			else if (cmd.equals("takeCandidate")) {

				String sdpMid = (String) jsonObject.get("id");
				String sdp = (String) jsonObject.get("candidate");
				long sdpMLineIndex = (long)jsonObject.get("label");

				WebRTCEncoderAdaptor webRTCEncoderAdaptor = publisherAdaptorList.get(connection.getId());
				if (webRTCEncoderAdaptor != null) {
					// webrtc publish
					IceCandidate iceCandidate = new IceCandidate(sdpMid, (int)sdpMLineIndex, sdp);
					webRTCEncoderAdaptor.addIceCandidate(iceCandidate);
				}
				else {
					WebRTCClient webRTCClient = webRTCClientsMap.get(connection.getId());
					if (webRTCClient != null) {
						// WebRTC play
						IceCandidate iceCandidate = new IceCandidate(sdpMid, (int)sdpMLineIndex, sdp);
						webRTCClient.addIceCandidate(iceCandidate);
					}
					else {
						String streamName = (String) jsonObject.get("streamName");
						if (streamName != null && !streamName.equals(""))  {
							//WebRTC p2p
							processSignallingTakeCandidate(streamName, connection, sdpMLineIndex,  sdpMid, sdp);
						}
					}
				}
			}
			else if (cmd.equals("stop")) {

				WebRTCClient webRTCClient = webRTCClientsMap.get(connection.getId());
				if (webRTCClient != null) {
				//	webRTCAdaptor.deregisterWebRTCClient(webRTCClient.getStreamId(), webRTCClient);
					webRTCClient.stop();
				}
				else {
					
					WebRTCEncoderAdaptor webRTCEncoderAdaptor = publisherAdaptorList.get(connection.getId());
					if (webRTCEncoderAdaptor != null) {
						// webrtc publish
						webRTCEncoderAdaptor.stop();
					}
				}
			}
			else if (cmd.equals("leave")) {
				//this command is only for signalling peers
				String streamName = (String) jsonObject.get("streamName");
				if (streamName != null && !streamName.equals("")) 
				{
					processSignallingLeave(streamName, connection);

				}
				else {
					JSONObject jsonResponse = new JSONObject();
					jsonResponse.put("command", "error");
					jsonResponse.put("definition", "NoStreamSpecified");
					connection.send(jsonResponse.toJSONString());
				}

			}
		}  catch (UnsupportedEncodingException e) {
			e.printStackTrace();
		}
	}


	private WebRTCEncoderAdaptor getNewWebRTCEncoderAdaptor() {
		WebRTCEncoderAdaptor encoderAdaptor = new WebRTCEncoderAdaptor(null, appSettings.getAdaptiveResolutionList());
		StorageClient storageClient = null; 
		if (applicationContext.containsBean("app.storageClient")) {
			storageClient = (StorageClient) applicationContext.getBean("app.storageClient");
		}

		encoderAdaptor.setStorageClient(storageClient);

		encoderAdaptor.setMp4MuxingEnabled(appSettings.isMp4MuxingEnabled(), appSettings.isAddDateTimeToMp4FileName());
		encoderAdaptor.setHLSMuxingEnabled(appSettings.isHlsMuxingEnabled());
		encoderAdaptor.setWebRTCEnabled(appSettings.isWebRTCEnabled());
		encoderAdaptor.setHLSFilesDeleteOnExit(appSettings.isDeleteHLSFilesOnExit());

		encoderAdaptor.setHlsTime(appSettings.getHlsTime());
		encoderAdaptor.setHlsListSize(appSettings.getHlsListSize());
		encoderAdaptor.setHlsPlayListType(appSettings.getHlsPlayListType());

		return encoderAdaptor;
	}

	public void processSignallingLeave(String streamName, WebSocketConnection connection) throws UnsupportedEncodingException {
		List<WebSocketConnection> webSocketConnections = signallingConnections.get(streamName);
		if (webSocketConnections != null)  
		{
			JSONObject jsonResponseObject = new JSONObject();
			jsonResponseObject.put("command", "stop");

			for (Iterator iterator = webSocketConnections.iterator(); iterator.hasNext();) 
			{
				WebSocketConnection webSocketConnection = (WebSocketConnection) iterator.next();
				webSocketConnection.send(jsonResponseObject.toJSONString());
				if (webSocketConnection.equals(connection)) 
				{
					//remove the current connection from room
					iterator.remove();	
				}
			}

			if (webSocketConnections.size() == 0) {
				signallingConnections.remove(streamName);
			}

			//notify current connection that it has been leaved
			JSONObject jsonResponse = new JSONObject();
			jsonResponse.put("command", "notification");
			jsonResponse.put("definition", "leaved");
			connection.send(jsonResponse.toJSONString());


		}
		else {
			JSONObject jsonResponse = new JSONObject();
			jsonResponse.put("command", "error");
			jsonResponse.put("definition", "No peer associated with that stream before");
			connection.send(jsonResponse.toJSONString());
		}

	}

	public boolean processSignallingTakeCandidate(String streamName, WebSocketConnection connection, long sdpMLineIndex, String sdpMid, String sdp) throws UnsupportedEncodingException {
		List<WebSocketConnection> webSocketConnections = signallingConnections.get(streamName);
		boolean result = false;
		if (webSocketConnections != null && webSocketConnections.size() == 2)  
		{
			JSONObject jsonResponseObject = new JSONObject();
			jsonResponseObject.put("command", "takeCandidate");
			jsonResponseObject.put("label", sdpMLineIndex);
			jsonResponseObject.put("id", sdpMid);
			jsonResponseObject.put("candidate", sdp);

			for (WebSocketConnection webSocketConnection : webSocketConnections) {
				if (!webSocketConnection.equals(connection)) {
					webSocketConnection.send(jsonResponseObject.toJSONString());
					result = true;
				}
			}
		}
		else {
			JSONObject jsonResponse = new JSONObject();
			jsonResponse.put("command", "error");
			jsonResponse.put("definition", "No peer associated with that stream before");
			connection.send(jsonResponse.toJSONString());
			result = false;
		}
		return result;

	}

	public boolean processSignallingTakeConf(String streamName, WebSocketConnection connection, String typeString, String sdpDescription) throws UnsupportedEncodingException {
		//if it is in signalling mode, 
		List<WebSocketConnection> webSocketConnections = signallingConnections.get(streamName);
		boolean result = false;
		if (webSocketConnections != null && webSocketConnections.size() == 2) 
		{
			JSONObject jsonResponseObject = new JSONObject();
			jsonResponseObject.put("command", "takeConfiguration");
			jsonResponseObject.put("sdp", sdpDescription);

			jsonResponseObject.put("type", typeString);

			for (WebSocketConnection webSocketConnection : webSocketConnections) {
				if (!webSocketConnection.equals(connection)) {
					webSocketConnection.send(jsonResponseObject.toJSONString());
					result = true;
				}
			}
		}
		else {
			JSONObject jsonResponse = new JSONObject();
			jsonResponse.put("command", "error");
			jsonResponse.put("definition", "No peer associated with that stream before");
			connection.send(jsonResponse.toJSONString());
			result = false;
		}
		return result;

	}

	@Override
	public void onWSMessage(WSMessage message) {

		String msg = new String(message.getPayload().array()).trim();
		log.debug("onWSMessage: {}\n", msg);

		// ignore ping and pong
		if (message.getMessageType() == MessageType.PING || message.getMessageType() == MessageType.PONG) {
			return;
		}
		// close if we get a close
		if (message.getMessageType() == MessageType.CLOSE) {
			message.getConnection().close();
			return;
		}

		//TODO: check that expected fields are present and do not throw exception in any case of failure

		JSONObject jsonObject;
		try {
			jsonObject = (JSONObject) parser.parse(msg);
			WebSocketConnection connection = message.getConnection();
			takeAction(jsonObject, connection);
		} catch (ParseException e) {
			e.printStackTrace();
		}

	}

	@Override
	public void onWSConnect(WebSocketConnection conn) {
		connections.add(conn);
	}

	@Override
	public void onWSDisconnect(WebSocketConnection conn) {
		connections.remove(conn);

		wsSignallingDisconnected(conn);
	}

	public void wsSignallingDisconnected(WebSocketConnection conn) {
		//for signalling connections
		String streamName = (String) conn.getSession().getAttribute(ATTR_STREAM_NAME);
		if (streamName != null) {
			List<WebSocketConnection> list = signallingConnections.get(streamName);

			if (list != null) {
				for (Iterator iterator = list.iterator(); iterator.hasNext();) 
				{
					WebSocketConnection webSocketConnection = (WebSocketConnection) iterator.next();
					if (webSocketConnection.equals(conn)) {
						iterator.remove();
					}
					else {
						JSONObject jsonResponseObject = new JSONObject();
						jsonResponseObject.put("command", "stop");
						try {
							webSocketConnection.send(jsonResponseObject.toJSONString());
						} catch (UnsupportedEncodingException e) {
							e.printStackTrace();
						}
					}
				}

				if (list.size() == 0) {
					signallingConnections.remove(streamName);
				}
			}

		}
	}



	@Override
	public void setApplicationContext(ApplicationContext applicationContext) throws BeansException {
		this.applicationContext = applicationContext;
		webRTCAdaptor = (IWebRTCAdaptor) applicationContext.getBean(IWebRTCAdaptor.BEAN_NAME);

		if (applicationContext.containsBean(AppSettings.BEAN_NAME)) {
			appSettings = (AppSettings)applicationContext.getBean(AppSettings.BEAN_NAME);
		}
		
		log.debug("webrtc adaptor in websocket listener ---> " + webRTCAdaptor);
		
		appAdaptor = (AntMediaApplicationAdapter)applicationContext.getBean("web.handler");
	}

	public Map<String, List<WebSocketConnection>> getSignallingConnections() {
		return signallingConnections;
	}

	@Override
	public void stop() {
		
	}

}
