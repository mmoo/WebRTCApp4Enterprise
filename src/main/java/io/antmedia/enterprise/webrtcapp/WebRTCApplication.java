package io.antmedia.enterprise.webrtcapp;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.red5.logging.Red5LoggerFactory;
import org.red5.net.websocket.WebSocketPlugin;
import org.red5.net.websocket.WebSocketScope;
import org.red5.net.websocket.WebSocketScopeManager;
import org.red5.net.websocket.listener.IWebSocketDataListener;
import org.red5.server.adapter.MultiThreadedApplicationAdapter;
import org.red5.server.api.scope.IScope;
import org.red5.server.api.stream.IBroadcastStream;
import org.red5.server.plugin.PluginRegistry;
import org.slf4j.Logger;
import org.springframework.beans.BeansException;
import org.springframework.context.ApplicationContext;
import org.springframework.context.ApplicationContextAware;
import org.springframework.scheduling.concurrent.ThreadPoolTaskScheduler;

import io.antmedia.AntMediaApplicationAdapter;
import io.antmedia.social.endpoint.VideoServiceEndpoint;

public class WebRTCApplication extends AntMediaApplicationAdapter implements ApplicationContextAware{


	private static final Logger logger = Red5LoggerFactory.getLogger(WebRTCApplication.class);

	private ApplicationContext applicationContext;
	private ThreadPoolTaskScheduler taskScheduler;
	private Map<String, Object> endpointMap = new HashMap<>();

	public int numberOfLiveStreams;
	
	@Override
	public void setApplicationContext(ApplicationContext applicationContext) throws BeansException {
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
	
	public ThreadPoolTaskScheduler getTaskScheduler() {
		return taskScheduler;
	}

	public void setTaskScheduler(ThreadPoolTaskScheduler taskScheduler) {
		this.taskScheduler = taskScheduler;
	}

	@Override
    public void appStop(IScope scope) {
      //  log.info("Chat stopping");
        // remove our app
        WebSocketScopeManager manager = ((WebSocketPlugin) PluginRegistry.getPlugin("WebSocketPlugin")).getManager(scope);
        manager.removeApplication(scope);
        manager.stop();
        super.appStop(scope);
    }
	
	@Override
	public void streamPublishStart(IBroadcastStream stream) {
		super.streamPublishStart(stream);
		numberOfLiveStreams++;	
	}
	
	
	@Override
	public void streamBroadcastClose(IBroadcastStream stream) {
		super.streamBroadcastClose(stream);
		endpointMap.remove(stream.getPublishedName());
		numberOfLiveStreams--;
	}
	
	public int getNumberOfLiveStreams() {
		return numberOfLiveStreams;
	}

	public void putEndpoints(String id, List<VideoServiceEndpoint> endpointList) {
		endpointMap.put(id, endpointList);
	}
	
	@Override
	protected VideoServiceEndpoint getVideoServiceEndPoint(IBroadcastStream stream, String type) 
	{
		if (endpointMap.containsKey(stream.getPublishedName())) {
			List<VideoServiceEndpoint> streamEndpoints = (List<VideoServiceEndpoint>) endpointMap.get(stream.getPublishedName());
			
			for (VideoServiceEndpoint serviceEndpoint : streamEndpoints) {
				if (serviceEndpoint.getName().equals(type)) {
					return serviceEndpoint;
				}
			}
		}
		return null;
	}

}
