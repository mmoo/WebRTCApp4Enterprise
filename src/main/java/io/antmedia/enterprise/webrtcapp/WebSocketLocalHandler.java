package io.antmedia.enterprise.webrtcapp;

import javax.websocket.server.ServerEndpoint;

import io.antmedia.enterprise.webrtc.ServletAwareConfig;
import io.antmedia.enterprise.webrtc.WebSocketHandler;

@ServerEndpoint(value="/websocket", configurator=ServletAwareConfig.class)
public class WebSocketLocalHandler extends WebSocketHandler {

}
