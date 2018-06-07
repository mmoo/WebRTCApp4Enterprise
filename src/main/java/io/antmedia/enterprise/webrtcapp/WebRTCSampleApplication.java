package io.antmedia.enterprise.webrtcapp;

import java.util.HashMap;
import java.util.Map;

import org.red5.logging.Red5LoggerFactory;
import org.red5.net.websocket.WebSocketPlugin;
import org.red5.net.websocket.WebSocketScope;
import org.red5.net.websocket.WebSocketScopeManager;
import org.red5.server.api.scope.IScope;
import org.red5.server.api.stream.IBroadcastStream;
import org.red5.server.plugin.PluginRegistry;
import org.slf4j.Logger;
import org.springframework.beans.BeansException;
import org.springframework.context.ApplicationContext;
import org.springframework.context.ApplicationContextAware;
import org.springframework.scheduling.concurrent.ThreadPoolTaskScheduler;

import io.antmedia.enterprise.webrtc.WebRTCApplication;

public class WebRTCSampleApplication extends WebRTCApplication implements ApplicationContextAware{
	
	
	@Override
	public void setApplicationContext(ApplicationContext applicationContext)  {
		this.applicationContext = applicationContext;
	}
	
	
	@Override
	public boolean appStart(IScope app) {
		// get the websocket plugin
		WebSocketPlugin wsPlugin = (WebSocketPlugin) PluginRegistry.getPlugin("WebSocketPlugin");
		// add this application to it
		wsPlugin.setApplication(this);
		// get the manager
		WebSocketScopeManager manager = wsPlugin.getManager(app);
		// get the ws scope
		WebSocketScope defaultWebSocketScope = (WebSocketScope) applicationContext.getBean("webSocketScopeDefault");
		// add the ws scope
		manager.addWebSocketScope(defaultWebSocketScope);
		return super.appStart(app);
	}
	
	
	@Override
    public void appStop(IScope scope) {
        // remove our app
        WebSocketScopeManager manager = ((WebSocketPlugin) PluginRegistry.getPlugin("WebSocketPlugin")).getManager(scope);
        manager.removeApplication(scope);
        manager.stop();
        super.appStop(scope);
    }


}
