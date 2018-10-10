package io.antmedia.enterprise.webrtcapp;

import org.red5.net.websocket.WebSocketPlugin;
import org.red5.net.websocket.WebSocketScope;
import org.red5.net.websocket.WebSocketScopeManager;
import org.red5.server.api.scope.IScope;
import org.red5.server.plugin.PluginRegistry;
import org.springframework.context.ApplicationContext;
import org.springframework.context.ApplicationContextAware;

import io.antmedia.enterprise.webrtc.WebRTCApplication;

public class WebRTCSampleApplication extends WebRTCApplication implements ApplicationContextAware{
	
	static WebRTCSampleApplication application;
	
	
	public static WebRTCSampleApplication getApplication() {
		return application;
	}
	
	
	@Override
	public void setApplicationContext(ApplicationContext applicationContext)  {
		this.applicationContext = applicationContext;
	}
	
	
	@Override
	public boolean appStart(IScope app) {
		application = this;

		return super.appStart(app);
	}
	

}
