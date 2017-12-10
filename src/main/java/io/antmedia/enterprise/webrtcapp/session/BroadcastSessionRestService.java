package io.antmedia.enterprise.webrtcapp.session;

import java.util.ArrayList;
import java.util.List;

import javax.servlet.ServletContext;
import javax.servlet.http.HttpServletRequest;
import javax.ws.rs.Consumes;
import javax.ws.rs.FormParam;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.QueryParam;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;

import org.springframework.context.ApplicationContext;
import org.springframework.stereotype.Component;
import org.springframework.web.context.WebApplicationContext;

import io.antmedia.AntMediaApplicationAdapter;
import io.antmedia.datastore.db.types.Broadcast;
import io.antmedia.datastore.db.types.Endpoint;
import io.antmedia.enterprise.social.endpoint.FacebookEndpoint;
import io.antmedia.enterprise.social.endpoint.YoutubeEndpoint;
import io.antmedia.enterprise.webrtcapp.WebRTCApplication;
import io.antmedia.rest.BroadcastRestService;
import io.antmedia.rest.BroadcastRestService.Result;
import io.antmedia.social.endpoint.PeriscopeEndpoint;
import io.antmedia.social.endpoint.VideoServiceEndpoint;
import io.antmedia.social.endpoint.VideoServiceEndpoint.DeviceAuthParameters;

@Component
@Path("/")
public class BroadcastSessionRestService extends BroadcastRestService{

	private static final String FACEBOOK_CLIENT_ID = "1898164600457124";
	private static final String FACEBOOK_CLIENT_SECRET = "51828acd849db391e1d7f3ae58cffff8";

	private static final String YOUTUBE_CLIENT_ID = "560273359199-95s13urkod9lo0srbjpr4e5c70vdj2f6.apps.googleusercontent.com";
	private static final String YOUTUBE_CLIENT_SECRET = "p5LbrEZf6B6tKo3c4Dna8ejV";


	private static final String PERISCOPE_CLIENT_ID = "Q90cMeG2gUzC6fImXcp2SvyVqwVSvGDlsFsRF4Uia9NR1M-Zru";
	private static final String PERISCOPE_CLIENT_SECRET = "dBCjxFbawo436VSWMvuD5SDSZoSdhew_-Fvrh5QhrBXuKoelVM";

	private static String SESSION_STORE_NAME = "session_store"; 

	@Context
	private HttpServletRequest request;
	
	@Context
	private ServletContext servletContext;

	List<VideoServiceEndpoint> endPoint ;
	
	private AntMediaApplicationAdapter app;
	private ApplicationContext appCtx;

	@Override
	public List<VideoServiceEndpoint> getEndpointList() {
		if (endPoint == null) {
			endPoint = new ArrayList();
			
			//PreferenceSessionStore sessionStore = new PreferenceSessionStore(null);
			PreferenceSessionStore sessionStore = (PreferenceSessionStore) request.getSession().getAttribute(SESSION_STORE_NAME);
			if (sessionStore == null) {
				sessionStore = new PreferenceSessionStore(null);
				request.getSession().setAttribute(SESSION_STORE_NAME, sessionStore);
			}
			endPoint.add(new FacebookEndpoint(FACEBOOK_CLIENT_ID, FACEBOOK_CLIENT_SECRET, sessionStore));
			endPoint.add(new PeriscopeEndpoint(PERISCOPE_CLIENT_ID, PERISCOPE_CLIENT_SECRET, sessionStore));
			endPoint.add(new YoutubeEndpoint(YOUTUBE_CLIENT_ID, YOUTUBE_CLIENT_SECRET, sessionStore));
			
			for (VideoServiceEndpoint videoServiceEndpoint : endPoint) {
				videoServiceEndpoint.start();
			}
		}
		return endPoint;
	}
	
	private ApplicationContext getAppContext() {
		if (appCtx == null && servletContext != null) {
			appCtx = (ApplicationContext) servletContext.getAttribute(WebApplicationContext.ROOT_WEB_APPLICATION_CONTEXT_ATTRIBUTE);
		}
		return appCtx;
	}
	
	protected AntMediaApplicationAdapter getApplication() {
		if (app == null) {
			app = (AntMediaApplicationAdapter) getAppContext().getBean("web.handler");
		}
		return app;
	}
	
	
	@GET
	@Path("/broadcast/isAvailable")
	@Produces(MediaType.APPLICATION_JSON)
	public Result isAvailable() 
	{
		boolean result = false;
		WebRTCApplication app = (WebRTCApplication)getApplication();
		if (app.getNumberOfLiveStreams() < 1) {
			result = true;
		}
		return new Result(result, null);
	}

	
	@POST
	@Consumes(MediaType.APPLICATION_FORM_URLENCODED)
	@Path("/broadcast/addSocialEndpoint")
	@Produces(MediaType.APPLICATION_JSON)
	@Override
	public Result addSocialEndpoint(@FormParam("id") String id, @FormParam("serviceName") String serviceName) 
	{
		Result result = super.addSocialEndpoint(id, serviceName);
		
		if (result.success) {
			WebRTCApplication application = (WebRTCApplication) getApplication();
			
			application.putEndpoints(id, getEndpointList());
		}
		
		return result;
	}
	
	
	
	


}
