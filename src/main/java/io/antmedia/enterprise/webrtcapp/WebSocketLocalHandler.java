package io.antmedia.enterprise.webrtcapp;

import javax.websocket.EndpointConfig;
import javax.websocket.OnOpen;
import javax.websocket.Session;
import javax.websocket.server.ServerEndpoint;
import javax.ws.rs.core.Context;

import org.apache.tomcat.websocket.server.DefaultServerEndpointConfigurator;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContext;
import org.springframework.stereotype.Component;

import io.antmedia.enterprise.webrtc.WebSocketHandler;


@ServerEndpoint(value="/websocket", configurator=DefaultServerEndpointConfigurator.class)
public class WebSocketLocalHandler extends WebSocketHandler {
	
	
	private ApplicationContext appCtx; 
	
	protected static Logger logger = LoggerFactory.getLogger(WebSocketLocalHandler.class);

	@Override
	public ApplicationContext getAppContext() {
		if (appCtx == null) {
			appCtx = WebRTCSampleApplication.getApplication().getContext().getApplicationContext();
		}
		return appCtx;
	}

}
