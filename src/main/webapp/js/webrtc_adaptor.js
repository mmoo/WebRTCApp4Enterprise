

/**
 * 
 * @returns
 */


function WebRTCAdaptor(initialValues) 
{
	var thiz = this;
	thiz.peerconnection_config = null;
	thiz.sdp_constraints = null;
	thiz.remotePeerConnection = null;
	thiz.initializer = false;
	thiz.webSocketAdaptor = null;
	thiz.streamName = null;
	/*
	  {
		  websocket_url:"ws://" + location.hostname + ":8081/WebRTCApp4",
		  mediaConstraints: mediaConstraints,
		  localVideoId: localVideo,
		  remoteVideoId: remoteVideo,
		  callback: function() {
			  console.log("success callback");
		  },
		  callbackError: function(error) {
			  console.log("error callback" + error);
		  }
	  }
	 */
	for(var key in initialValues) {
		if(initialValues.hasOwnProperty(key)) {
			this[key] = initialValues[key];
		}
	}

	thiz.localVideo = document.getElementById(thiz.localVideoId);
	thiz.remoteVideo = document.getElementById(thiz.remoteVideoId);

	if (!("WebSocket" in window)) {
		console.log("WebSocket not supported.");
		callbackError("WebSocketNotSupported");
		return;
	}


	console.log("websocket url: " + this.websocket_url);


	this.join = function(streamName) {
		thiz.streamName = streamName;

		var jsCmd;
		{
			jsCmd = {
					command : "join",
					streamName : streamName,
			};
		}

		thiz.webSocketAdaptor.send(JSON.stringify(jsCmd));
	}

	this.leave = function () {

		var jsCmd;
		{

			jsCmd = {
					command : "leave",
					streamName: thiz.streamName,
			};
		}

		thiz.webSocketAdaptor.send(JSON.stringify(jsCmd));

		thiz.closePeerConnection();
	}

	gotStream = function (stream) {
		thiz.localStream = stream;
		thiz.localVideo.srcObject = stream;
		thiz.webSocketAdaptor = new thiz.WebSocketAdaptor();
	};

	navigator.mediaDevices.getUserMedia(mediaConstraints)
	.then(gotStream)
	.catch(function(error) {
		thiz.callbackError(error.name);		
	});


	this.onTrack = function(event) {
		thiz.remoteVideo.srcObject = event.streams[0];
	}

	this.iceCandidateReceived = function(event) {
		if (event.candidate) {

			jsCmd = {
					command : "takeCandidate",
					streamName : thiz.streamName,
					label : event.candidate.sdpMLineIndex,
					id : event.candidate.sdpMid,
					candidate : event.candidate.candidate
			};

			thiz.webSocketAdaptor.send(JSON.stringify(jsCmd));
		}
	}


	this.initPeerConnection = function() {
		if (thiz.remotePeerConnection == null) {
			thiz.remotePeerConnection = new RTCPeerConnection(thiz.peerconnection_config);
			thiz.remotePeerConnection.onicecandidate = thiz.iceCandidateReceived;
			thiz.remotePeerConnection.ontrack = thiz.onTrack;
		}
	}

	this.closePeerConnection = function() {
		thiz.initializer = false;
		if (thiz.remotePeerConnection != null
				&& thiz.remotePeerConnection.signalingState != "closed") {
			thiz.remotePeerConnection.close();
			thiz.remotePeerConnection = null;
		}
	}

	this.gotDescription = function(configuration) {
		thiz.remotePeerConnection.setLocalDescription(configuration);

		jsCmd = {
				command : "takeConfiguration",
				streamName : thiz.streamName,
				type : configuration.type,
				sdp : configuration.sdp

		};

		thiz.webSocketAdaptor.send(JSON.stringify(jsCmd));
	}


	this.WebSocketAdaptor = function() {
		var wsConn = new WebSocket(thiz.websocket_url);

		wsConn.onopen = function() {
			console.log("websocket connected");
			thiz.callback("initialized");
		}

		this.send = function(text) {
			wsConn.send(text);
		}

		wsConn.onmessage = function(event) {
			obj = JSON.parse(event.data);
			//console.log(obj);
			if (obj.command == "start") {
				thiz.initializer = true;
				thiz.initPeerConnection();

				console.log("received start command");
				thiz.remotePeerConnection.addStream(thiz.localStream);

				thiz.remotePeerConnection.createOffer(thiz.sdp_constraints)
					.then(thiz.gotDescription)
					.catch(function () {
						console.log("create offer error");
					});

			}
			else if (obj.command == "takeCandidate") {

				thiz.initPeerConnection();
				var candidate = new RTCIceCandidate({
					sdpMLineIndex : obj.label,
					candidate : obj.candidate
				});
				thiz.remotePeerConnection.addIceCandidate(candidate);
				console.log("received ice candidate");

			} else if (obj.command == "takeConfiguration") {

				thiz.initPeerConnection();
				thiz.remotePeerConnection
				.setRemoteDescription(new RTCSessionDescription({
					sdp : obj.sdp,
					type : obj.type
				}));
				console.log("received remote description:")

				if (thiz.initializer == false) {
					thiz.remotePeerConnection.addStream(thiz.localStream);
					thiz.remotePeerConnection.createAnswer(thiz.sdp_constraints)
						.then(thiz.gotDescription)
						.catch(function() {
							console.log("create offer error");
						});
				}
			}
			else if (obj.command == "stop") {
				thiz.closePeerConnection();
			}
			else if (obj.command == "error") {
				thiz.callbackError(obj.definition);
			}
			else if (obj.command == "notification") {
				thiz.callback(obj.definition);
			}

		}

		wsConn.onerror = function(error) {
			console.log(" error occured: " + error);
			thiz.callbackError(error)
		}

		wsConn.onclose = function(event) {
			console.log("connection closed.");
		}
	};




}

