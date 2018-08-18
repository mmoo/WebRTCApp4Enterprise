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
		System.out.println("\n\n\n"+app+"\n\n\n");
		
		
		application = this;
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
