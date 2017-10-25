var wsConn;

var pc_config = {
	'iceServers': [{
		'url': 'stun:stun.l.google.com:19302'
	}]
};

var pcConstraints = {
	"optional": [{
		"DtlsSrtpKeyAgreement": true
	}]
};

var sdpConstraints = {
	'mandatory': {
		'OfferToReceiveAudio': false,
		'OfferToReceiveVideo': false
	}
};

var facebookEndpointAdded;
var youtubeEndpointAdded;
var periscopeEndpointAdded;


if ("WebSocket" in window) {
	wsConn = new WebSocket("ws://" + location.hostname + ":8081/WebRTCApp4");

	wsConn.onopen = function () {
		console.log("websocket connected");
	}

	wsConn.onmessage = function (event) {
		obj = JSON.parse(event.data);
		console.log(event.data);
		if (obj.command == "start") {
			if (typeof localStream != "undefined") {
				remotePeerConnection = new RTCPeerConnection(pc_config,
					pcConstraints);
				remotePeerConnection.addStream(localStream);

				remotePeerConnection.createOffer(gotDescription,
					function () {
						console.log("create offer error");
					}, sdpConstraints);
			}

		}
		if (obj.command == "takeCandidate") {

			var candidate = new RTCIceCandidate({
				sdpMLineIndex: obj.label,
				candidate: obj.candidate
			});
			remotePeerConnection.addIceCandidate(candidate);
			console.log("received ice candidate");
			console.log(obj.candidate);
		} else if (obj.command == "takeConfiguration") {

			remotePeerConnection
				.setRemoteDescription(new RTCSessionDescription({
					sdp: obj.sdp,
					type: obj.type
				}));
			console.log("received remote description:")
			console.log(obj.sdp);

			startAnimation();
			hideLoader();

		}

	}

	wsConn.onerror = function (hata) {
		console.log(" error occured: " + hata);
	}

	wsConn.onclose = function (olay) {
		console.log("connection closed.");
	}

} else {
	alert("Your browser does not support WebSocket. You may want to use Chrome or Firefox");
}

function addSocialEndpoint(streamId, serviceName) {
	fetch("rest/broadcast/addSocialEndpoint",
		{
			method: 'post',
			headers: {
				"Content-type": "application/x-www-form-urlencoded"
			},
			credentials: 'include',
			body: "id=" + streamId + "&serviceName="+serviceName
		}
	).then(function (response) {
		response.json().then(function(data) {
			console.log("add social endpoint result:" + data.success + " for " + serviceName);
			if (data.success == true) {
				if (serviceName == "facebook") {
					facebookEndpointAdded = true;
				} 
				else if(serviceName == "youtube"){
					youtubeEndpointAdded = true;
				}
				else if(serviceName == "periscope") {
					periscopeEndpointAdded = true;
				}
			}
			else {
				var message = "Service(" +serviceName +") cannot be added.";
				if (serviceName == "youtube") {
					message += " Please make sure that you have enabled your youtube account for live streaming";
				}
				else {
					message += " Please report this issue.";
				}
				alert(message);
			}
			
		})
	});
}
function createBroadcast() {
	var jsonCreateBroadcast = {
		name: streamNameBox.value
	};
	fetch('rest/broadcast/create',
		{
			method: 'post',
			headers: {
				"Content-type": "application/json"
			},
			credentials: 'include',
			body: JSON.stringify(jsonCreateBroadcast)
		}
	).then(function (response) {

		response.json().then(function (data) {
			console.log(data.streamId);

			facebookEndpointAdded = true;
			if ($("#facebook-checkbox").is(":checked") == true && facebookAuthenticated) {
				facebookEndpointAdded = false;
				addSocialEndpoint(data.streamId, "facebook");
			}

			youtubeEndpointAdded = true;
			if ($("#youtube-checkbox").is(":checked") == true && youtubeAuthenticated) {
				youtubeEndpointAdded = false;
				addSocialEndpoint(data.streamId, "youtube");
			}

			periscopeEndpointAdded = true;
			if ($("#periscope-checkbox").is(":checked") == true && periscopeAuthenticated) {
				periscopeEndpointAdded = false;
				addSocialEndpoint(data.streamId, "periscope");
			}

			setTimeout(triggerPublish, 1000, data.streamId);

		});

	});
}

function triggerPublish(streamId) {
	if (periscopeEndpointAdded == true && youtubeEndpointAdded == true && 
		facebookEndpointAdded == true) {
		var message = "Broadcasting - Check your ";
		if ($("#facebook-checkbox").is(":checked")) {
			message += "facebook"
		}
		if ($("#periscope-checkbox").is(":checked")) {
			if ($("#facebook-checkbox").is(":checked") && $("#youtube-checkbox").is(":checked")) {
				message += ","
			}
			else if ($("#facebook-checkbox").is(":checked") && !$("#youtube-checkbox").is(":checked")) {
				message += " or ";
			}
			
			message += "periscope"
		}
		if ($("#youtube-checkbox").is(":checked")) {
			if ($("#facebook-checkbox").is(":checked") || $("#periscope-checkbox").is(":checked")) {
				message += " or ";
			}
			message += "youtube(live streaming)"
		}
		
		message += " profile to watch";
		
		$("#broadcastingInfo").text(message);
		var jsCmd;
		{
			jsCmd = {
				command: "publish",
				streamName: streamId,
			};
		}

		wsConn.send(JSON.stringify(jsCmd));

		$("#start_publishing_button").text("Stop");
	}
	else {
		setTimeout(triggerPublish, 1000, streamId);
	}
	

}

function Start() {

	if (typeof remotePeerConnection == "undefined"
		|| remotePeerConnection.signalingState == "closed") 
	
	{
		fetch('rest/broadcast/isAvailable',
				{
					method: 'get',
					credentials: 'include',
				}
			).then(function(response) {
				
				response.json().then(function(data) {
					
					if (data.success) 
					{
						/*
						if ($("#facebook-checkbox").is(":checked") 
								|| $("#youtube-checkbox").is(":checked")
								|| $("#periscope-checkbox").is(":checked")) 
						*/
						{
							showLoader();
							createBroadcast();
						}
						/*
						else {
							alert("Please check at least one box among Facebook, Youtube or Periscope boxes");
						}
						*/
					}
					else {
						alert("Server is loaded, please try again later");
					}
				});	
			});	
	}
	else {
		Stop();
		$("#start_publishing_button").text("Start Publishing");
	}
}

function Stop() {

	var jsCmd;
	{
		jsCmd = {
			command: "stop",
		};
	}

	try {
	   wsConn.send(JSON.stringify(jsCmd));
	}
	catch (e) {
		
	}
	if (typeof remotePeerConnection != "undefined"
		&& remotePeerConnection.signalingState != "closed") {
		remotePeerConnection.close();
	}

}

function gotStream(stream) {
    $(".mdl-button").prop("disabled", false);
	$("input[type=checkbox]").prop("disabled", false);

	localVideo.srcObject = stream;
	localStream = stream;
}

function gotDescription(configuration) {
	remotePeerConnection.setLocalDescription(configuration);

	jsCmd = {
		command: "takeConfiguration",
		type: configuration.type,
		sdp: configuration.sdp

	};

	wsConn.send(JSON.stringify(jsCmd));
	console.log("yerel configuration alindi. ", configuration);
}


