package io.antmedia.enterprise.webrtcapp.session;

import java.util.HashMap;
import java.util.Map;

import org.springframework.context.annotation.Scope;
import org.springframework.stereotype.Component;
import org.springframework.context.annotation.ScopedProxyMode;

import io.antmedia.datastore.preference.PreferenceStore;

public class PreferenceSessionStore extends PreferenceStore{

	Map<String, String> map = new HashMap();
	
	public PreferenceSessionStore(String fileName) {
		super(fileName);
		
	}
	
	@Override
	public void put(String key, String value) {
		map.put(key, value);
	}
	
	@Override
	public String get(String key) {
		return map.get(key);
		
	}
	
	@Override
	public boolean save() {
		return true;
	}

}
